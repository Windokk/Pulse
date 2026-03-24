#pragma once

#include "engine/rendering/renderer/renderer_api.hpp"

#include "engine/rendering/framebuffer/framebuffer.hpp"

#include "engine/rendering/pipeline/pipeline.hpp"

#include "engine/rendering/mesh/mesh.hpp"

#include <map>
#include <variant>

namespace Pulse::Engine::Rendering {

    class LightManager;
    class ShadowManager;
    using NumericValue = std::variant<bool, float, int, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

    class Renderer;

    struct RenderPass{
        std::shared_ptr<Framebuffer> target = nullptr;
        bool clearColor = true;
        bool clearDepth = true;
        std::map<std::string, NumericValue> customUniforms;
        std::map<std::string, uint64_t> customSamplers;
        bool overridePipeline = false;
        std::shared_ptr<Pipeline> customPipeline = nullptr;
        bool allowResize = true;
        bool allowCulling = true;
        private:
            std::vector<DrawCommand> drawList = {};
            bool drawListDirty = false;
            std::unordered_map<uint32_t, size_t> drawCommandsLookup;
        friend class Renderer;
    };


    /// @todo move this in a "setting manager" system
    struct RendererSettings{
        int viewportWidth;
        int viewportHeight;
        bool multisampling = true;
        RendererAPI::API api;
    };

    class Renderer{
        public:
            void Init(std::shared_ptr<RendererSettings> initialSettings);
            uint64_t GenerateSortKey(const DrawCommand &cmd, const uint32_t submeshID);
            std::shared_ptr<Pipeline> GetOrAdd(const PipelineSpecifications &specs);
            void Render();
            void Shutdown();
            void AddCommands(const std::vector<DrawCommand>& commands, const std::vector<std::string>& passes);
            void RemoveCommands(const std::vector<uint32_t> commandsID, const std::vector<std::string>& passes);
            void ForceDrawlistUpdate(const std::vector<std::string>& passes);

            void AddRenderPass(const std::shared_ptr<RenderPass> pass, const std::string& name, const std::vector<std::string>& dependencies);
            void RemoveRenderPass(const std::string& name);
            
            std::shared_ptr<RenderPass> GetRenderPass(const std::string& name) const
            {
                auto it = m_RenderPasses.find(name);

                if (it != m_RenderPasses.end())
                    return it->second;

                return nullptr;
            }

            bool HasRenderPass(const std::string& name) const { return m_RenderPasses.find(name) != m_RenderPasses.end(); }

            void RescaleFramebuffers(int newWidth, int newHeight);

            uint32_t GetViewportTextureHandle() const { return m_ViewportBuffer->GetResolveColorAttachment(); }

            void ToggleMultisampling(const bool on);

            std::string GetDeviceVendor();
            std::string GetRendererName();
            std::string GetDriverVersion();
            
            RendererAPI* GetRendererAPI() const 
            {
                if(m_RendererAPI) 
                    return m_RendererAPI.get();
                else
                    return nullptr; 
            }
            const std::shared_ptr<Mesh> GetUnitCube() { return m_UnitCube; }
            const std::shared_ptr<Mesh> GetUnitQuad() { return m_UnitQuad; }
            const std::shared_ptr<ShadowManager> GetShadowManager() { return m_ShadowManager; }
            const std::shared_ptr<LightManager> GetLightManager() { return m_LightManager; }



        private:

            void BeginFrame();
            void DrawFrame();
            void EndFrame();

            void ReorderDrawList();
            void BuildExecutionOrder();

            void BeginRenderPass(const std::shared_ptr<RenderPass>& pass);
            void ExecuteRenderPass();
            void EndRenderPass();

            std::shared_ptr<RendererAPI> m_RendererAPI;

            std::unordered_map<std::string, std::shared_ptr<RenderPass>> m_RenderPasses;
            std::unordered_map<std::string, std::vector<std::string>> m_RenderPassDependencies;
            std::vector<std::string> m_ExecutionOrder;
            std::shared_ptr<RenderPass> m_CurrentPass;
            std::vector<std::string> m_PassInsertionOrder;

            std::shared_ptr<Mesh> m_UnitCube = nullptr;
            std::shared_ptr<Mesh> m_UnitQuad = nullptr;

            std::shared_ptr<Framebuffer> m_ViewportBuffer;

            std::shared_ptr<ShadowManager> m_ShadowManager;
            std::shared_ptr<LightManager> m_LightManager;

            std::unordered_map<PipelineSpecifications,std::shared_ptr<Pipeline>,PipelineSpecsHash> m_Pipelines;

            bool m_MultisamplingEnabled = false;

            std::shared_ptr<RendererSettings> m_Settings;

            bool m_NeedExecutionOrderRebuild = false;
    };
}