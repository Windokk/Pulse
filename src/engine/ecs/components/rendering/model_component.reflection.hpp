#pragma once

#include "model_component.hpp"
#include "engine/core/reflection_fields.hpp"

// Reflection for class Model

inline FieldInfo Model_mesh_info = {
    "mesh",
    TypeID::Asset,
    32,
    ReadMeshAssetFromModel,
    WriteMeshAssetToModel,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    nullptr
};

static Container Model_materials_container = MakeVectorContainer<std::shared_ptr<Pulse::Engine::Rendering::Material>, Pulse::Engine::Filesystem::AssetID>(ReadMaterialAssetFromModel, WriteMaterialAssetToModel);

inline FieldInfo Model_materials_info = {
    "materials",
    TypeID::Asset,
    48,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    &Model_materials_container,
    nullptr
};

inline ClassDescriptor Pulse::Engine::ECS::Components::Model::descriptor = {
    "Model",
    {
        &Model_mesh_info,
        &Model_materials_info,
    }
};

