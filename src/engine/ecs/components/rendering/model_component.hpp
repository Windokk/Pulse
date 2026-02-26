#pragma once

#include "engine/ecs/components/core/component.hpp"

#include "engine/rendering/mesh/mesh.hpp"

namespace Pulse::Engine::Rendering{
    class Mesh;
    class Material;
}

namespace Pulse::Engine::ECS::Components
{
    class CLASS() Model : public Component{
        public:
            Model(std::shared_ptr<Objects::Actor> parent, uint32_t localID);

            public:

            void Deserialize(json componentData) override;

            ordered_json Serialize() override;
            
            void SetMesh(std::string meshPath);
            void SetMesh(Filesystem::AssetID meshID);
            void UpdateReferenceInLevel();
            void SetMaterials(std::vector<std::shared_ptr<Rendering::Material>> &&materials);

            std::shared_ptr<Rendering::Mesh> GetMesh() { return mesh; }
            std::vector<std::shared_ptr<Rendering::Material>> GetMaterials() { return materials; }

            int GetMaterialsCount() { return materials.size(); }

            void Update();
            void RemoveFromDrawList();

            void Destroy() override;

            std::shared_ptr<Component> Clone() const override;
            
            FIELD(Editable)
            Filesystem::AssetID meshID;

            FIELD(Editable)
            std::vector<Filesystem::AssetID> materialsID;
            
            DECLARE_DESCRIPTOR(Model)

            void OnFieldChanged(const FieldChangedEvent &event) override;
        
        private:

            bool alreadySubmitted = false;

            std::shared_ptr<Rendering::Mesh> mesh;
            std::vector<std::shared_ptr<Rendering::Material>> materials;

    };
}