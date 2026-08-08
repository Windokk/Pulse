#pragma once

#include "engine/audio/audio_manager.hpp"

#include "engine/objects/components/core/component.hpp"

#include "engine/rendering/utils.hpp"

#include "engine/core/attributes.hpp"

namespace Pulse::Engine::Objects::Components
{
    class CLASS() AudioSource : public Component {

        public:
            AudioSource(std::shared_ptr<Actor> parent, uint32_t local_id);

            void SetPath(Filesystem::Path path);
            void SetVolume(float volume);
            void Play();
            void Pause();
            void RemoveSound();
            void Update();

            void Deserialize(const json componentData) override;

            ordered_json Serialize() override;

            void Destroy() override;

            void DeActivate() override
            {
                Component::DeActivate();

                Pause();
            }

            std::shared_ptr<Component> Clone() const override;

            void OnFieldChanged(const FieldChangedEvent& event) override;
            
            DECLARE_DESCRIPTOR(AudioSource)

            FIELD(Editable)
            std::string file;

            FIELD(Editable)
            float volume = -1.0f;

        private:
            Filesystem::Path path;
            Audio::AudioID audioID;
    };
}