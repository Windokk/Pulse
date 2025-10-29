#pragma once 

#include "engine/filesystem/filesystem.hpp"

#include "audioID.hpp"

#include <fmod/fmod.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <string>
#include <iostream>

namespace Pulse::Engine::Audio
{
    struct Sound{
        FMOD_SOUND* fmod_sound;
        Filesystem::Path path;
        glm::vec3 pos;
        std::string buffer;
        bool isPlaying = false;
        float initialVolume;
    };
    
    class AudioManager{
        public:
        
        void Init(float masterVolume);
        void Shutdown();
        void Tick();
        void CreateSound(AudioID id, Filesystem::Path path, glm::vec3 pos);
        void RemoveSound(AudioID id);
        void PlaySound(AudioID id, float volume);
        void PauseSound(AudioID id);
        void UpdateSound(AudioID id, glm::vec3 pos, float volume);
        void Update(glm::vec3 listenerPos, glm::vec2 listenerFacingNormalized, float maxDistance);

        private:

        FMOD_SYSTEM *system;
        std::unordered_map<std::string, FMOD_CHANNEL*> channels;
	    FMOD_CREATESOUNDEXINFO exinfo;
        float masterVolume = 100.0f;
    };

}