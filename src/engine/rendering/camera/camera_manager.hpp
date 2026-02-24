#pragma once

#include "engine/rendering/utils.hpp"
#include "engine/ecs/components/rendering/camera.hpp"
#include "engine/core/objectID.hpp"

#include <string>
#include <vector>
#include <memory>

namespace Pulse::Engine::Rendering{
    
    using namespace ECS::Components;

    class CameraManager{
        public:
            
            /// @brief Adds a new camera with a name
            void AddCamera(const Core::ObjectID parentID, std::shared_ptr<Camera> camera) {
                cameras[parentID] = camera;

                if (!activeCamera)
                    activeCamera = camera;
            }

            /// @brief Removes a camera by name
            void RemoveCamera(const Core::ObjectID parentID) {
                if (cameras.count(parentID)) {
                    if (cameras[parentID] == activeCamera)
                        activeCamera = nullptr;
                    cameras.erase(parentID);
                }
            }

            /// @brief Update all camera's sizes
            /// @param width The new width (in px)
            /// @param height The new height (in px)
            void UpdateSize(int width, int height) {
                for(std::pair<Core::ObjectID, std::shared_ptr<Camera>> cam : cameras){
                    cam.second->UpdateSize(width, height);
                }
            }

            /// @brief Update the active camera
            void Tick(){
                if(activeCamera != nullptr)
                    activeCamera->UpdateMatrix();
            }

            /// @brief Get a camera by name
            std::shared_ptr<Camera> GetCamera(const Core::ObjectID parentID) {
                if (cameras.count(parentID))
                    return cameras[parentID];
                return nullptr;
            }

            /// @brief Set the active camera
            void SetActiveCamera(const Core::ObjectID parentID) {
                if (cameras.count(parentID))
                    activeCamera = cameras[parentID];
            }

            /// @brief Get the current active camera
            std::shared_ptr<Camera> GetActiveCamera() {
                return activeCamera;
            }

            /// @brief Clear all cameras
            void Clear() {
                cameras.clear();
                activeCamera = nullptr;
            }

        private:

            std::unordered_map<Core::ObjectID, std::shared_ptr<Camera>> cameras;
            std::shared_ptr<Camera> activeCamera;
    };
}