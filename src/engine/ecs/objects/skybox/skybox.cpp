#include "skybox.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/texture/cubemap/cubemap.hpp"
#include "engine/rendering/texture/cubemap/envmap.hpp"
#include "engine/rendering/material/material.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/rendering/shader/shader.hpp"

namespace Pulse::Engine::ECS::Objects{
    
    Skybox::Skybox(std::shared_ptr<Rendering::EnvironmentMap> envMap, std::shared_ptr<Rendering::Material> material)
    {
        m_EnvMap = envMap;
        m_Material = material;
        CreateDrawCommands();
    }

    void Skybox::SetEnvironmentMap(std::shared_ptr<Rendering::EnvironmentMap> envMap)
    {
        m_EnvMap = envMap;
        CreateDrawCommands();
    }

    void Skybox::SetMaterial(std::shared_ptr<Rendering::Material> material)
    {
        m_Material = material;
        CreateDrawCommands();
    }

    void Skybox::Destroy()
    {
        LevelObject::Destroy();
    }

    void Skybox::CreateDrawCommands()
    {
        std::shared_ptr<Rendering::Mesh> unitCube = Core::GetEngine().GetRenderer()->GetUnitCube();
        Rendering::PipelineSpecifications specs;
        specs.vertexLayout = unitCube->GetVertexLayout();
        specs.depthCompare = Rendering::DepthCompareOp::LessOrEqual;
        specs.depthWrite = false;
        specs.cullMode = Rendering::CullMode::Front;
        specs.shader = m_Material->GetShader();
        specs.topology = Rendering::PrimitiveTopology::Triangles;
        specs.debugName = "SkyboxPipeline";

        std::shared_ptr<Rendering::Pipeline> pipeline = Core::GetEngine().GetRenderer()->GetOrAdd(specs);
        m_Material->SetPipeline(pipeline);
        m_Material->SetTextureParameter("gCubemapTexture", m_EnvMap->GetCubemap()->GetHandle());

        Rendering::DrawCommand cmd;
        cmd.objectID = id.GetAsInt();
        cmd.modelMatrix = glm::mat4(1.0f);
        cmd.mesh = unitCube;
        cmd.indexCount = unitCube->GetIndexCount();
        cmd.indexOffset = 0;
        cmd.vertexCount = unitCube->GetVertexCount();
        cmd.material = m_Material;

        Core::GetEngine().GetRenderer()->AddOrUpdateCommands({cmd}, {"ForwardPass"});
    }
}