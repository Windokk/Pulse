#pragma once

#include "reflection_types.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>
#include <typeinfo>
#include <cxxabi.h>
#include <memory>

enum FieldFlags : uint32_t {
    Editable     = 1 << 0,
    ReadOnly     = 1 << 1
};

struct Container{
    // element info
    TypeID elementType;
    size_t elementSize;

    TypeID editorElementType;
    
    // common queries
    size_t (*size)(void* container);
    bool   (*isAssociative)();

    // element conversion
    void (*elementRead)(const void* element, void* outEditorValue);
    void (*elementWrite)(void* component, void* element, const void* editorValue);

    // SEQUENTIAL containers (vectors)
    void* (*getByIndex)(void* container, size_t index);
    void  (*insertAt)(void* container, size_t index, const void* element);
    void  (*eraseAt)(void* container, size_t index);

    // ASSOCIATIVE containers (map, set)
    void* (*findByKey)(void* container, const void* key);
    void  (*insertByKey)(void* container, const void* key, const void* value);
    void  (*eraseByKey)(void* container, const void* key);

    // lifecycle
    void (*clear)(void* container);
};

struct FieldInfo {
    // Identity
    const char* name;         // Field name
    TypeID type;      // Type stored in the component

    // Memory access
    uint32_t offset;          // Offset in the struct (used if no getter/setter)

    // Behavior

    // Optional getter/setter functions. If nullptr, read/write directly using offset.
    void (*read)(void* object, void* out_value);
    void (*write)(void* object, const void* value);

    // Helpers
    void (*copy)(void* src, const void* dst);  // Copies value
    bool (*equals)(const void* a, const void* b); // Compares values

    // Editor metadata

    uint32_t flags;           // Editable / ReadOnly
    float min;                // Optional min value for editor widgets
    float max;                // Optional max value for editor widgets

    const Container* container = nullptr;
    const EnumDescriptor* enumDesc = nullptr;
};

struct ClassDescriptor{
    std::string name;
    std::vector<FieldInfo*> fields;
};

struct StructDescriptor{
    std::string name;
    std::vector<FieldInfo*> fields;
};

inline std::string demangle(const char* name) {
    int status = 0;
    std::unique_ptr<char, void(*)(void*)> res{
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
}

template<typename StorageT, typename EditorT = StorageT>
Container MakeVectorContainer(
    void (*elementRead)(const void*, void*) = nullptr,
    void (*elementWrite)(void*, void*, const void*) = nullptr
    ) {
    Container c{};

    c.elementType = GetTypeIDFromString(demangle(typeid(StorageT).name()));
    c.elementSize = sizeof(StorageT);
    c.editorElementType = GetTypeIDFromString(demangle(typeid(EditorT).name()));

    if constexpr (std::is_same_v<StorageT, EditorT>) {
        c.elementRead = elementRead ? elementRead :
            [](const void* e, void* out) {
                *static_cast<EditorT*>(out) =
                    *static_cast<const StorageT*>(e);
            };

        c.elementWrite = elementWrite ? elementWrite :
            [](void* e, const void* in) {
                *static_cast<StorageT*>(e) =
                    *static_cast<const EditorT*>(in);
            };
    } else {
        c.elementRead = elementRead;
        c.elementWrite = elementWrite;
    }

    c.size = [](void* c) -> size_t {
        return static_cast<std::vector<StorageT>*>(c)->size();
    };

    c.isAssociative = []() { return false; };

    c.getByIndex = [](void* c, size_t i) -> void* {
        return &(*static_cast<std::vector<StorageT>*>(c))[i];
    };

    c.insertAt = [](void* c, size_t i, const void* v) {
        auto& vec = *static_cast<std::vector<StorageT>*>(c);
        vec.insert(vec.begin() + i, *static_cast<const StorageT*>(v));
    };

    c.eraseAt = [](void* c, size_t i) {
        auto& vec = *static_cast<std::vector<StorageT>*>(c);
        vec.erase(vec.begin() + i);
    };

    c.clear = [](void* c) {
        static_cast<std::vector<StorageT>*>(c)->clear();
    };

    return c;
}

template<typename K, typename V, typename EditorV = V>
Container MakeMapContainer(
    void (*elementRead)(const void*, void*) = nullptr,
    void (*elementWrite)(void*, void*, const void*) = nullptr
) {
    Container c{};

    c.elementType = GetTypeIDFromString(demangle(typeid(V).name()));
    c.elementSize = sizeof(V);
    c.editorElementType = GetTypeIDFromString(demangle(typeid(EditorV).name()));

    if constexpr (std::is_same_v<V, EditorV>) {
        c.elementRead = elementRead ? elementRead :
            [](const void* e, void* out) {
                *static_cast<EditorV*>(out) =
                    *static_cast<const V*>(e);
            };

        c.elementWrite = elementWrite ? elementWrite :
            [](void* e, const void* in) {
                *static_cast<V*>(e) =
                    *static_cast<const EditorV*>(in);
            };
    } else {
        c.elementRead = elementRead;
        c.elementWrite = elementWrite;
    }

    c.size = [](void* c) -> size_t {
        return static_cast<std::map<K, V>*>(c)->size();
    };

    c.isAssociative = []() { return true; };

    // sequential ops unused
    c.getByIndex = nullptr;
    c.insertAt = nullptr;
    c.eraseAt = nullptr;

    // associative ops
    c.findByKey = [](void* c, const void* key) -> void* {
        auto& m = *static_cast<std::map<K, V>*>(c);
        auto it = m.find(*static_cast<const K*>(key));
        return it == m.end() ? nullptr : &it->second;
    };

    c.insertByKey = [](void* c, const void* key, const void* value) {
        auto& m = *static_cast<std::map<K, V>*>(c);
        m[*static_cast<const K*>(key)] =
            *static_cast<const V*>(value);
    };

    c.eraseByKey = [](void* c, const void* key) {
        static_cast<std::map<K, V>*>(c)->erase(
            *static_cast<const K*>(key));
    };

    c.clear = [](void* c) {
        static_cast<std::map<K, V>*>(c)->clear();
    };

    return c;
}

inline void* FieldRead(const FieldInfo& field, void* object, void* scratchBuffer)
{
    uint8_t* base = static_cast<uint8_t*>(object) + field.offset;

    if (field.container)
    {
        // Value lives inside the object
        return base;
    }

    // Non-container: copy into scratch buffer
    if (field.read)
    {
        field.read(object, scratchBuffer);
    }
    else
    {
        std::memcpy(scratchBuffer, base, GetTypeSize(field.type));
    }

    return scratchBuffer;
}

inline void FieldWrite(const FieldInfo& field, void* object, const void* value) {
    
    if (field.write) {
        field.write(object, value);
    } else {
        std::memcpy(
            static_cast<uint8_t*>(object) + field.offset,
            value,
            GetTypeSize(field.type)
        );
    }
}