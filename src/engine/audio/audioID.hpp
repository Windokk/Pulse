#pragma once

#include <string>
#include <atomic>
#include <unordered_set>
#include <map>

namespace Pulse::Engine::Audio
{
    class Sound;

    class AudioID {
        public:
            
            AudioID() : packed(0) {}
            explicit AudioID(int value) : packed(value) {}

            bool IsValid() const { return packed != 0; }
            
            int GetAsInt() const {
                return packed;
            }

            std::string GetAsString() const {
                return std::to_string(GetAsInt());
            }

            
            bool operator==(const AudioID& other) const { return packed == other.packed; }
            bool operator!=(const AudioID& other) const { return !(*this == other); }
            bool operator<(const AudioID& other) const { return packed < other.packed; }

            
            friend class AudioIDBuilder;
    
        private:
            int packed;
    };
    
    class AudioIDBuilder {
        public:
        AudioIDBuilder& WithValue(int val) {
                value = val;
                generated = false;
                return *this;
            }
        
            AudioIDBuilder& Generate() {
                value = GenerateNextID();
                generated = true;
                return *this;
            }
        
            AudioID Build() const {
                return AudioID(value);
            }
        
        private:
            int GenerateNextID() {
                static std::atomic<int> nextId{1};
                return nextId.fetch_add(1);
            }
        
            int value = 0;
            bool generated = false;
    };

    class AudioIDManager {
        public:

            void DestroyID(const AudioID& id) {
                availableIDs.insert(id.GetAsInt());
                AudioIDMap.erase(id);
            }
            
            AudioID GenerateNewID() {
                if (!availableIDs.empty()) {
                    int id = *availableIDs.begin();
                    availableIDs.erase(availableIDs.begin());
                    return AudioID(id);
                }
                return AudioID(AudioIDBuilder().Generate().Build().GetAsInt());
            }

            std::map<AudioID, Sound*> GetAudioMap() { return AudioIDMap; }

            void AssignID(AudioID id, Sound* obj){
                AudioIDMap[id] = obj;
            }
        
            Sound* GetSoundFromID(AudioID id){
                auto it = AudioIDMap.find(id);
                if (it != AudioIDMap.end()) {
                    return it->second;
                }
                return nullptr;
            }

        private:
            std::map<AudioID, Sound*> AudioIDMap;
            std::unordered_set<int> availableIDs;
    };
}
