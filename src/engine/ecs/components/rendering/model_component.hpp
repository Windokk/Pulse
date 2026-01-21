#pragma once

#include "engine/ecs/components/core/component.hpp"

#include "engine/rendering/mesh/mesh.hpp"

namespace Pulse::Engine::Rendering{
    class Mesh;
    class Material;
}

namespace Pulse::Engine::ECS::Components
{
    class Model : public Component{
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
            
        private:

            bool alreadySubmitted = false;

            ATTRIBUTE(Editable, write=WriteMeshAssetToModel, read=ReadMeshAssetFromModel, type=Pulse::Engine::Filesystem::AssetID) 
            std::shared_ptr<Rendering::Mesh> mesh;
            
            ATTRIBUTE(Editable, cwrite=WriteMaterialAssetToModel, cread=ReadMaterialAssetFromModel, type=Pulse::Engine::Filesystem::AssetID) 
            std::vector<std::shared_ptr<Rendering::Material>> materials;

            DECLARE_DESCRIPTOR(Model)

    };
}

inline void WriteMeshAssetToModel(void* object, const void* value);

inline void ReadMeshAssetFromModel(void* object, void* outValue);

inline void WriteMaterialAssetToModel(void* component, void* element, const void* editorValue);

inline void ReadMaterialAssetFromModel(const void* element, void* outEditorValue);