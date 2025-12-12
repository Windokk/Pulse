#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/rendering/ui/text.hpp"
#include "editor/core/platform/qt/qt_platform.hpp"
#include "engine/rendering/opengl/opengl.hpp"

#include <QApplication>

#include "editor/gui/main_win.hpp"

using namespace Pulse;
using namespace Pulse::Engine;
using namespace Pulse::Engine::Core;
using namespace Pulse::Engine::Rendering;
using namespace Pulse::Engine::ECS::Components;
using namespace Pulse::Engine::ECS::Objects;

static QApplication* s_app = nullptr;

#if defined(__WIN32__) || defined(__WIN64__)
#   define API_EXPORT __declspec(dllexport)
#else
#   define API_EXPORT __attribute__((visibility("default")))
#endif

extern "C" API_EXPORT void InitializeSingletons(Core::EngineInstance* engine) {
    SetEngine(engine);
}

extern "C" API_EXPORT void EditorStart(){
    
}

extern "C" API_EXPORT void EditorTick(){
    Core::GetEngine().GetWindow()->ProcessInputs();
}

extern "C" API_EXPORT void EditorCleanup(){

}

static int qt_argc = 0;
static char** qt_argv = nullptr;

extern "C" API_EXPORT Platform::IPlatform* CreatePlatform(int argc, char** argv) {
    if (!s_app){
        qt_argc = argc;

        // Deep copy argv
        qt_argv = new char*[argc];
        for(int i=0;i<argc;++i){
            qt_argv[i] = strdup(argv[i]);
        }

        QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
        s_app = new QApplication(qt_argc, qt_argv);
        s_app->setPalette(Editor::createDarkPalette());
    }
    
    return new Editor::Core::Platform::QTPlatform();
}
