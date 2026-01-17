#pragma once

#include "commands.hpp"

namespace Pulse::Editor::Commands{

    class CommandStack {
        public:
            static CommandStack& Get() {
                static CommandStack instance;
                return instance;
            }

            void Begin(const char* name) {
                assert(!active);
                active = std::make_unique<CompositeCommand>(name);
            }

            void Add(std::unique_ptr<Command> cmd) {
                assert(active);
                active->Add(std::move(cmd));
            }

            void End() {
                assert(active);

                active->Finalize();

                if (active->IsValid()) {
                    active->Execute();
                    undoStack.push_back(std::move(active));
                    redoStack.clear();
                } else {
                    active.reset();
                }
            }

            void Undo() {
                if (undoStack.empty()) return;
                auto cmd = std::move(undoStack.back());
                undoStack.pop_back();
                cmd->Undo();
                redoStack.push_back(std::move(cmd));
            }

            void Redo() {
                if (redoStack.empty()) return;
                auto cmd = std::move(redoStack.back());
                redoStack.pop_back();
                cmd->Redo();
                undoStack.push_back(std::move(cmd));
            }

        private:
            std::unique_ptr<CompositeCommand> active;
            std::vector<std::unique_ptr<Command>> undoStack;
            std::vector<std::unique_ptr<Command>> redoStack;
    };
}