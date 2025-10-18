#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/rendering/ui/text.hpp"
#include "editor/core/platform/qt/qt_platform.hpp"
#include "engine/rendering/opengl/opengl.hpp"

#include <QApplication>

#include "editor/gui/main_win.hpp"

using namespace Epoch;
using namespace Epoch::Engine;
using namespace Epoch::Engine::Core;
using namespace Epoch::Engine::Rendering;
using namespace Epoch::Engine::ECS::Components;
using namespace Epoch::Engine::ECS::Objects;

static QApplication* s_app = nullptr;

extern "C" __declspec(dllexport) void InitializeSingletons(Core::EngineInstance* engine) {
    SetEngine(engine);
}

extern "C" __declspec(dllexport) void EditorStart(){
    
}

extern "C" __declspec(dllexport) void EditorTick(){

}

extern "C" __declspec(dllexport) void EditorCleanup(){

}

extern "C" __declspec(dllexport) Platform::IPlatform* CreatePlatform(int argc, char** argv) {
    if (!s_app){
        QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
        s_app = new QApplication(argc, argv);
        s_app->setPalette(Editor::createDarkPalette());
    }
    
    return new Editor::Core::Platform::QTPlatform();

}