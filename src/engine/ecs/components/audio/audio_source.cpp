#include "audio_source.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::ECS::Components{
    
    AudioSource::AudioSource(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
    }

    void AudioSource::Update()
    {
        if(audioID.IsValid() && activated){
            Core::GetEngine().GetAudioManager()->UpdateSound(audioID, parent->transform->GetPosition(), volume);
        }
        else{
            if(!path.full.empty() && volume != -1.0f){
                audioID = Core::GetEngine().GetAudioIDManager()->GenerateNewID();
                Core::GetEngine().GetAudioManager()->CreateSound(audioID, path, parent->transform->GetPosition());

            }
        }
    }

    void AudioSource::Deserialize(json componentData, json levelData)
    {
        float volume = componentData["volume"];
        std::string path = componentData["path"];

        SetPath(Filesystem::Path(path, false));

        SetVolume(volume);

        if(componentData.contains("active") && componentData["active"].get<bool>())
            Activate();
        else
            DeActivate();
    }

    void AudioSource::Destroy()
    {
        RemoveSound();
    }

    std::shared_ptr<Component> AudioSource::Clone() const
    {
        auto cloned = std::make_shared<AudioSource>(*this);

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

        Core::GetEngine().GetAudioManager()->PlaySound(this->audioID, this->volume);
    }

    void AudioSource::Pause()
    {
        Core::GetEngine().GetAudioManager()->PauseSound(this->audioID);
    }

    void AudioSource::RemoveSound()
    {
        Core::GetEngine().GetAudioManager()->RemoveSound(this->audioID);
    }
}