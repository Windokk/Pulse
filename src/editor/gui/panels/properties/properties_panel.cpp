#include "properties_panel.hpp"

#include "engine/core/reflection_fields.hpp"
#include "engine/core/engine.hpp"

#include "editor/gui/main_window.hpp"

#include <glm/glm.hpp>
#include <type_traits>

using namespace Pulse::Engine::ECS::Objects;
using namespace Pulse::Engine::ECS::Components;

namespace Pulse::Editor::GUI{

    template<typename T>
    bool InputVector4(const char* label, T v[4], float speed = 0.1f, float min = 0.0f, float max = 0.0f)
    {
        bool changed = false;

        ImGui::PushID(label);

        float lineHeight = ImGui::GetFrameHeight();
        ImVec2 fieldSize = ImVec2(ImGui::CalcItemWidth() / 4.0f - 4.0f, 0);

        const ImU32 colors[4] = {
            IM_COL32(220, 50, 50, 255),   // X - Red
            IM_COL32(50, 200, 70, 255),   // Y - Green
            IM_COL32(80, 120, 255, 255),  // Z - Blue
            IM_COL32(220, 180, 60, 255)   // W - Purple
        };

        for (int i = 0; i < 4; i++)
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

            if constexpr (std::is_same_v<T, int>) {
                changed |= ImGui::DragInt("##v", &v[i], speed, (int)min, (int)max);
            } else if constexpr (std::is_same_v<T, unsigned int>) {
                int temp = static_cast<int>(v[i]);
                if (ImGui::DragInt("##v", &temp, speed, 0, (int)max)) {
                    v[i] = static_cast<unsigned int>(temp);
                    changed = true;
                }
            } else if constexpr (std::is_floating_point_v<T>) {
                changed |= ImGui::DragFloat("##v", &v[i], speed, min, max);
            }

            ImGui::PopItemWidth();

            if (i < 3)
                ImGui::SameLine();

            ImGui::PopID();
        }

        ImGui::PopID();

