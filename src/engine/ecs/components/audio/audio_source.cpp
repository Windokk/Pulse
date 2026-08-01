#include "audio_source.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

#include "audio_source.reflection.hpp"

namespace Pulse::Engine::ECS::Components{
    
    AudioSource::AudioSource(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
    }

    void AudioSource::Update()
    {
        if(audioID.IsValid() && activated){
            GetEngineContext()->GetAudioManager()->UpdateSound(audioID, parent->transform->GetPosition(), volume);
        }
        else{
            if(!path.full.empty() && volume != -1.0f){
                audioID = GetEngineContext()->GetAudioIDManager()->GenerateNewID();
                GetEngineContext()->GetAudioManager()->CreateSound(audioID, path, parent->transform->GetPosition());

            }
        }
    }

    void AudioSource::Deserialize(const json componentData)
    {
        auto getFloat = [&](const char* key, float defaultValue = 0.0f) -> float
        {
            if (!componentData.contains(key) || !componentData[key].is_number())
                return defaultValue;
            return componentData[key].get<float>();
        };

        auto getString = [&](const char* key, const std::string& defaultValue = "") -> std::optional<std::string>
        {
            if (!componentData.contains(key) || !componentData[key].is_string())
                return std::nullopt;
            return componentData[key].get<std::string>();
        };

        auto volume = getFloat("volume", 1.0f);

        auto pathOpt = getString("path");
        if (!pathOpt)
        {
            DEBUG_ERROR("AudioSource missing or invalid 'path'");
            return;
        }

        SetPath(Filesystem::Path(*pathOpt, false));
        SetVolume(volume);

        bool active = false;
        if (componentData.contains("active") && componentData["active"].is_boolean())
            active = componentData["active"].get<bool>();

        if (active)
            Activate();
        else
            DeActivate();
    }

    ordered_json AudioSource::Serialize()
    {
        ordered_json comp;

        comp["type"] = "audio";

        comp["active"] = activated;

        comp["volume"] = volume;

        comp["path"] = path.full;

        return comp;
    }

    void AudioSource::Destroy()
    {
        RemoveSound();
    }

    std::shared_ptr<Component> AudioSource::Clone() const
    {
        auto cloned = Object::Create<AudioSource>(*this);

        return cloned;
    }

    void AudioSource::SetPath(Filesystem::Path newPath)
    {
        this->path = path;
        Update();
    }

    void AudioSource::SetVolume(float volume)
    {
        this->volume = volume;
        Update();
    }

    void AudioSource::Play()
    {
        if(!activated)
            return;

        GetEngineContext()->GetAudioManager()->PlaySound(this->audioID, this->volume);
    }

    void AudioSource::Pause()
    {
        GetEngineContext()->GetAudioManager()->PauseSound(this->audioID);
    }

    void AudioSource::RemoveSound()
    {
        GetEngineContext()->GetAudioManager()->RemoveSound(this->audioID);
    }

    void AudioSource::OnFieldChanged(const FieldChangedEvent &event)
    {
        if(event.field->name == "volume"){
            SetVolume(volume);
        }
        if(event.field->name == "file"){
            SetPath(file);
        }
    }
}