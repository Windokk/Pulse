#include "light_component.hpp"

#include "engine/rendering/renderer/renderer.hpp"
#include "engine/ecs/components/misc/transform.hpp"
#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

#include "light_component.reflection.hpp"

#include "glm/ext.hpp"

using namespace Pulse::Engine::Core;

namespace Pulse::Engine::ECS::Components{

    void Light::UpdateEditorValues(){
        type = (Rendering::LightType)lightData->type;
        radius = lightData->radius;
        intensity = lightData->intensity;
        outerCutoff = glm::degrees(glm::acos(lightData->outerCutoff));
        innerCutoff = glm::degrees(glm::acos(lightData->innerCutoff));
        color = lightData->color;
        castShadows = lightData->castShadow;
    }

    Light::Light(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
        lightData = std::make_shared<Rendering::LightData>();

        std::shared_ptr<Transform> tr = parent->transform;

        lightData->position = glm::vec3(tr->GetPosition().x, tr->GetPosition().y, tr->GetPosition().z);
        lightData->direction = tr->GetForward();
        
        UpdateEditorValues();
    }

    /// @brief Set the light's type
    /// @param type The new type (Directional, Spot, Point...)
    void Light::SetType(Rendering::LightType type)
    {
        if(!activated)
            return;

        lightData->type = (int)type;

        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);

        UpdateEditorValues();
    }

    /// @brief Set the light's intensity
    /// @param intensity The new intensity
    void Light::SetIntensity(float intensity)
    {
        if(!activated)
            return;

        lightData->intensity = intensity;

        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);
        
        UpdateEditorValues();
    }

    /// @brief Set the light's position in the world
    /// @param postion The new position (in world units)
    void Light::SetPosition(glm::vec3 postion)
    {
        if(!activated)
            return;

        lightData->position = postion;

        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);
    }

    /// @brief Set the light's direction (Only for spot and directionnal lights)
    /// @param direction The new direction
    void Light::SetDirection(glm::vec3 direction)
    {
        if(!activated)
            return;

        lightData->direction = glm::normalize(direction);
            
        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);
    }
    
    /// @brief Set the radius of the light (Only for spot and point lights)
    /// @param radius The new radius (in world units)
    void Light::SetRadius(float radius)
    {
        if(!activated)
            return;

        lightData->radius = radius;
        
        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);

        UpdateEditorValues();
    }

    /// @brief Sets the color of the light
    /// @param color The new color
    void Light::SetColor(glm::vec3 color)
    {
        if(!activated)
            return;

        lightData->color = color;
        
        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);

        UpdateEditorValues();
    }

    /// @brief Set the outer cuttof (Only for spot lights)
    /// @param cutoff The new cutoff, in degrees
    void Light::SetOuterCutoff(float cutoff)
    {
        if(!activated)
            return;

        lightData->outerCutoff = glm::cos(glm::radians(cutoff));
        
        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);

        UpdateEditorValues();
    }

    /// @brief Set the inner cuttof (Only for spot lights)
    /// @param cutoff The new cutoff, in degrees
    void Light::SetInnerCuttof(float cutoff)
    {
        if(!activated)
            return;

        lightData->innerCutoff = glm::cos(glm::radians(cutoff));
        
        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);

        UpdateEditorValues();
    }

    /// @brief Set the light's index in the scene
    /// @param index This new light's index
    void Light::SetLightIndex(int index)
    {
        if(lightIndex != -1 || !activated)
            return;
        
        if(parent && parent->level && parent->level->IsLoaded()){
            Core::GetEngine().GetRenderer()->GetLightManager()->AddLight(index, lightData);
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(index);
            lightIndex = index;
        }
    }
    
    /// @brief Set wether this light should cast shadows
    /// @param castShadows true : cast shadows, false : doesn't cast shadows
    void Light::SetCastShadow(bool castShadows)
    {
        if(!activated)
            return;

        lightData->castShadow = castShadows;
        
        if(parent && parent->level && parent->level->IsLoaded())
            Core::GetEngine().GetRenderer()->GetLightManager()->Update(lightIndex);
            
        UpdateEditorValues();
    }

    void Light::Deserialize(json componentData)
    {
        if(componentData["light_type"] == "directional"){
            SetType(Rendering::LightType::Directional);
        }
        else if(componentData["light_type"] == "point"){
            SetType(Rendering::LightType::Point);
        }
        else if(componentData["light_type"] == "spot"){
            SetType(Rendering::LightType::Spot);
        }
        else{
            DEBUG_ERROR("Light type not recognized : " + (std::string)componentData["light_type"]);
        }
        
        SetPosition(parent->transform ? parent->transform->GetPosition() : glm::vec3(0.0f));
        SetDirection(parent->transform ? parent->transform->GetForward() : glm::vec3(0.0f, -1.0f, 0.0f));
        SetIntensity(componentData["intensity"]);
        SetRadius(componentData["radius"]);
        SetColor(COL_RGB(componentData["color"]["r"], componentData["color"]["g"], componentData["color"]["b"]));
        SetInnerCuttof(componentData["innerCutoff"]);
        SetOuterCutoff(componentData["outerCutoff"]);
        SetCastShadow(componentData["castShadow"]);
        
        if(componentData.contains("active") && componentData["active"].get<bool>())
            Activate();
        else
            DeActivate();
    }

    ordered_json Light::Serialize()
    {
        ordered_json comp;

        comp["type"] = "light";

        comp["active"] = activated;

        switch(lightData->type){
            case (int)Rendering::LightType::Directional:{
                comp["light_type"] = "directional";
                break;
            }
            case (int)Rendering::LightType::Spot:{
                comp["light_type"] = "spot";
                break;
            }
            case (int)Rendering::LightType::Point:{
                comp["light_type"] = "point";
                break;
            }
        }

        comp["intensity"] = lightData->intensity;
        comp["radius"] = lightData->radius;
        comp["color"]["r"] = lightData->color.r;
        comp["color"]["g"] = lightData->color.g;
        comp["color"]["b"] = lightData->color.b;
        comp["innerCutoff"] = lightData->innerCutoff;
        comp["outerCutoff"] = lightData->outerCutoff;
        comp["castShadow"] = lightData->castShadow;

        return comp;
    }

    /// @brief Getter for this light component's data
    /// @return A copy of this light component's data
    Rendering::LightData Light::GetData()
    {
        return *lightData.get();
    }

    void Light::Destroy()
    {
        Core::GetEngine().GetRenderer()->GetLightManager()->RemoveLight(lightIndex);
    }

    std::shared_ptr<Component> Light::Clone() const
    {
        auto cloned = Object::Create<Light>(*this);

        return cloned;
    }

    void Light::OnFieldChanged(const FieldChangedEvent &event)
    {
        if(event.field->name == "type")
        {
            SetType(type);
        }
        if(event.field->name == "intensity"){
            SetIntensity(intensity);
        }
        if(event.field->name == "radius"){
            SetRadius(radius);
        }
        if(event.field->name == "color")
        {
            SetColor(glm::vec3(color));
        }
        if(event.field->name == "outerCutoff")
        {
            SetOuterCutoff(outerCutoff);
        }
        if(event.field->name == "innerCutoff")
        {
            SetInnerCuttof(innerCutoff);
        }
        if(event.field->name == "castShadows")
        {
            SetCastShadow(castShadows);
        }
    }
}