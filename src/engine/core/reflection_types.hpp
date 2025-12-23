#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <algorithm>s
#include <vector>

enum FieldFlags : uint32_t {
    Editable     = 1 << 0,
    ReadOnly     = 1 << 1
};

struct ComponentDescriptor{
    std::string name;
    std::vector<FieldInfo*> fields;
};

enum class TypeID : uint16_t {
    // Invalid / base
    Unknown = 0,

    // Integer types
    Int8,
    Int16,
    Int32,
    Int64,

    UInt8,
    UInt16,
    UInt32,
    UInt64,

    // Floating point
    Float,
    Double,

    // Boolean
    Bool,

    // Math vectors
    // (glm / custom)
    Vec2,
    Vec3,
    Vec4,

    IVec2,
    IVec3,
    IVec4,

    UVec2,
    UVec3,
    UVec4,

    // Matrices
    Mat2,
    Mat3,
    Mat4,

    // Quaternion
    Quat,

    // Colors
    ColorRGB,
    ColorRGBA,

    // Strings
    String,        // std::string
    CString,       // const char*

    // Containers
    // (editor usually handles these specially)
    Array,
    Vector,
    Map,
    Set,

    // User-defined / fallback
    Struct,
    Enum
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
};

static TypeID GetTypeIDFromString(const std::string& typeName) {
    static const std::unordered_map<std::string, TypeID> typeMap = {
        // Integer types
        {"int8_t", TypeID::Int8},
        {"int16_t", TypeID::Int16},
        {"int32_t", TypeID::Int32},
        {"int64_t", TypeID::Int64},

        {"uint8_t", TypeID::UInt8},
        {"uint16_t", TypeID::UInt16},
        {"uint32_t", TypeID::UInt32},
        {"uint64_t", TypeID::UInt64},

        // Floating point
        {"float", TypeID::Float},
        {"double", TypeID::Double},

        // Boolean
        {"bool", TypeID::Bool},

        // Math vectors (glm / custom)
        {"glm::vec2", TypeID::Vec2},
        {"glm::vec3", TypeID::Vec3},
        {"glm::vec4", TypeID::Vec4},

        {"glm::ivec2", TypeID::IVec2},
        {"glm::ivec3", TypeID::IVec3},
        {"glm::ivec4", TypeID::IVec4},

        {"glm::uvec2", TypeID::UVec2},
        {"glm::uvec3", TypeID::UVec3},
        {"glm::uvec4", TypeID::UVec4},

        // Matrices
        {"glm::mat2", TypeID::Mat2},
        {"glm::mat3", TypeID::Mat3},
        {"glm::mat4", TypeID::Mat4},

        // Quaternion
        {"glm::quat", TypeID::Quat},

        // Colors
        {"ColorRGB", TypeID::ColorRGB},
        {"ColorRGBA", TypeID::ColorRGBA},

        // Strings
        {"std::string", TypeID::String},
        {"const char*", TypeID::CString},

        // Containers
        {"array", TypeID::Array},
        {"vector", TypeID::Vector},
        {"map", TypeID::Map},
        {"set", TypeID::Set},

        // User-defined / fallback
        {"struct", TypeID::Struct},
        {"enum", TypeID::Enum},
    };

    auto it = typeMap.find(typeName);
    if (it != typeMap.end()) {
        return it->second;
    }
    return TypeID::Unknown; // fallback if not found
}

static std::string GetStringFromTypeID(TypeID type) {
    switch (type) {
        // Invalid / base
        case TypeID::Unknown: return "Unknown";

        // Integer types
        case TypeID::Int8:  return "Int8";
        case TypeID::Int16: return "Int16";
        case TypeID::Int32: return "Int32";
        case TypeID::Int64: return "Int64";

        case TypeID::UInt8:  return "UInt8";
        case TypeID::UInt16: return "UInt16";
        case TypeID::UInt32: return "UInt32";
        case TypeID::UInt64: return "UInt64";

        // Floating point
        case TypeID::Float:  return "Float";
        case TypeID::Double: return "Double";

        // Boolean
        case TypeID::Bool: return "Bool";

        // Vectors
        case TypeID::Vec2: return "Vec2";
        case TypeID::Vec3: return "Vec3";
        case TypeID::Vec4: return "Vec4";

        case TypeID::IVec2: return "IVec2";
        case TypeID::IVec3: return "IVec3";
        case TypeID::IVec4: return "IVec4";

        case TypeID::UVec2: return "UVec2";
        case TypeID::UVec3: return "UVec3";
        case TypeID::UVec4: return "UVec4";

        // Matrices
        case TypeID::Mat2: return "Mat2";
        case TypeID::Mat3: return "Mat3";
        case TypeID::Mat4: return "Mat4";

        // Quaternion
        case TypeID::Quat: return "Quat";

        // Colors
        case TypeID::ColorRGB: return "ColorRGB";
        case TypeID::ColorRGBA: return "ColorRGBA";

        // Strings
        case TypeID::String: return "String";
        case TypeID::CString: return "CString";

        // Containers
        case TypeID::Array: return "Array";
        case TypeID::Vector: return "Vector";
        case TypeID::Map: return "Map";
        case TypeID::Set: return "Set";

        // User-defined / fallback
        case TypeID::Struct: return "Struct";
        case TypeID::Enum: return "Enum";
    }

    return "Unknown"; // fallback, should never hit
}