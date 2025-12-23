#pragma once

#include "engine/rendering/opengl/opengl.hpp"

#include "engine/ecs/components/misc/transform.hpp"

//3D
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
    glm::vec2 texCoord;
    glm::vec3 tangent;
    
    bool operator==(const Vertex& other) const {
        return position == other.position &&
               normal == other.normal &&
               color == other.color &&
               texCoord == other.texCoord && tangent == other.tangent;
    }
};

namespace std {
    template <>
    struct hash<Vertex> {
        size_t operator()(const Vertex& v) const {
            size_t h1 = hash<float>()(v.position.x) ^ (hash<float>()(v.position.y) << 1) ^ (hash<float>()(v.position.z) << 2);
            size_t h2 = hash<float>()(v.normal.x) ^ (hash<float>()(v.normal.y) << 1) ^ (hash<float>()(v.normal.z) << 2);
            size_t h3 = hash<float>()(v.color.r) ^ (hash<float>()(v.color.g) << 1) ^ (hash<float>()(v.color.b) << 2) ^ (hash<float>()(v.color.a) << 3);
            size_t h4 = hash<float>()(v.texCoord.x) ^ (hash<float>()(v.texCoord.y) << 1);
            size_t h5 = hash<float>()(v.tangent.x) ^ (hash<float>()(v.tangent.y) << 1) ^ (hash<float>()(v.tangent.z) << 2);
            return h1 ^ h2 ^ h3 ^ h4 ^ h5;
        }
    };
}


//Colors
struct COL_RGB{
    COL_RGB() = default;
    COL_RGB(float r, float g, float b){
        components[0] = r;
        components[1] = g;
        components[2] = b;
    };

    COL_RGB(float scalar){
        components[0] = scalar;
        components[1] = scalar;
        components[2] = scalar;
    };

    COL_RGB(glm::vec3 col){
        components[0] = col.x;
        components[1] = col.y;
        components[2] = col.z;
    }

    COL_RGB& operator=(const COL_RGB& other) = default;

    COL_RGB operator+(const COL_RGB& other) const {
        return COL_RGB(
            components[0] + other.components[0],
            components[1] + other.components[1],
            components[2] + other.components[2]
        );
    }

    COL_RGB operator-(const COL_RGB& other) const {
        return COL_RGB(
            components[0] - other.components[0],
            components[1] - other.components[1],
            components[2] - other.components[2]
        );
    }

    COL_RGB operator*(const COL_RGB& other) const {
        return COL_RGB(
            components[0] * other.components[0],
            components[1] * other.components[1],
            components[2] * other.components[2]
        );
    }

    COL_RGB operator/(const COL_RGB& other) const {
        return COL_RGB(
            components[0] / other.components[0],
            components[1] / other.components[1],
            components[2] / other.components[2]
        );
    }

    COL_RGB operator+(float scalar) const {
        return COL_RGB(
            components[0] + scalar,
            components[1] + scalar,
            components[2] + scalar
        );
    }

    COL_RGB operator-(float scalar) const {
        return COL_RGB(
            components[0] - scalar,
            components[1] - scalar,
            components[2] - scalar
        );
    }

    COL_RGB operator*(float scalar) const {
        return COL_RGB(
            components[0] * scalar,
            components[1] * scalar,
            components[2] * scalar
        );
    }

    COL_RGB operator/(float scalar) const {
        return COL_RGB(
            components[0] / scalar,
            components[1] / scalar,
            components[2] / scalar
        );
    }

    COL_RGB& operator+=(const COL_RGB& other) {
        components[0] += other.components[0];
        components[1] += other.components[1];
        components[2] += other.components[2];
        return *this;
    }

    COL_RGB& operator-=(const COL_RGB& other) {
        components[0] -= other.components[0];
        components[1] -= other.components[1];
        components[2] -= other.components[2];
        return *this;
    }

    COL_RGB& operator*=(const COL_RGB& other) {
        components[0] *= other.components[0];
        components[1] *= other.components[1];
        components[2] *= other.components[2];
        return *this;
    }

    COL_RGB& operator/=(const COL_RGB& other) {
        components[0] /= other.components[0];
        components[1] /= other.components[1];
        components[2] /= other.components[2];
        return *this;
    }

    COL_RGB& operator+=(float scalar) {
        components[0] += scalar;
        components[1] += scalar;
        components[2] += scalar;
        return *this;
    }

    COL_RGB& operator-=(float scalar) {
        components[0] -= scalar;
        components[1] -= scalar;
        components[2] -= scalar;
        return *this;
    }

    COL_RGB& operator*=(float scalar) {
        components[0] *= scalar;
        components[1] *= scalar;
        components[2] *= scalar;
        return *this;
    }

    COL_RGB& operator/=(float scalar) {
        components[0] /= scalar;
        components[1] /= scalar;
        components[2] /= scalar;
        return *this;
    }

    operator glm::vec3() const {
        return glm::vec3(components[0], components[1], components[2]);
    }

    float r() const { return components[0]; }
    float g() const { return components[1]; }
    float b() const { return components[2]; }

