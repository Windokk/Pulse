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

            // Optional hooks
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
        TypeID type;
        size_t size;
        std::vector<uint8_t> data;

        ValueBuffer(TypeID t)
            : type(t), size(GetTypeSize(t)), data(size) {}

        void* ptr() { return data.data(); }
        const void* ptr() const { return data.data(); }
    };

    class ModifyFieldCommand final : public Command {
        public:
            ModifyFieldCommand(void* obj, const FieldInfo* f)
                : object(obj), field(f),
                before(f->type), after(f->type)
            {
                FieldRead(*field, object, before.ptr());
            }

            void Finalize() override {
                if (!finalized) {
                    FieldRead(*field, object, after.ptr());
                    finalized = true;
                }
            }

            bool IsValid() const override {
                return std::memcmp(
                    before.ptr(), after.ptr(), before.size
                ) != 0;
            }

            void Execute() override { Redo(); }

            void Undo() override {
                FieldWrite(*field, object, before.ptr());
            }

            void Redo() override {
                FieldWrite(*field, object, after.ptr());
            }

            const char* GetName() const override {
                return field->name;
            }

        private:
            void* object;
            const FieldInfo* field;
            ValueBuffer before, after;
            bool finalized = false;
        };
}
