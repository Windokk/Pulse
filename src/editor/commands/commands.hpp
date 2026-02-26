#pragma once

#include <iostream>
#include <stack>
#include <memory>
#include <cstring>

#include "engine/core/reflection_fields.hpp"

namespace Pulse::Editor::Commands{

    class Command {
        public:
            virtual ~Command() = default;

            // Called once when added to the stack
            virtual void Execute() = 0;

            // History operations
            virtual void Undo() = 0;
            virtual void Redo() = 0;

            // Hooks
            virtual void Finalize() {}
            virtual bool IsValid() const { return true; }
            virtual const char* GetName() const = 0;
    };

    class CompositeCommand final : public Command {
        public:
            explicit CompositeCommand(const char* label)
                : name(label) {}

            void Add(std::unique_ptr<Command> cmd) {
                commands.push_back(std::move(cmd));
            }

            void Finalize() override {
                for (auto& cmd : commands)
                    cmd->Finalize();

                // Remove no-op commands
                commands.erase(
                    std::remove_if(
                        commands.begin(),
                        commands.end(),
                        [](auto& c) { return !c->IsValid(); }
                    ),
                    commands.end()
                );
            }

            bool IsValid() const override {
                return !commands.empty();
            }

            void Execute() override {
                for (auto& cmd : commands)
                    cmd->Execute();
            }

            void Undo() override {
                for (auto it = commands.rbegin();
                    it != commands.rend(); ++it)
                    (*it)->Undo();
            }

            void Redo() override {
                for (auto& cmd : commands)
                    cmd->Redo();
            }

            const char* GetName() const override {
                return name;
            }

        private:
            const char* name;
            std::vector<std::unique_ptr<Command>> commands;
    };

    struct ValueBuffer {
        const FieldInfo* field;
        std::unique_ptr<uint8_t[]> data;

        ValueBuffer(const FieldInfo* f)
            : field(f),
            data(std::make_unique<uint8_t[]>(GetTypeSize(f->type))) {}

        void* ptr() { return data.get(); }
        const void* ptr() const { return data.get(); }
    };

    class ModifyFieldCommand final : public Command {
        public:
            ModifyFieldCommand(void* obj, const FieldInfo* f)
                : object(obj), field(f),
                before(f), after(f)
            {
                void* realPtr = GetFieldPointer();
                field->CopyConstruct(before.ptr(), realPtr);
            }

            ~ModifyFieldCommand()
            {
                field->Destroy(before.ptr());
                field->Destroy(after.ptr());
            }

            void Finalize() override
            {
                if (!finalized)
                {
                    void* realPtr = GetFieldPointer();
                    field->CopyConstruct(after.ptr(), realPtr);
                    finalized = true;
                }
            }

            bool IsValid() const override
            {
                return !field->Equals(before.ptr(), after.ptr());
            }

            void Execute() override { Redo(); }

            void Undo() override
            {
                field->Assign(GetFieldPointer(), before.ptr());
            }

            void Redo() override
            {
                field->Assign(GetFieldPointer(), after.ptr());
            }

            const char* GetName() const override
            {
                return field->name;
            }

        private:
            void* GetFieldPointer() const
            {
                return static_cast<uint8_t*>(object) + field->offset;
            }

            void* object;
            const FieldInfo* field;
            ValueBuffer before;
            ValueBuffer after;
            bool finalized = false;
        };
}
