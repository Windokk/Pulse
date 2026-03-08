#pragma once

#include "engine/rendering/utils.hpp"

namespace Pulse::Engine::Rendering {

    class DebugShape
    {
        public:

            DebugShape() = default;
            virtual ~DebugShape() = default;

            const std::vector<Vertex>& GetVertices() const { return vertices; }
            const std::vector<uint32_t>& GetIndices() const { return indices; }

            int GetIndexCount() const { return indices.size(); }

        protected:

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
    };

    class DebugBox : public DebugShape
    {
        public:

            DebugBox(glm::vec3 halfExtent, COL_RGBA color)
            {
                GenerateBox(halfExtent, color);
            }

        private:

            void GenerateBox(glm::vec3 halfExtent, COL_RGBA color);
    };

    class DebugSphere : public DebugShape
    {
        public:

            DebugSphere(float radius, COL_RGBA color)
            {
                GenerateSphere(radius, color);
            }

        private:

            void GenerateSphere(float radius, COL_RGBA color);
    };

    class DebugCapsule : public DebugShape
    {
        public:

            DebugCapsule(float radius, float halfHeight, COL_RGBA color)
            {
                GenerateCapsule(radius, halfHeight, color);
            }
        private:
            void GenerateCapsule(float radius, float halfHeight, COL_RGBA color);
    };

    class DebugCylinder : public DebugShape
    {
        public:

            DebugCylinder(float radius, float halfHeight, COL_RGBA color)
            {
                GenerateCylinder(radius, halfHeight, color);
            }

        private:
            void GenerateCylinder(float radius, float halfHeight, COL_RGBA color);
    };
}