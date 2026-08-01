#pragma once

#include <gtest/gtest.h>

#include "engine/core/engine.hpp"
#include "engine/core/objectID.hpp"
#include "engine/events/event_system.hpp"

// Minimal IEngineContext usable from tests without a real window/GL/audio/physics
// bootstrap. Only ObjectIDManager and EventDispatcher are backed by real instances,
// since those are the only subsystems ECS object/component construction depends on
// when no Level is attached. Every other accessor aborts the test loudly instead of
// silently returning nullptr, so a test that reaches into an unsupported subsystem
// fails fast with a clear message rather than crashing on a null deref.
namespace Pulse::Tests {

    class TestEngineContext : public Pulse::Engine::Core::IEngineContext {
        public:
            Pulse::Engine::Core::ObjectIDManager* GetObjectIDManager() const override {
                return const_cast<Pulse::Engine::Core::ObjectIDManager*>(&objectIDManager);
            }

            Pulse::Engine::Events::EventDispatcher* GetEventDispatcher() const override {
                return const_cast<Pulse::Engine::Events::EventDispatcher*>(&eventDispatcher);
            }

            bool ShouldEnd() override { Unsupported("ShouldEnd"); return true; }
            void Destroy() override { Unsupported("Destroy"); }
            bool Run() override { Unsupported("Run"); return false; }
            void Init(Pulse::Engine::Core::EngineCreationSettings) override { Unsupported("Init"); }
            void InitSystems() override { Unsupported("InitSystems"); }

            void SetSettings(const Pulse::Engine::Core::EngineCreationSettings&) override { Unsupported("SetSettings"); }
            Pulse::Engine::Core::EngineCreationSettings GetSettings() const override {
                Unsupported("GetSettings");
                return {};
            }

            Pulse::Engine::Core::Platform::IWindow* GetWindow() const override { Unsupported("GetWindow"); return nullptr; }
            Pulse::Engine::Core::Platform::IInput* GetInputManager() const override { Unsupported("GetInputManager"); return nullptr; }

            Pulse::Engine::Rendering::Renderer* GetRenderer() const override { Unsupported("GetRenderer"); return nullptr; }
            Pulse::Engine::Rendering::CameraManager* GetCameraManager() const override { Unsupported("GetCameraManager"); return nullptr; }
            Pulse::Engine::Core::Resources::ResourcesManager* GetResourcesManager() const override { Unsupported("GetResourcesManager"); return nullptr; }
            Pulse::Engine::Filesystem::FileManager* GetFileManager() const override { Unsupported("GetFileManager"); return nullptr; }
            Pulse::Engine::Filesystem::AssetIDManager* GetAssetIDManager() const override { Unsupported("GetAssetIDManager"); return nullptr; }
            Pulse::Engine::Physics::PhysicsManager* GetPhysicsManager() const override { Unsupported("GetPhysicsManager"); return nullptr; }
            Pulse::Engine::Levels::LevelManager* GetLevelManager() const override { Unsupported("GetLevelManager"); return nullptr; }
            Pulse::Engine::Audio::AudioManager* GetAudioManager() const override { Unsupported("GetAudioManager"); return nullptr; }
            Pulse::Engine::Audio::AudioIDManager* GetAudioIDManager() const override { Unsupported("GetAudioIDManager"); return nullptr; }
            Pulse::Engine::Time::TimeManager* GetTimeManager() const override { Unsupported("GetTimeManager"); return nullptr; }
            Pulse::Engine::Projects::BuildSettings* GetBuildSettings() const override { Unsupported("GetBuildSettings"); return nullptr; }
            std::shared_ptr<Pulse::Engine::Projects::Project> GetCurrentProject() const override { Unsupported("GetCurrentProject"); return nullptr; }
            Pulse::Engine::Debugging::Profiler* GetProfiler() const override { Unsupported("GetProfiler"); return nullptr; }

            bool IsInPlayMode() const override { return false; }
            void SetPlayMode(bool) override { Unsupported("SetPlayMode"); }

        private:
            static void Unsupported(const char* method) {
                ADD_FAILURE() << "TestEngineContext::" << method
                              << " was called but is not backed by a real subsystem. "
                                 "This test reaches beyond ObjectIDManager/EventDispatcher; "
                                 "either scope the test to those, or extend TestEngineContext.";
            }

            mutable Pulse::Engine::Core::ObjectIDManager objectIDManager;
            mutable Pulse::Engine::Events::EventDispatcher eventDispatcher;
    };

}
