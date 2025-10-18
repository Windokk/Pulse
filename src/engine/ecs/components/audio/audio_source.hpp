#pragma once

#include "engine/audio/audio_manager.hpp"

#include "engine/ecs/components/core/component.hpp"

#include "engine/rendering/utils.hpp"

namespace Epoch::Engine::ECS::Components
{

    class AudioSource : public Component {
        private:
            Filesystem::Path path;
            Audio::AudioID audioID;
            float volume = -1.0f;


        public:
            AudioSource(Objects::Actor *parent, uint32_t local_id);

            void SetPath(Filesystem::Path path);
            void SetVolume(float volume);
            void Play();
            void Pause();
            void RemoveSound();
            void Update();

            void DeActivate() override
            {
                Component::DeActivate();

                Pause();
            }
    };
}