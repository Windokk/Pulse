#pragma once

#include "reflection_types.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>

enum FieldFlags : uint32_t {
    Editable     = 1 << 0,
    ReadOnly     = 1 << 1
};

struct Container{
    // element info
    TypeID elementType;
    size_t elementSize;

    // common queries
    size_t (*size)(void* container);
    bool   (*isAssociative)();

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

struct ComponentDescriptor{
    std::string name;
    std::vector<FieldInfo*> fields;
};

template<typename T>
Container MakeVectorContainer() {
    static Container container {
        // elementType 
        GetTypeIDFromString(typeid(T).name()),

        // elementSize
        sizeof(T),

        // size
        [](void* container) -> size_t {
            return static_cast<std::vector<T>*>(container)->size();
        },

        // isAssociative
        []() { return false; },

        // getByIndex
        [](void* container, size_t i) -> void* {
            return &(*static_cast<std::vector<T>*>(container))[i];
        },

        // insertAt
        [](void* container, size_t i, const void* v) {
            auto& cont = *static_cast<std::vector<T>*>(container);
            cont.insert(cont.begin() + i, *static_cast<const T*>(v));
        },

        // eraseAt
        [](void* container, size_t i) {
            auto& cont = *static_cast<std::vector<T>*>(container);
            cont.erase(cont.begin() + i);
        },

        // findByKey
        nullptr,
        // insertByKey
        nullptr,
        // eraseByKey
        nullptr,

        // clear
        [](void* container) {
            static_cast<std::vector<T>*>(container)->clear();
        }
    };

    return container;
}

template<typename K, typename V>
Container MakeMapContainer() {
    static Container container {
        // elementType 
        GetTypeIDFromString(typeid(V).name()),

        // elementSize
        sizeof(V),

        // size
        [](void* c) -> size_t {
            return static_cast<std::map<K, V>*>(c)->size();
        },

        // isAssociative
        []() { return true; },

        // getByIndex
        nullptr,

        // insertAt
        nullptr,

        // eraseAt
        nullptr,

        // findByKey
        [](void* container, const void* key) -> void* {
            auto& m = *static_cast<std::map<K,V>*>(container);
            auto it = m.find(*static_cast<const K*>(key));
            return it == m.end() ? nullptr : &it->second;
        },

        /* insertByKey */
        [](void* container, const void* key, const void* value) {
            auto& m = *static_cast<std::map<K,V>*>(container);
            m[*static_cast<const K*>(key)] =
                *static_cast<const V*>(value);
        },

        /* eraseByKey */
        [](void* container, const void* key) {
            static_cast<std::map<K,V>*>(container)->erase(
                *static_cast<const K*>(key));
        },

        /* clear */
        [](void* container) {
            static_cast<std::map<K,V>*>(container)->clear();
        }
    };

    return container;
}