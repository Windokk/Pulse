#pragma once

#include "engine/rendering/renderer/renderer_api.hpp"

#include <glm/glm.hpp>

namespace Pulse::Engine::Rendering{

    class Shader;

    class GLRendererAPI : public RendererAPI{
        
        public :
            void ExecuteDrawCommand(const DrawCommand& command, const std::shared_ptr<RenderPass> pass) override;

            void ToggleMultisampling(const bool on) override;

            void SetViewport(uint32_t x, uint32_t y,
                                        uint32_t width, uint32_t height) override;

            void SetClearColor(float r, float g, float b, float a) override;

            void Clear(ClearBit clearBits) override;

            std::string GetDeviceVendor() override;
            std::string GetRendererName() override;
            std::string GetDriverVersion() override;

        private: 
            
            void BindMesh(Mesh* mesh);

            void BindPipeline(std::shared_ptr<Pipeline> pipeline);

            void BindMaterial(Material* mat);

            void BindPassData(const std::shared_ptr<RenderPass> pass, std::shared_ptr<Pipeline> pipeline);

            void BindPassData(const std::shared_ptr<RenderPass> pass, Material* material);

            void BindLevelState(std::shared_ptr<Shader> shader, glm::mat4 modelMatrix, int objectID);

            void DrawIndexed(const std::shared_ptr<Pipeline> pipeline, uint32_t indexCount, uint32_t indexOffset);
                
            void DrawFullScreenTriangle();
    };
}