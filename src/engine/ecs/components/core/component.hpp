#pragma once

#include <cstdint>

#include <nlohmann/json.hpp>

using namespace nlohmann;
using ordered_json = nlohmann::ordered_json;

struct ClassDescriptor;

namespace Pulse::Engine::ECS{
    namespace Objects{
        class Actor;
    }

    namespace Components{

        template<typename BaseType, typename DerivedType>
        constexpr bool IsSubclassOf() {
            return std::is_base_of<BaseType, DerivedType>::value;
        }

        class Component{

            public:
                Component(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);
                virtual ~Component();
                virtual void Destroy();
                std::shared_ptr<Objects::Actor> parent = nullptr;

                /// @brief Activates this component
                virtual void Activate() { activated = true; }

                /// @brief Deactivates this component
                virtual void DeActivate() { activated = false; }

                virtual void Deserialize(json componentData) {};
                virtual ordered_json Serialize() { ordered_json dummy; return dummy; };

                template<typename T>
                bool IsInstanceOf() const {
                    return dynamic_cast<const T*>(this) != nullptr;
                }

                bool Active(){
                    return activated;
                }

                void SetLocalId(uint32_t newLocalId){
                    local_id = newLocalId;
                }

                uint32_t GetLocalId(){
                    return local_id;
                }

                /// @brief Custom logic for duplication of the component (copy fields)
                /// @return A "copy" of this component
                /// @todo
                /// Should be replaced with automatic cloning using reflection
                /// @endinternal
                virtual std::shared_ptr<Component> Clone() const = 0;

                void SetParent(std::shared_ptr<Objects::Actor> newParent){
                    parent = newParent;
                }


                virtual const ClassDescriptor* GetDescriptor() const = 0;

            private:

            protected:
                uint32_t local_id;
                bool activated = true;

        };
    }
    
}