    private:

        float components[3] = {0.0f, 0.0f ,0.0f};
};
struct COL_RGBA{    
    COL_RGBA() = default;

    COL_RGBA(glm::vec4 col){
        components[0] = col.x;
        components[1] = col.y;
        components[2] = col.z;
        components[3] = col.w;
    }

    COL_RGBA(glm::vec3 col){
        components[0] = col.x;
        components[1] = col.y;
        components[2] = col.z;
        components[3] = 1.0f;
    }

    COL_RGBA(float r, float g, float b, float a){
        components[0] = r;
        components[1] = g;
        components[2] = b;
        components[3] = a;
    };

    COL_RGBA(float r, float g, float b){
        components[0] = r;
        components[1] = g;
        components[2] = b;
        components[3] = 1.0f;
    };

    COL_RGBA(float scalar){
        components[0] = scalar;
        components[1] = scalar;
        components[2] = scalar;
        components[3] = scalar;
    };

    COL_RGBA& operator=(const COL_RGBA& other) = default;

    COL_RGBA operator+(const COL_RGBA& other) const {
        return COL_RGBA(
            components[0] + other.components[0],
            components[1] + other.components[1],
            components[2] + other.components[2],
            components[3] + other.components[3]
        );
    }

    COL_RGBA operator-(const COL_RGBA& other) const {
        return COL_RGBA(
            components[0] - other.components[0],
            components[1] - other.components[1],
            components[2] - other.components[2],
            components[3] - other.components[3]
        );
    }

    COL_RGBA operator*(const COL_RGBA& other) const {
        return COL_RGBA(
            components[0] * other.components[0],
            components[1] * other.components[1],
            components[2] * other.components[2],
            components[3] * other.components[3]
        ); 
    }

    COL_RGBA operator/(const COL_RGBA& other) const {
        return COL_RGBA(
            components[0] / other.components[0],
            components[1] / other.components[1],
            components[2] / other.components[2],
            components[3] / other.components[3]
        );
    }

    COL_RGBA operator+(float scalar) const {
        return COL_RGBA(
            components[0] + scalar,
            components[1] + scalar,
            components[2] + scalar,
            components[3] + scalar
        );
    }

    COL_RGBA operator-(float scalar) const {
        return COL_RGBA(
            components[0] - scalar,
            components[1] - scalar,
            components[2] - scalar,
            components[2] - scalar
        );
    }

    COL_RGBA operator*(float scalar) const {
        return COL_RGBA(
            components[0] * scalar,
            components[1] * scalar,
            components[2] * scalar,
            components[3] * scalar
        );
    }

    COL_RGBA operator/(float scalar) const {
        return COL_RGBA(
            components[0] / scalar,
            components[1] / scalar,
            components[2] / scalar,
            components[2] / scalar
        );
    }

    COL_RGBA& operator+=(const COL_RGBA& other) {
        components[0] += other.components[0];
        components[1] += other.components[1];
        components[2] += other.components[2];
        components[3] += other.components[3];
        return *this;
    }

    COL_RGBA& operator-=(const COL_RGBA& other) {
        components[0] -= other.components[0];
        components[1] -= other.components[1];
        components[2] -= other.components[2];
        components[3] -= other.components[3];
        return *this;
    }

    COL_RGBA& operator*=(const COL_RGBA& other) {
        components[0] *= other.components[0];
        components[1] *= other.components[1];
        components[2] *= other.components[2];
        components[3] *= other.components[3];
        return *this;
    }

    COL_RGBA& operator/=(const COL_RGBA& other) {
        components[0] /= other.components[0];
        components[1] /= other.components[1];
        components[2] /= other.components[2];
        components[3] /= other.components[3];
        return *this;
    }

    COL_RGBA& operator+=(float scalar) {
        components[0] += scalar;
        components[1] += scalar;
        components[2] += scalar;
        components[3] += scalar;
        return *this;
    }

    COL_RGBA& operator-=(float scalar) {
        components[0] -= scalar;
        components[1] -= scalar;
        components[2] -= scalar;
        components[3] -= scalar;
        return *this;
    }

    COL_RGBA& operator*=(float scalar) {
        components[0] *= scalar;
        components[1] *= scalar;
        components[2] *= scalar;
        components[3] *= scalar;
        return *this;
    }

    COL_RGBA& operator/=(float scalar) {
        components[0] /= scalar;
        components[1] /= scalar;
        components[2] /= scalar;
        components[3] /= scalar;
        return *this;
    }

    operator glm::vec4() const {
        return glm::vec4(components[0], components[1], components[2], components[3]);
    }

    operator glm::vec3() const {
        return glm::vec3(components[0], components[1], components[2]);
    }

    operator COL_RGB() const {
        return COL_RGB(components[0], components[1], components[2]);
    }

    COL_RGB rgb(){
        return COL_RGB(components[0], components[1], components[2]);
    }

    float& r() { return components[0]; }
    float& g() { return components[1]; }
    float& b() { return components[2]; }
    float& a() { return components[3]; }

    private:

        float components[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

