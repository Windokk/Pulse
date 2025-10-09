#include "audio_manager.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/levels/level_manager.hpp"

#include "engine/rendering/utils.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/rendering/camera/camera_manager.hpp"

#include <cassert>

#include "engine/time/time_manager.hpp"

namespace Epoch::Engine::Audio
{
    using namespace Filesystem;
    using namespace Rendering;

    FMOD_RESULT F_CALLBACK OnSoundStopped(FMOD_CHANNELCONTROL* chanControl,
                                      FMOD_CHANNELCONTROL_TYPE controlType,
                                      FMOD_CHANNELCONTROL_CALLBACK_TYPE callbackType,
                                      void* commandData1,
                                      void* commandData2)
    {
        if (callbackType == FMOD_CHANNELCONTROL_CALLBACK_END) {
            void* userData = nullptr;
            FMOD_Channel_GetUserData((FMOD_CHANNEL*)chanControl, &userData);

            if (userData) {
                AudioID* id = static_cast<AudioID*>(userData);

                auto* sound = AudioIDManager::GetInstance().GetSoundFromID(*id);
                if (sound) {
                    sound->isPlaying = false;
                }

                delete id;
            }
        }

        return FMOD_OK;
    }

    void AudioManager::Init(float masterVolume)
    {
        AudioManager::masterVolume = masterVolume;

        // Initialize FMOD system
        FMOD_System_Create(&system, FMOD_VERSION);
        FMOD_System_Init(system, 512, FMOD_INIT_NORMAL, 0);
    }

    void AudioManager::CreateSound(AudioID id, Filesystem::Path path, glm::vec3 pos)
    {

        if (path.Exists()) {
            auto sound = new Sound();
            FMOD_CHANNEL* channel = nullptr;

            std::string file = path.ReadFile();             

            const char* buffer = file.data();
            size_t buffer_size = file.size();

            FMOD_CREATESOUNDEXINFO exinfo{};
            exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
            exinfo.length = buffer_size;

            FMOD_RESULT result = FMOD_System_CreateSound(system, buffer, FMOD_2D | FMOD_OPENMEMORY, &exinfo, &sound->fmod_sound);
            if (result != FMOD_OK) {
                DEBUG_ERROR("FMOD error: " + result);
                return;
            }

            sound->path = path;
            sound->pos = pos;
            sound->buffer = file;

            AudioIDManager::GetInstance().AssignID(id, sound);
            channels.emplace(id.GetAsString() + "_channel", channel);
        } else {
            DEBUG_ERROR("Couldn't load sound: " + path.full);
        }
    }

    void AudioManager::RemoveSound(AudioID id)
    {
        Sound* sound = AudioIDManager::GetInstance().GetSoundFromID(id);
        FMOD_Sound_Release(sound->fmod_sound);
        delete sound;
        AudioIDManager::GetInstance().DestroyID(id);
        channels.erase(id.GetAsString()+"_channel");
    }

    void AudioManager::PlaySound(AudioID id, float volume)
    {
        if(!AudioIDManager::GetInstance().GetSoundFromID(id)->isPlaying){
            FMOD_System_PlaySound(system, AudioIDManager::GetInstance().GetSoundFromID(id)->fmod_sound, nullptr, true, &channels.at(id.GetAsString()+"_channel"));
            AudioID* idCopy = new AudioID(id);
            FMOD_Channel_SetUserData(channels.at(id.GetAsString()+"_channel"), idCopy);
            FMOD_Channel_SetCallback(channels.at(id.GetAsString()+"_channel"), OnSoundStopped);
            FMOD_Channel_SetPaused(channels.at(id.GetAsString()+"_channel"), false);
            AudioIDManager::GetInstance().GetSoundFromID(id)->isPlaying = true;
            AudioIDManager::GetInstance().GetSoundFromID(id)->initialVolume = volume/100.0f;
        }
    }

    void AudioManager::PauseSound(AudioID id)
    {
        if(AudioIDManager::GetInstance().GetSoundFromID(id)->isPlaying){
            FMOD_Channel_SetPaused(channels.at(id.GetAsString()+"_channel"), true);
            AudioIDManager::GetInstance().GetSoundFromID(id)->isPlaying = false;
        }
    }

    void AudioManager::UpdateSound(AudioID id, glm::vec3 pos, float volume)
    {
        AudioIDManager::GetInstance().GetSoundFromID(id)->pos = pos;
        AudioIDManager::GetInstance().GetSoundFromID(id)->initialVolume = volume;
    }

    void AudioManager::Update(glm::vec3 listenerPos, glm::vec2 listenerFacingNormalized, float maxDistance)
    {
        if(maxDistance <= 0){
            DEBUG_FATAL("maxDistance can't be >= 0");
        }
        
        for (const auto& pair : AudioIDManager::GetInstance().GetAudioMap()) {
            if(pair.second->isPlaying){
                float pan = glm::sin(glm::orientedAngle(glm::normalize(glm::vec2(pair.second->pos.x - listenerPos.x, pair.second->pos.z - listenerPos.z)), listenerFacingNormalized));
                if (pan < -1.0f) pan = -1.0f;
                if (pan > 1.0f) pan = 1.0f;
                float volume = 1.0f - (glm::distance(listenerPos, pair.second->pos) / maxDistance);
                volume *= masterVolume/100;
                if (volume < 0.0f) volume = 0.0f;
                if (volume > 1.0f) volume = 1.0f;
                
                volume *= pair.second->initialVolume;

                FMOD_Channel_SetPan(channels.at(pair.first.GetAsString()+"_channel"), -1 * pan);
                FMOD_Channel_SetVolume(channels.at(pair.first.GetAsString()+"_channel"), volume);
            }
        }

        FMOD_System_Update(system);
    }

    void AudioManager::Shutdown()
    {
        for (const auto& pair : AudioIDManager::GetInstance().GetAudioMap()) {
            AudioIDManager::GetInstance().DestroyID(pair.first); 
        }

        FMOD_System_Close(system);
        FMOD_System_Release(system);
    }

    void AudioManager::Tick()
    {
        if(int levelCount = Levels::LevelManager::GetInstance().GetLoadedLevelCount() > 0){
            for(int i = 0; i < levelCount; i++){
                for(auto& source : Levels::LevelManager::GetInstance().GetLevelAt(i)->audioSources){
                    source->Update();
                }

                std::shared_ptr<ECS::Components::Camera> cam = CameraManager::GetInstance().GetActiveCamera();

                if(cam == nullptr)
                    return;

                AudioManager::Update(cam->parent->transform->GetPosition(), glm::normalize(glm::vec2(cam->parent->transform->GetForward().x, cam->parent->transform->GetForward().z)), 100.0f);
                
            }
            
        }
    }
}