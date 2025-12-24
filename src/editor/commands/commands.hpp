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
            virtual void Execute() = 0;  // Apply the change
            virtual void Undo() = 0;     // Revert the change
            virtual void Redo() = 0;     // Reapply the change
    };

    class CompositeCommand : public Command {
        public:
            void Add(std::unique_ptr<Command> cmd) {
                commands.push_back(std::move(cmd));
            }

            void Execute() override {
                Redo();
            }

            bool Empty() const {
                return commands.empty();
            }

            void Undo() override {
                // undo in reverse order
                for (auto it = commands.rbegin(); it != commands.rend(); ++it)
                    (*it)->Undo();
            }

            void Redo() override {
                // redo in original order
                for (auto& cmd : commands)
                    cmd->Redo();
            }
            
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

    inline void FieldRead(const FieldInfo& field, void* object, void* out) {
        if (field.read) {
            field.read(object, out);
        } else {
            std::memcpy(
                out,
                static_cast<uint8_t*>(object) + field.offset,
                GetTypeSize(field.type)
            );
        }
    }

    inline void FieldWrite(const FieldInfo& field, void* object, const void* value) {
        if (field.write) {
            field.write(object, value);
        } else {
            std::memcpy(
                static_cast<uint8_t*>(object) + field.offset,
                value,
                GetTypeSize(field.type)
            );
        }
    }

    class ModifyFieldCommand : public Command {
        private:
            void* object;              // The object being modified
            const FieldInfo* field;          // The field being modified
            ValueBuffer before;        // Old value of the field (before the change)
            ValueBuffer after;         // New value of the field (after the change)

        public:
            ModifyFieldCommand(void* obj, const FieldInfo* f) : object(obj), field(f), before(f->type), after(f->type){
                // capture old value
                FieldRead(*field, object, before.ptr());
            }

            void Execute() override {
                Redo();
            }

            void CaptureAfter() {
                FieldRead(*field, object, after.ptr());
            }

            void Undo() override {
                FieldWrite(*field, object, before.ptr());
            }

            void Redo() override {
                FieldWrite(*field, object, after.ptr());
            }
    };
}
