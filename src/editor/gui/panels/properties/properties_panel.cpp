#include "properties_panel.hpp"

#include "engine/core/reflection_fields.hpp"
#include "engine/core/engine.hpp"

#include "editor/gui/main_window.hpp"

using namespace Pulse::Engine::ECS::Objects;
using namespace Pulse::Engine::ECS::Components;

namespace Pulse::Editor::GUI{

    bool InputVector3(const char* label, float v[3], float speed = 0.1f)
    {
        bool changed = false;

        ImGui::PushID(label);

        float lineHeight = ImGui::GetFrameHeight();
        ImVec2 fieldSize = ImVec2(ImGui::CalcItemWidth() / 3.0f - 4.0f, 0);

        const ImU32 colors[3] = {
            IM_COL32(220, 50, 50, 255),   // X - Red
            IM_COL32(50, 200, 70, 255),   // Y - Green
            IM_COL32(80, 120, 255, 255)   // Z - Blue
        };

        for (int i = 0; i < 3; i++)
        {
            ImGui::PushID(i);

            ImVec2 cursorPos = ImGui::GetCursorScreenPos();

            // Draw colored left bar
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + 3.0f, cursorPos.y + lineHeight),
                colors[i]
            );

            // Offset input so it doesn't overlap bar
            ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + 4.0f, cursorPos.y));

            ImGui::PushItemWidth(fieldSize.x - 4.0f);
            changed |= ImGui::DragFloat("##v", &v[i], speed);
            ImGui::PopItemWidth();

            if (i < 2)
                ImGui::SameLine();

            ImGui::PopID();
        }

        ImGui::SameLine();

        ImGui::TextUnformatted(label);

        ImGui::PopID();

        return changed;
    }

    void PropertiesPanel::Draw(std::shared_ptr<Actor> actor)
    {
        ImGui::Begin("Properties");

        if(actor){
            
            DrawActorInfo(actor);

            ImGui::Separator();

            for (int i = 0; i < actor->GetComponents().size(); i++)
            {
                ImGui::PushID(i);
                DrawComponent(actor->GetComponents()[i]);
                ImGui::PopID();
            }
        }

        ImGui::End();
    }

    void PropertiesPanel::DrawActorInfo(std::shared_ptr<Actor> actor)
    {
        char buffer[256];
        strcpy(buffer, actor->GetName().c_str());

        if (ImGui::InputText("Name", buffer, sizeof(buffer)))
        {
            actor->SetName(buffer);
        }

        ImGui::Text("ID: %d", actor->GetID().GetAsInt());
        ImGui::Text("Components: %zu", actor->GetComponents().size());
    }

    void PropertiesPanel::DrawComponent(std::shared_ptr<Component> comp)
    {
        const ClassDescriptor* desc = comp->GetDescriptor();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;

        if (ImGui::CollapsingHeader(desc->name.c_str(), flags))
        {
            bool active = comp->Active();
            if (ImGui::Checkbox("Active", &active))
            {
                active ? comp->Activate() : comp->DeActivate();
            }

            ImGui::Separator();

            for (FieldInfo* field : desc->fields)
            {
                void* valuePtr = FieldRead(*field, comp.get());
                DrawField(field, valuePtr, comp);
            }
        }
    }

    void PropertiesPanel::DrawField(const FieldInfo *field, void *value, std::shared_ptr<Engine::ECS::Components::Component> comp, const Container *container)
    {
        switch(container ? field->container->elementType : field->type){
            case TypeID::Int32:
            {
                int* v = static_cast<int*>(value);
                if (ImGui::InputInt(field->name, v))
                {
                    FieldChangedEvent evt{ field, *v };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Float:
            {
                float* v = static_cast<float*>(value);
                if (ImGui::InputFloat(field->name, v))
                {
                    FieldChangedEvent evt{ field, *v };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Bool:
            {
                bool* v = static_cast<bool*>(value);
                if (ImGui::Checkbox(field->name, v))
                {
                    FieldChangedEvent evt{ field, *v };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::String:
            {
                std::string* str = static_cast<std::string*>(value);
                char buffer[256];
                strcpy(buffer, str->c_str());

                if (ImGui::InputText(field->name, buffer, sizeof(buffer)))
                {
                    *str = buffer;

                    FieldChangedEvent evt{ field, *str };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Quat:
            {
                glm::quat* quat = static_cast<glm::quat*>(value);
                
                glm::vec3 vec = glm::degrees(glm::eulerAngles(*quat));
                
                if (InputVector3(field->name, &vec.x))
                {
                    *quat = glm::quat(glm::radians(vec));
                    FieldChangedEvent evt{ field, *quat };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Enum:
            {
                int current;
                memcpy(&current, value, field->enumDesc->size);

                std::vector<const char*> items;
                for (auto& e : field->enumDesc->values)
                    items.push_back(e.name);

                if (ImGui::Combo(field->name, &current,
                                items.data(), items.size()))
                {
                    memcpy(value, &current, field->enumDesc->size);

                    FieldChangedEvent evt{ field, current };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Vector:
            {
                if (!field->container)
                    break;

                if (ImGui::TreeNode(field->name))
                {
                    size_t count = field->container->Size(value);

                    for (size_t i = 0; i < count; i++)
                    {
                        void* element = field->container->GetByIndex(value, i);

                        ImGui::PushID((int)i);
                        DrawField(field, element, comp, field->container);
                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                }
                break;
            }
            case TypeID::Vec3:
            {
                glm::vec3* vec = static_cast<glm::vec3*>(value);

                if (InputVector3(field->name, &vec->x))
                {
                    FieldChangedEvent evt{ field, *vec };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
        }

    }
}