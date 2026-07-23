#pragma once

#include "engine/rendering/utils.hpp"

#include <memory>

namespace Pulse::Engine::Rendering {

    class Mesh;

    class DebugShape
    {
        public:

            DebugShape() = default;
            virtual ~DebugShape();

            std::shared_ptr<Mesh> m_Mesh;
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