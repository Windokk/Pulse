#pragma once

#include "editor/gui/resources/icon_atlas.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

namespace Pulse::Engine::Rendering{
    class Mesh;
}

namespace Pulse::Editor::GUI {

    // Renders static meshes as small white/unlit thumbnails (a fixed 3/4-angle "product shot",
    // ignoring the mesh's real materials entirely) and packs them into one shared atlas texture,
    // the same way IconAtlas packs its file-loaded PNG icons - so the asset browser can show a
    // real silhouette of a mesh instead of a generic model icon.
    //
    // Rendering happens with raw OpenGL calls, deliberately bypassing the engine's normal
    // Renderer/RenderPass/DrawCommand pipeline: that pipeline is a per-frame pass DAG with no
    // "render this one thing right now" entry point, and reusing a shared cached Material would
    // leak this feature's one-off camera matrices into its persistent (and possibly shared, e.g.
    // materials/default.mat) parameter state. GetOrCreateThumbnail() is meant to be called only
    // from editor UI code drawn after the main scene's Renderer::Render() has already completed
    // for the frame (e.g. from AssetBrowser::Draw(), which runs during EditorMainWindow::
    // SwapBuffers() - after the frame's real render, before ImGui's own draw-data upload), so
    // this can't corrupt what's already been rendered, and GLStateCache::Reset() (called at the
    // start of both this and every following render pass) discards the state changes this leaves
    // behind, exactly the way it already discards ImGui's.
    class MeshThumbnailCache
    {
        public:
            static MeshThumbnailCache& Instance();

            // Call once per frame (AssetBrowser::Draw() does) to reset the per-frame cap on how
            // many new thumbnails get loaded+rendered this frame - keeps a folder full of
            // never-before-seen meshes from stalling a single frame.
            void BeginFrame();

            // Returns the atlas region for this mesh asset's thumbnail, lazily loading the mesh
            // and rendering it into the atlas the first time it's requested (subject to the
            // per-frame cap above). Returns nullptr while not ready yet - throttled, the atlas
            // ran out of slots, or the mesh failed to load - callers should fall back to a
            // generic icon in that case and simply ask again next frame.
            const AtlasRegion* GetOrCreateThumbnail(const std::string& meshPathInProject);

            uint32_t GetAtlasTextureHandle() const { return m_AtlasTexture; }

        private:
            MeshThumbnailCache() = default;

            void Init();
            void RenderThumbnail(const std::shared_ptr<Engine::Rendering::Mesh>& mesh, int slot);

            static constexpr int kAtlasSize = 1024;
            static constexpr int kCellSize = 128;
            static constexpr int kGridDim = kAtlasSize / kCellSize;
            static constexpr int kMaxNewPerFrame = 2;

            bool m_Initialized = false;

            uint32_t m_AtlasTexture = 0;
            uint32_t m_AtlasFBO = 0;

            uint32_t m_WorkingFBO = 0;
            uint32_t m_WorkingColor = 0;
            uint32_t m_WorkingDepth = 0;

            int m_NextSlot = 0;
            int m_RenderedThisFrame = 0;

            std::unordered_map<std::string, AtlasRegion> m_Regions;
    };
}
