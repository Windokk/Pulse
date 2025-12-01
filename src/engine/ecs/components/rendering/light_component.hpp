#pragma once

#include "engine/ecs/components/core/component.hpp"

#include "engine/rendering/renderer/renderer.hpp"

namespace Pulse::Engine::ECS::Components
{
    class Light : public Component{
        public:
            Light(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

            void SetType(Rendering::LightType type);
            void SetIntensity(float intensity);
            void SetPosition(glm::vec3 postion);
            void SetDirection(glm::vec3 direction);
            void SetRadius(float radius);
            void SetColor(glm::vec3 color);
            void SetOuterCutoff(float cutoff);
            void SetInnerCuttof(float cutoff);
            void SetLightIndex(int index);
            void SetCastShadow(bool castShadows);

            void Deserialize(json componentData, json levelData) override;

            json Serialize() override;
            
            Rendering::LightData GetData();
            
            void Destroy() override;

            std::shared_ptr<Component> Clone() const override;

        private:
            int lightIndex = -1;
            std::shared_ptr<Rendering::LightData> lightData;
    };
}