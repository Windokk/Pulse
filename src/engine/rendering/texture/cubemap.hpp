#pragma once

#include "engine/ecs/components/core/transform.hpp"

#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>
#include <string>
#include <memory>

namespace Pulse::Engine::Filesystem{

    class Path;
}

namespace Pulse::Engine::Rendering {

    struct DrawCommand;

    struct CubeVertex{
        glm::vec3 pos;
    };

    struct CubemapInfos{
        int width, height;       
        int nrChannels;
        std::shared_ptr<Filesystem::Path> folder;
    };

    class Shader;

    class Cubemap{
        public:
            Cubemap(Filesystem::Path filepath);
            void Draw(std::shared_ptr<Shader> shader, glm::mat4 view, glm::mat4 projection);
            void Init(Filesystem::Path filepath);
            void Bind();
            void UnBind();
            void Cleanup();
            void GenerateMesh();
            unsigned int GetID() { return ID; }
            CubemapInfos* GetInfos() { return &infos; }

        private:
            unsigned int ID;
            CubemapInfos infos;
            unsigned int VAO, VBO, EBO;
    };
}