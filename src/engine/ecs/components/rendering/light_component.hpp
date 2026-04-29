#pragma once

#include "engine/ecs/components/core/component.hpp"

#include "engine/rendering/renderer/renderer.hpp"

#include "engine/core/attributes.hpp"

#include "engine/rendering/lighting/light_manager.hpp"

namespace Pulse::Engine::ECS::Components
{
    
    class CLASS() Light : public Component{
        public:
            Light(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

            void SetType(Rendering::LightType type);
            void SetIntensity(float intensity);
            void SetPosition(glm::vec3 postion);
            void SetLightIndex(int index);
            void SetCastShadow(bool castShadows);
            void SetColor(glm::vec3 color);

            // Directional / Spot light
            void SetDirection(glm::vec3 direction);

            // Spot light
            void SetOuterCutoff(float cutoff);
            void SetInnerCuttoff(float cutoff);

            // Point light / Spot light
            void SetRadius(float radius);
            
            int GetLightIndex() { return lightIndex; }

            void Deserialize(json componentData) override;

            ordered_json Serialize() override;
            
            Rendering::LightData GetData();
            
            void Destroy() override;

            std::shared_ptr<Component> Clone() const override;

            void OnFieldChanged(const FieldChangedEvent &event) override;

            void UpdateEditorValues();

            FIELD(Editable)
            Rendering::LightType type;

            FIELD(Editable)
            float intensity;

            FIELD(Editable)
            float radius;

            FIELD(Editable)
            COL_RGB color;

            FIELD(Editable)
            float outerCutoff;

            FIELD(Editable)
            float innerCutoff;

            FIELD(Editable)
            bool castShadows;

        private:
            int lightIndex = -1;
            std::shared_ptr<Rendering::LightData> lightData;

            DECLARE_DESCRIPTOR(Light)
    };
}