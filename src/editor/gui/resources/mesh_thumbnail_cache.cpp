#include "mesh_thumbnail_cache.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"
#include "engine/rendering/backends/opengl/mesh/gl_mesh.hpp"
#include "engine/rendering/backends/opengl/shader/gl_shader.hpp"

#include "engine/rendering/mesh/mesh.hpp"
#include "engine/rendering/shader/shader.hpp"

#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"

#include "engine/debugging/logger.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace Pulse::Editor::GUI{

    using namespace Pulse::Engine;

    MeshThumbnailCache& MeshThumbnailCache::Instance()
    {
        static MeshThumbnailCache instance;
        return instance;
    }

    void MeshThumbnailCache::BeginFrame()
    {
        m_RenderedThisFrame = 0;
    }

    void MeshThumbnailCache::Init()
    {
        if (m_Initialized)
            return;

        // Atlas: a plain color texture plus a thin FBO wrapper used only as a glBlitFramebuffer
        // destination (see RenderThumbnail) - it's never bound/cleared/drawn into directly, so it
        // needs no depth attachment of its own.
        glGenTextures(1, &m_AtlasTexture);
        glBindTexture(GL_TEXTURE_2D, m_AtlasTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kAtlasSize, kAtlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &m_AtlasFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_AtlasFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_AtlasTexture, 0);
        GLenum atlasDrawBuffer = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &atlasDrawBuffer);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            DEBUG_ERROR("MeshThumbnailCache: atlas framebuffer incomplete (0x", std::hex, status, std::dec, ")");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Working target: what we actually draw one mesh into, before blitting the result into
        // its slot in the shared atlas above.
        glGenTextures(1, &m_WorkingColor);
        glBindTexture(GL_TEXTURE_2D, m_WorkingColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kCellSize, kCellSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenRenderbuffers(1, &m_WorkingDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, m_WorkingDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kCellSize, kCellSize);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glGenFramebuffers(1, &m_WorkingFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_WorkingFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_WorkingColor, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_WorkingDepth);
        GLenum workingDrawBuffer = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &workingDrawBuffer);
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            DEBUG_ERROR("MeshThumbnailCache: working framebuffer incomplete (0x", std::hex, status, std::dec, ")");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_Initialized = true;
    }

    void MeshThumbnailCache::RenderThumbnail(const std::shared_ptr<Rendering::Mesh>& mesh, int slot)
    {
        auto shader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/mesh/unlit");
        if (!shader)
            return;

        auto glMesh = std::static_pointer_cast<Rendering::GLMesh>(mesh);
        auto glShader = std::static_pointer_cast<Rendering::GLShader>(shader);

        glm::vec3 boundsMin = mesh->GetBoundsMin();
        glm::vec3 boundsMax = mesh->GetBoundsMax();
        glm::vec3 center = (boundsMin + boundsMax) * 0.5f;
        glm::vec3 extents = (boundsMax - boundsMin) * 0.5f;
        float radius = glm::length(extents);
        if (radius < 0.001f)
            radius = 1.0f;

        const float fovY = 35.0f;
        float distance = (radius / sinf(glm::radians(fovY * 0.5f))) * 1.3f;

        // Fixed 3/4 "product shot" angle, same idea used for every mesh so the grid reads
        // consistently rather than each thumbnail being framed from a different direction.
        glm::vec3 dir = glm::normalize(glm::vec3(1.0f, 0.8f, 1.2f));
        glm::vec3 eye = center + dir * distance;

        glm::mat4 view = glm::lookAt(eye, center, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(fovY), 1.0f, distance * 0.01f, distance + radius * 2.0f + 1.0f);

        // This whole function calls raw GL directly instead of going through GLStateCache's
        // normal bind sites, so force it to distrust whatever it thinks is currently bound -
        // exactly the situation Reset()'s own doc comment calls out (ImGui, compute dispatches, ...).
        Rendering::GLStateCache::Reset();

        glBindFramebuffer(GL_FRAMEBUFFER, m_WorkingFBO);
        glViewport(0, 0, kCellSize, kCellSize);
        glClearColor(0.145f, 0.145f, 0.157f, 1.0f);
        Rendering::GLStateCache::SetDepthWrite(true);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Rendering::GLStateCache::SetDepthTest(true);
        Rendering::GLStateCache::SetDepthFunc(GL_LESS);
        Rendering::GLStateCache::SetCullMode(false, GL_BACK);
        Rendering::GLStateCache::SetBlendEnabled(false);
        Rendering::GLStateCache::SetPolygonMode(GL_FILL);

        glShader->Bind();
        shader->SetMat4("model", glm::mat4(1.0f));
        shader->SetMat4("uProjection", proj);
        shader->SetMat4("uView", view);
        shader->SetBool("useTexture", false);
        shader->SetBool("useCustomColor", true);
        shader->SetVec4("customColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        shader->SetBool("masked", false);

        Rendering::GLStateCache::BindVertexArray(glMesh->GetVAO());

        for (const auto& sub : mesh->GetSubMeshes())
        {
            glDrawElements(GL_TRIANGLES, (GLsizei)sub.indexCount, GL_UNSIGNED_INT, (void*)(sub.indexOffset * sizeof(uint32_t)));
        }

        int col = slot % kGridDim;
        int row = slot / kGridDim;
        int dstX = col * kCellSize;
        int dstY = row * kCellSize;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_WorkingFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_AtlasFBO);
        glBlitFramebuffer(
            0, 0, kCellSize, kCellSize,
            dstX, dstY, dstX + kCellSize, dstY + kCellSize,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Leave the cache distrustful for whatever runs next (this frame's ImGui pass, or the
        // first real render pass of the next frame - which would otherwise get its own reset
        // anyway, but this keeps the "state after this function" guarantee independent of that).
        Rendering::GLStateCache::Reset();
    }

    const AtlasRegion* MeshThumbnailCache::GetOrCreateThumbnail(const std::string& meshPathInProject)
    {
        if (!m_Initialized)
            Init();

        auto it = m_Regions.find(meshPathInProject);
        if (it != m_Regions.end())
            return &it->second;

        if (m_NextSlot >= kGridDim * kGridDim)
            return nullptr; // atlas full - caller falls back to the generic icon

        if (m_RenderedThisFrame >= kMaxNewPerFrame)
            return nullptr; // throttled - try again next frame

        auto mesh = Core::GetEngine().GetResourcesManager()->GetMesh(meshPathInProject);
        if (!mesh || mesh->GetIndexCount() == 0)
            return nullptr;

        int slot = m_NextSlot++;
        RenderThumbnail(mesh, slot);
        m_RenderedThisFrame++;

        int col = slot % kGridDim;
        int row = slot / kGridDim;
        float cellUV = 1.0f / (float)kGridDim;

        // V flipped (uv0 at the cell's bottom, uv1 at its top) to match how this codebase already
        // displays any other render-to-texture result in ImGui (see ViewportWindow::Draw()'s
        // ImGui::Image(..., ImVec2(0,1), ...) for the main viewport texture) - a plain top-down
        // UV mapping would show every mesh thumbnail upside down.
        AtlasRegion region;
        region.uv0 = ImVec2(col * cellUV, (row + 1) * cellUV);
        region.uv1 = ImVec2((col + 1) * cellUV, row * cellUV);

        auto inserted = m_Regions.emplace(meshPathInProject, region);
        return &inserted.first->second;
    }
}
