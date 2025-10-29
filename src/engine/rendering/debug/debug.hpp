#pragma once

#include "engine/rendering/material/material.hpp"

namespace Pulse::Engine::Rendering{

    class DebugShape{
        public:
            DebugShape() = default;
            virtual ~DebugShape();

            int GetIndexCount() { return indexCount; }
            GLuint GetVAO() { return VAO; }

        protected:
            GLuint VAO, VBO, EBO;
            std::vector<Vertex> vertices;
            int indexCount;
    };
 
    class DebugBox : public DebugShape{
        public:
            DebugBox(glm::vec3 halfExtent, COL_RGBA color);
    };

    class DebugSphere : public DebugShape{
        public:
            DebugSphere(float radius, COL_RGBA color);
    };

    class DebugCapsule : public DebugShape{
        public:
            DebugCapsule(float radius, float halfHeight, COL_RGBA color);
    };

    class DebugCylinder : public DebugShape{
        public:
            DebugCylinder(float radius, float halfHeight, COL_RGBA color);
    };
    
}