#pragma once

#include "engine/rendering/utils.hpp"
#include "engine/ecs/components/rendering/camera.hpp"

#include <string>
#include <vector>
#include <memory>

namespace Epoch::Engine::Rendering{
    
    using namespace ECS::Components;

    class CameraManager{
        public:
            
            /// @brief Adds a new camera with a name
            void AddCamera(const std::string& name, std::shared_ptr<Camera> camera) {
                cameras[name] = camera;

                if (!activeCamera)
                    activeCamera = camera;
            }

            /// @brief Removes a camera by name
            void RemoveCamera(const std::string& name) {
                if (cameras.count(name)) {
                    if (cameras[name] == activeCamera)
                        activeCamera = nullptr;
                    cameras.erase(name);
                }
            }

            /// @brief Update all camera's sizes
            /// @param width The new width (in px)
            /// @param height The new height (in px)
            void UpdateSize(int width, int height) {
                for(std::pair<std::string, std::shared_ptr<Camera>> cam : cameras){
                    cam.second->UpdateSize(width, height);
                }
            }

            /// @brief Update the active camera
            void Tick(){
                if(activeCamera != nullptr)
                    activeCamera->UpdateMatrix();
            }

            /// @brief Get a camera by name
            std::shared_ptr<Camera> GetCamera(const std::string& name) {
                if (cameras.count(name))
                    return cameras[name];
                return nullptr;
            }

            /// @brief Set the active camera
            void SetActiveCamera(const std::string& name) {
                if (cameras.count(name))
                    activeCamera = cameras[name];
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

            std::unordered_map<std::string, std::shared_ptr<Camera>> cameras;
            std::shared_ptr<Camera> activeCamera;
    };
}