#include "gl_api.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"
#include "engine/rendering/backends/opengl/material/gl_material.hpp"
#include "engine/rendering/backends/opengl/pipeline/gl_pipeline.hpp"
#include "engine/rendering/backends/opengl/mesh/gl_mesh.hpp"
#include "engine/rendering/camera/camera_manager.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/lighting/light_manager.hpp"
#include "engine/rendering/texture/cubemap/envmap.hpp"
#include "engine/rendering/lighting/shadow_manager.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/rendering/backends/opengl/shader/gl_shader.hpp"

#include "engine/core/engine.hpp"
#include "engine/levels/level_manager.hpp"

namespace Pulse::Engine::Rendering{

    GLenum PrimitiveTopologyToGL(PrimitiveTopology topology)
    {
        switch (topology)
        {
            case PrimitiveTopology::Points:    return GL_POINTS;
            case PrimitiveTopology::Lines:     return GL_LINES;
            case PrimitiveTopology::LineStrip: return GL_LINE_STRIP;
            case PrimitiveTopology::Triangles: return GL_TRIANGLES;
            case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
            case PrimitiveTopology::TriangleFan: return GL_TRIANGLE_FAN;
        }

        return GL_TRIANGLES;
    }

    void GLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        glViewport(x, y, width, height);
    }

    void GLRendererAPI::SetClearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

    void GLRendererAPI::Clear(ClearBit clearBits)
    {
        GLbitfield bits = 0;

        if ((uint32_t)clearBits & (uint32_t)ClearBit::Color)
            bits |= GL_COLOR_BUFFER_BIT;

        if ((uint32_t)clearBits & (uint32_t)ClearBit::Depth)
            bits |= GL_DEPTH_BUFFER_BIT;

        if ((uint32_t)clearBits & (uint32_t)ClearBit::Stencil)
            bits |= GL_STENCIL_BUFFER_BIT;

        glClear(bits);
    }

    std::string GLRendererAPI::GetDeviceVendor()
    {
        const GLubyte* vendor = glGetString(GL_VENDOR);
        return vendor ? reinterpret_cast<const char*>(vendor) : "Unknown";
    }

    std::string GLRendererAPI::GetRendererName()
    {
        const GLubyte* renderer = glGetString(GL_RENDERER);
        return renderer ? reinterpret_cast<const char*>(renderer) : "Unknown";
    }

    std::string GLRendererAPI::GetDriverVersion()
    {
        const GLubyte* version = glGetString(GL_VERSION);
        return version ? reinterpret_cast<const char*>(version) : "Unknown";
    }

    void GLRendererAPI::BindMesh(std::shared_ptr<Mesh> mesh)
    {
        std::shared_ptr<GLMesh> glMesh = std::static_pointer_cast<GLMesh>(mesh);
        glBindVertexArray(glMesh->GetVAO());
    }

    void GLRendererAPI::BindPipeline(std::shared_ptr<Pipeline> pipeline)
    {
        std::shared_ptr<GLPipeline> glPipeline = std::static_pointer_cast<GLPipeline>(pipeline);
        glPipeline->Bind();
    }

    void GLRendererAPI::BindMaterial(std::shared_ptr<Material> mat)
    {
        std::shared_ptr<GLMaterial> glMat = std::static_pointer_cast<GLMaterial>(mat);
        glMat->Bind();
    }

    void GLRendererAPI::BindPassData(const std::shared_ptr<RenderPass> pass, std::shared_ptr<Pipeline> pipeline)
    {
        std::shared_ptr<GLShader> glShader = std::static_pointer_cast<GLShader>(pipeline->GetSpecifications().shader);
        glShader->Bind();

        uint32_t textureSlot = 0;

        for (auto& [name, texture] : pass->customSamplers)
        {
            glActiveTexture(GL_TEXTURE0 + textureSlot);
            glBindTexture(GL_TEXTURE_2D, texture);

            glShader->SetInt(name, textureSlot);

            textureSlot++;
        }

        for (auto& [name, value] : pass->customUniforms)
        {
            std::visit([&](auto&& v)
            {
                using T = std::decay_t<decltype(v)>;

                if constexpr (std::is_same_v<T, bool>)
                    glShader->SetBool(name, v);

                else if constexpr (std::is_same_v<T, int>)
                    glShader->SetInt(name, v);

                else if constexpr (std::is_same_v<T, float>)
                    glShader->SetFloat(name, v);

                else if constexpr (std::is_same_v<T, glm::vec2>)
                    glShader->SetVec2(name, v);

                else if constexpr (std::is_same_v<T, glm::vec3>)
                    glShader->SetVec3(name, v);

                else if constexpr (std::is_same_v<T, glm::vec4>)
                    glShader->SetVec4(name, v);

                else if constexpr (std::is_same_v<T, glm::mat2>)
                    glShader->SetMat2(name, v);

                else if constexpr (std::is_same_v<T, glm::mat3>)
                    glShader->SetMat3(name, v);

                else if constexpr (std::is_same_v<T, glm::mat4>)
                    glShader->SetMat4(name, v);

            }, value);
        }
    }

    void GLRendererAPI::BindPassData(const std::shared_ptr<RenderPass> pass, std::shared_ptr<Material> material)
    {
        for (auto& [name, texture] : pass->customSamplers){
            material->SetTextureParameter(name, texture);
        }

        for (auto& [name, value] : pass->customUniforms){
            material->SetScalarParameter(name, value);
        }
    }

    void GLRendererAPI::BindLevelState(std::shared_ptr<Shader> shader, glm::mat4 modelMatrix, int objectID)
    {
        auto level = Core::GetEngine().GetLevelManager()->GetLevelAt(0);
        auto camera = Core::GetEngine().GetCameraManager()->GetActiveCamera();

        auto it = shader->GetActiveUniformsMap().find("useEnvReflections");

        if(it != shader->GetActiveUniformsMap().end() && level->skybox){
            //Bind skybox data
            glBindTextureUnit(shader->GetActiveSamplersMap().find("ibl_irradianceMap")->second.binding, level->skybox->GetEnvMap()->GetIrradiance()->GetHandle());

            glBindTextureUnit(shader->GetActiveSamplersMap().find("ibl_prefilteredEnvMap")->second.binding, level->skybox->GetEnvMap()->GetPrefilter()->GetHandle());

            glBindTextureUnit(shader->GetActiveSamplersMap().find("ibl_brdfLUT")->second.binding, level->skybox->GetEnvMap()->GetBRDFLUT()->GetHandle());
        }

        shader->SetMat4("model", modelMatrix);
        shader->SetMat4("projection", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetProjection());
        shader->SetMat4("view", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetView());
        shader->SetInt("lightNB", Core::GetEngine().GetRenderer()->GetLightManager()->GetLightsCount());
        shader->SetVec3("camPos", Core::GetEngine().GetCameraManager()->GetActiveCamera()->parent->transform->GetPosition());
        shader->SetFloat("ambientIntensity", Core::GetEngine().GetLevelManager()->GetLevelAt(0)->ambientIntensity);
        shader->SetFloat("ambientIntensity", Core::GetEngine().GetLevelManager()->GetLevelAt(0)->ambientIntensity);
        shader->SetInt("objID", objectID);
    }

    void GLRendererAPI::DrawIndexed(const std::shared_ptr<Pipeline> pipeline, uint32_t indexCount, uint32_t indexOffset)
    {
        glDrawElements(
            PrimitiveTopologyToGL(pipeline->GetSpecifications().topology),
            indexCount,
            GL_UNSIGNED_INT,
            (void*)(indexOffset * sizeof(uint32_t))
        );
    }

    void GLRendererAPI::DrawFullScreenTriangle()
    {
        glBindVertexArray(0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void GLRendererAPI::ExecuteDrawCommand(const DrawCommand &command, const std::shared_ptr<RenderPass> pass)
    {
        if(!command.fullscreenTri)
            BindMesh(command.mesh);

        std::shared_ptr<Pipeline> pipeline = nullptr;
        if(pass->overridePipeline)
            pipeline = pass->customPipeline;
        else
            pipeline = command.material->GetPipeline();
        BindPipeline(pipeline);

        if(!command.fullscreenTri)
            BindLevelState(pipeline->GetSpecifications().shader, command.modelMatrix, command.objectID);

        if(pass->overridePipeline){
            BindPassData(pass, pipeline);
        }
        else{
            if(command.material->GetRecieveShadows())
                Core::GetEngine().GetRenderer()->GetShadowManager()->BindShadowMaps(command.material);
            BindPassData(pass, command.material);
            BindMaterial(command.material);
        }
        
        if(command.fullscreenTri){
            DrawFullScreenTriangle();
        }
        else{
            DrawIndexed(pipeline, command.indexCount, command.indexOffset);
        }
    }

    void GLRendererAPI::ToggleMultisampling(const bool on)
    {
        if(on)
            glEnable(GL_MULTISAMPLE);
        else
            glDisable(GL_MULTISAMPLE);
    }
}
