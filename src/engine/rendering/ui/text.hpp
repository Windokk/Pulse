#pragma once

#include "font.hpp"

#include "engine/rendering/utils.hpp"
#include "engine/rendering/shader/shader.hpp"

#include "engine/ecs/components/core/transform.hpp"

#include <string>

namespace Pulse::Engine::Rendering::UI
{
    using namespace ECS::Components;

    class Text{
        std::string text;
        glm::vec4 color;
        std::shared_ptr<Font> font;
        Transform transform;

        public:
        Text(std::shared_ptr<Font> font, std::string text, glm::vec4 color, Transform transform);

        void Draw(std::shared_ptr<Shader> shader, glm::mat4 projection, glm::mat4 view);

        void SetText(std::string newText) { text = newText;}

    };
}