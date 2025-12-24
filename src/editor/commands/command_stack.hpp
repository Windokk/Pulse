#pragma once

#include "commands.hpp"

namespace Pulse::Editor::Commands{

    class CommandStack {
        public:

            CommandStack(const CommandStack&) = delete;
            CommandStack& operator=(const CommandStack&) = delete;

            // Provide access to the singleton instance
            static CommandStack& GetInstance() {
                static CommandStack instance;
                return instance;
            }

            void Execute(std::unique_ptr<Command> cmd) {
                cmd->Redo();
                undoStack.push_back(std::move(cmd));
                redoStack.clear();
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
            CommandStack() = default;

            std::vector<std::unique_ptr<Command>> undoStack;
            std::vector<std::unique_ptr<Command>> redoStack;
    };


}