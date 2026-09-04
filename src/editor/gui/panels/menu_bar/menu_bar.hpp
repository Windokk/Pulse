#pragma once

#include "engine/objects/actors/actor.hpp"

namespace Pulse::Editor::Core{

    class EditorMainWindow;
}

namespace Pulse::Editor::GUI{

    class MenuBar
    {
        public:
            void Draw();

            void SetParentWindow(Core::EditorMainWindow* parent);

        private:
            void DrawFileMenu();
            void DrawEditMenu();
            void DrawSelectMenu();
            void DrawToolsMenu();
            void DrawWindowMenu();
            void DrawHelpMenu();

            void DrawSaveAsPopup();
            void DrawRenamePopup();
            void DrawAboutPopup();

            Core::EditorMainWindow* parent = nullptr;

            // Copy/Cut/Paste
            std::shared_ptr<Engine::Objects::Actor> clipboardActor = nullptr;

            bool openSaveAsPopup = false;
            char saveAsNameBuffer[256] = "NewLevel";

            bool openRenamingPopup = false;
            char renameBuffer[256] = "";

            bool openAboutPopup = false;
    };
}