        return changed;
    }

    template<typename T>
    bool InputVector3(const char* label, T v[3], float speed = 0.1f, float min = 0.0f, float max = 0.0f)
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
            
            if constexpr (std::is_same_v<T, int>) {
                changed |= ImGui::DragInt("##v", &v[i], speed, (int)min, (int)max);
            } else if constexpr (std::is_same_v<T, unsigned int>) {
                int temp = static_cast<int>(v[i]);
                if (ImGui::DragInt("##v", &temp, speed, 0, (int)max)) {
                    v[i] = static_cast<unsigned int>(temp);
                    changed = true;
                }
            } else if constexpr (std::is_floating_point_v<T>) {
                changed |= ImGui::DragFloat("##v", &v[i], speed, min, max);
            }
            
            ImGui::PopItemWidth();

            if (i < 2)
                ImGui::SameLine();

            ImGui::PopID();
        }

        ImGui::PopID();

        return changed;
    }

    template<typename T>
    bool InputVector2(const char* label, T v[2], float speed = 0.1f, float min = -FLT_MAX, float max = FLT_MAX)
    {
        bool changed = false;
        ImGui::PushID(label);

        float lineHeight = ImGui::GetFrameHeight();
        ImVec2 fieldSize = ImVec2(ImGui::CalcItemWidth() / 2.0f - 4.0f, 0);

        const ImU32 colors[2] = {
            IM_COL32(220, 50, 50, 255),   // X - Red
            IM_COL32(50, 200, 70, 255),   // Y - Green
        };

        for (int i = 0; i < 2; i++)
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

            if constexpr (std::is_same_v<T, int>) {
                changed |= ImGui::DragInt("##v", &v[i], speed, (int)min, (int)max);
            } else if constexpr (std::is_same_v<T, unsigned int>) {
                int temp = static_cast<int>(v[i]);
                if (ImGui::DragInt("##v", &temp, speed, 0, (int)max)) {
                    v[i] = static_cast<unsigned int>(temp);
                    changed = true;
                }
            } else if constexpr (std::is_floating_point_v<T>) {
                changed |= ImGui::DragFloat("##v", &v[i], speed, min, max);
            }

            ImGui::PopItemWidth();
            if (i < 1) ImGui::SameLine();
            ImGui::PopID();
        }

        ImGui::PopID();
        return changed;
    }

    template<typename MatType>
    bool InputMatrix(const char* label, MatType& m, float speed = 0.1f)
    {
        constexpr int C = MatType::length();                     // columns
        constexpr int R = MatType::col_type::length();            // rows

        static const ImU32 axisColors[4] =
        {
            IM_COL32(220, 50, 50, 255),   // X
            IM_COL32(80, 200, 80, 255),   // Y
            IM_COL32(80, 120, 220, 255),  // Z
            IM_COL32(220, 180, 60, 255)   // W
        };

        bool changed = false;

        ImGui::PushID(label);

        float cellWidth = ImGui::CalcItemWidth() / C;
        float lineHeight = ImGui::GetFrameHeight();

        ImVec2 pos = ImGui::GetCursorScreenPos();

        for (int r = 0; r < R; r++)
        {
            for (int c = 0; c < C; c++)
            {
                ImGui::PushID(r * C + c);

                ImGui::SetCursorScreenPos(ImVec2(pos.x + 8.0f + cellWidth * c, pos.y + 4 + lineHeight * r));

                // Colored axis bar
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos(),
                    ImVec2(ImGui::GetCursorScreenPos().x - 3.0f, ImGui::GetCursorScreenPos().y + lineHeight),
                    axisColors[r]
                );

                ImGui::PushItemWidth(cellWidth - 6.0f);

                float v = m[c][r];

                if (ImGui::DragFloat("##cell", &v, speed))
                {
                    m[c][r] = v;
                    changed = true;
                }

                ImGui::PopItemWidth();

                if (c < C - 1)
                    ImGui::SameLine();

                ImGui::PopID();
            }
        }

        ImGui::PopID();

        return changed;
    }

    void PropertiesPanel::Draw(std::shared_ptr<Actor> actor)
    {
        ImGui::Begin("Properties");

        if(actor){
            
            DrawActorInfo(actor);

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

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Name");
        ImGui::SameLine();

        if (ImGui::InputText("##ActorName", buffer, sizeof(buffer)))
        {
            actor->SetName(buffer);
        }

        ImGui::Text("Object ID: %d", actor->GetID().GetAsInt());
        ImGui::Text("Components: %zu", actor->GetComponents().size());
    }

    void PropertiesPanel::DrawComponent(std::shared_ptr<Component> comp)
    {
        const ClassDescriptor* desc = comp->GetDescriptor();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
        ImGui::SetNextItemAllowOverlap();
        bool open = ImGui::CollapsingHeader(desc->name.c_str(), flags);

        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        float headerHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

        bool active = comp->Active();
        std::string id = "##" + desc->name + "Active";
        if (ImGui::Checkbox(id.c_str(), &active))
        {
            active ? comp->Activate() : comp->DeActivate();
        }

        ImGui::PopStyleVar();

        if (open)
        {
            if (ImGui::BeginTable("##PropertiesTable", 2, ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("##Property", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch);

                for (FieldInfo* field : desc->fields)
                {
                    if (field->type == TypeID::Struct)
                    {
                        uint8_t* base = reinterpret_cast<uint8_t*>(comp.get()) + field->offset;
                        InstancedStruct* structPtr = reinterpret_cast<InstancedStruct*>(base);

                        for (auto& subField : structPtr->descriptor->fields)
                        {
                            void* valuePtr = FieldRead(*subField, structPtr->data);
                            DrawField(subField, valuePtr, comp);
                        }
                    }
                    else
                    {
                        void* valuePtr = FieldRead(*field, comp.get());
                        DrawField(field, valuePtr, comp);
                    }
                }
                
                ImGui::EndTable();
            }
        }
    }

    void PropertiesPanel::DrawField(const FieldInfo *field, void *value, std::shared_ptr<Engine::ECS::Components::Component> comp, const Container *container, const int valueIndexInContainer)
    {
        const char * fieldName = field->name;

        bool readOnly = (field->flags & ReadOnly) && !(field->flags & Editable);

        if(valueIndexInContainer != -1)
            fieldName = std::to_string(valueIndexInContainer).c_str();

        ImGui::TableNextRow();

        // Column 0 : Label
        ImGui::TableSetColumnIndex(0);

        ImGui::AlignTextToFramePadding();
        ImGui::Text(fieldName);

        // Column 1 : Widget
        ImGui::TableSetColumnIndex(1);

        std::string id = "##" + std::string(fieldName);

        if(readOnly)
            ImGui::BeginDisabled();

        switch(container ? container->elementType : field->type){
            case TypeID::Float:
            {
                float* v = static_cast<float*>(value);
                if (ImGui::DragFloat(id.c_str(), v, 0.05f, field->min, field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Double:
            {
                double* v = static_cast<double*>(value);
                if (ImGui::DragScalar(id.c_str(), ImGuiDataType_Double, v, 0.05f, &field->min, &field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Int8:
            {
                int8_t* v = static_cast<int8_t*>(value);
                int tmp = static_cast<int>(*v);
                if (ImGui::DragInt(id.c_str(), &tmp, 0.05f, (int8_t)field->min, (int8_t)field->max))
                {
                    *v = static_cast<int8_t>(tmp);
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Int16:
            {
                int16_t* v = static_cast<int16_t*>(value);
                int tmp = static_cast<int>(*v);
                if (ImGui::DragInt(id.c_str(), &tmp, 1.0f, (int16_t)field->min, (int16_t)field->max))
                {
                    *v = static_cast<int16_t>(tmp);
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Int32:
            {
                int* v = static_cast<int*>(value);
                if (ImGui::DragInt(id.c_str(), v, 0.05f, (int32_t)field->min, (int32_t)field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Int64:
            {
                int64_t* v = static_cast<int64_t*>(value);
                if (ImGui::DragScalar(id.c_str(), ImGuiDataType_S64, v, 0.05f, (int64_t*)&field->min, (int64_t*)&field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::UInt8:
            {
                uint8_t* v = static_cast<uint8_t*>(value);
                int tmp = static_cast<int>(*v);
                if (ImGui::DragInt(id.c_str(), &tmp, 1.0f, 0, (uint8_t)field->max))
                {
                    *v = static_cast<uint8_t>(tmp);
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::UInt16:
            {
                uint16_t* v = static_cast<uint16_t*>(value);
                int tmp = static_cast<int>(*v);
                if (ImGui::DragInt(id.c_str(), &tmp, 1.0f, 0, (uint16_t)field->max))
                {
                    *v = static_cast<uint16_t>(tmp);
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::UInt32:
            {
                uint32_t* v = static_cast<uint32_t*>(value);
                if (ImGui::DragScalar(id.c_str(), ImGuiDataType_U32, v, 0, (uint32_t*)&field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::UInt64:
            {
                uint64_t* v = static_cast<uint64_t*>(value);
                if (ImGui::DragScalar(id.c_str(), ImGuiDataType_U64, v, 0, (uint64_t*)&field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Bool:
            {
                bool* v = static_cast<bool*>(value);
                if (ImGui::Checkbox(id.c_str(), v))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Asset:
            {
                Filesystem::AssetID* asset = static_cast<Filesystem::AssetID*>(value);

                static char buffer[256] = "";

                auto* manager = Engine::Core::GetEngine().GetAssetIDManager();

                if (asset)
                {
                    if (manager)
                    {
                        auto assetPtr = manager->GetAssetFromID(*asset);
                        if (assetPtr)
                        {
                            const std::string& name = assetPtr->baseInfos.nameInProject;

                            if (!name.empty())
                            {
                                strncpy(buffer, name.c_str(), sizeof(buffer) - 1);
                                buffer[sizeof(buffer) - 1] = '\0'; // enforce null-termination
                            }
                        }
                    }
                }

                if (asset && ImGui::InputText(id.c_str(), buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    if(manager)
                    {
                        *static_cast<Filesystem::AssetID*>(value) = manager->GetIDFromNameInProject(buffer);
                    }
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::String:
            {
                std::string* str = static_cast<std::string*>(value);
                static char buffer[256] = "";

                if (!str->empty()) {
                    strncpy(buffer, str->c_str(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                }

                if (ImGui::InputText(id.c_str(), buffer, sizeof(buffer)))
                {
                    *str = std::string(buffer);
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Quat:
            {
                glm::quat* quat = static_cast<glm::quat*>(value);
                
                glm::vec3 vec = glm::degrees(glm::eulerAngles(*quat));
                
                if (InputVector3<float>(fieldName, &vec.x, 0.1f, field->min, field->max))
                {
                    *quat = glm::quat(glm::radians(vec));
                    FieldChangedEvent evt{ field };
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

                if (ImGui::Combo(id.c_str(), &current,
                                items.data(), items.size()))
                {
                    memcpy(value, &current, field->enumDesc->size);

                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Vector:
            {
                if (!field->container)
                    break;

                if (ImGui::TreeNode(id.c_str()))
                {
                    size_t count = field->container->Size(value);

                    for (size_t i = 0; i < count; i++)
                    {
                        void* element = field->container->GetByIndex(value, i);

                        ImGui::PushID((int)i);
                        DrawField(field, element, comp, field->container, i);
                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                }
                break;
            }
            case TypeID::Vec4:
            {
                glm::vec4* vec = static_cast<glm::vec4*>(value);

                if (InputVector4<float>(fieldName, &vec->x, 0.1f, field->min, field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            } 
            case TypeID::Vec3:
            {
                glm::vec3* vec = static_cast<glm::vec3*>(value);

                if (InputVector3<float>(fieldName, &vec->x, 0.1f, field->min, field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Vec2:
            {
                glm::vec2* vec = static_cast<glm::vec2*>(value);

                if (InputVector2<float>(fieldName, &vec->x, 0.1f, field->min, field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::IVec2:
            {
                glm::ivec2* vec = static_cast<glm::ivec2*>(value);

                if (InputVector2<int>(fieldName, &vec->x, 1.0f, (int)field->min, (int)field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            } 
            case TypeID::IVec3:
            {
                glm::ivec3* vec = static_cast<glm::ivec3*>(value);

                if (InputVector3<int>(fieldName, &vec->x, 1.0f, (int)field->min, (int)field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::IVec4:
            {
                glm::ivec2* vec = static_cast<glm::ivec2*>(value);

                if (InputVector4<int>(fieldName, &vec->x, 1.0f, (int)field->min, (int)field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::UVec2:
            {
                glm::uvec2* vec = static_cast<glm::uvec2*>(value);

                if (InputVector2<unsigned int>(fieldName, &vec->x, 1.0f, 0.0f, (int)field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            } 
            case TypeID::UVec3:
            {
                glm::uvec3* vec = static_cast<glm::uvec3*>(value);

                if (InputVector3<unsigned int>(fieldName, &vec->x, 1.0f, 0.0f, (int)field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::UVec4:
            {
                glm::uvec2* vec = static_cast<glm::uvec2*>(value);

                if (InputVector4<unsigned int>(fieldName, &vec->x, 1.0f, 0.0f, (int)field->max))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Mat2:
            {
                glm::mat2* mat = static_cast<glm::mat2*>(value);

                if (InputMatrix(field->name, *mat))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Mat3:
            {
                glm::mat3* mat = static_cast<glm::mat3*>(value);

                if (InputMatrix(field->name, *mat))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::Mat4:
            {
                glm::mat4* mat = static_cast<glm::mat4*>(value);

                if (InputMatrix(field->name, *mat))
                {
                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::ColorRGB:
            {
                COL_RGB* v = static_cast<COL_RGB*>(value);
                static float value[3] = {v->r(), v->g(), v->b()};
                if(ImGui::ColorEdit3(id.c_str(), value)){

                    *v = COL_RGB(value[0], value[1], value[2]);

                    FieldChangedEvent evt{field};
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::ColorRGBA:
            {
                COL_RGBA* v = static_cast<COL_RGBA*>(value);
                static float value[4] = {v->r(), v->g(), v->b(), v->a()};
                if(ImGui::ColorEdit4(id.c_str(), value)){

                    *v = COL_RGBA(value[0], value[1], value[2], value[3]);

                    FieldChangedEvent evt{field};
                    comp->OnFieldChanged(evt);
                }
                break;
            }
            case TypeID::CString:
            {
                char* str = static_cast<char*>(value);

                static char buffer[256];

                strncpy(buffer, str, sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';

                if (ImGui::InputText(id.c_str(), buffer, sizeof(buffer)))
                {
                    strncpy(str, buffer, 255);
                    str[255] = '\0';

                    FieldChangedEvent evt{ field };
                    comp->OnFieldChanged(evt);
                }

                break;
            }
        }

        if(readOnly)
            ImGui::EndDisabled();
    }
}