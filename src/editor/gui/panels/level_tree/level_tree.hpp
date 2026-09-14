#pragma once

#include "engine/levels/level.hpp"
#include "engine/objects/actors/actor.hpp"

#include <typeinfo>

namespace Pulse::Editor::Core{
    
    class EditorMainWindow;
}

namespace Pulse::Editor::GUI{


    class LevelTree
    {
        public:
            void Draw();

            void SetParentWindow(Core::EditorMainWindow* parent);
            void SetSelection(std::shared_ptr<Engine::Objects::Actor> actor);

        private:
            void DrawActorNode(std::shared_ptr<Engine::Objects::Actor> actor);

            // Detaches `actor` from wherever it currently sits (another actor's children, or the
            // level's root actor list) and reattaches it under `newParent` - or, if `newParent` is
            // null, as a new root actor of the level.
            void ReparentActor(std::shared_ptr<Engine::Objects::Actor> actor, std::shared_ptr<Engine::Objects::Actor> newParent);

            Core::EditorMainWindow* parent = nullptr;
            Engine::Core::ObjectID selectedID;
            char renameBuffer[256];
            std::shared_ptr<Engine::Objects::Actor> renamingActor = nullptr;
            bool openRenamingPopup = false;
    };


}