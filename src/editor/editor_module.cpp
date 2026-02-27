#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/rendering/ui/text.hpp"
#include "engine/rendering/opengl/opengl.hpp"
#include "editor/commands/command_stack.hpp"

#include "editor/core/platform/glfw/glfw_platform.hpp"

#include "editor/gui/main_window.hpp"

using namespace Pulse;
using namespace Pulse::Engine;
using namespace Pulse::Engine::Core;
using namespace Pulse::Engine::Rendering;
using namespace Pulse::Engine::ECS::Components;
using namespace Pulse::Engine::ECS::Objects;
using namespace Pulse::Editor::Commands;

#if defined(__WIN32__) || defined(__WIN64__)
#   define API_EXPORT __declspec(dllexport)
#else
#   define API_EXPORT __attribute__((visibility("default")))
#endif

extern "C" API_EXPORT void InitializeSingletons(Core::EngineInstance* engine) {
    SetEngine(engine);
}

extern "C" API_EXPORT void EditorStart(){
    CommandStack::Get();
}

extern "C" API_EXPORT void EditorTick(){
    Core::GetEngine().GetWindow()->ProcessInputs();
}

extern "C" API_EXPORT void EditorCleanup(){

}

extern "C" API_EXPORT Platform::IPlatform* CreatePlatform(int argc, char** argv) {
    return new Pulse::Editor::Core::GLFWPlatform();
}
