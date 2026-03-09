#pragma once

#include "engine/rendering/utils.hpp"

#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Rendering {

    enum class ShaderDataType
    {   None         = 0,
        Bool,
        Int,
        Float,

        Vec2,
        Vec3,
        Vec4,

        Mat2,
        Mat3,
        Mat4,

        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture2DMultisample,

        Texture1DArray,
        Texture2DArray,
        TextureCubeArray,
        Texture2DMultisampleArray
    };

    struct UniformInfo
    {
        std::string name;
        ShaderDataType type;
        uint32_t location;
        uint32_t arraySize;

        bool IsTexture() const
        {
            return type == ShaderDataType::Texture2D || type == ShaderDataType::Texture1D || type == ShaderDataType::TextureCube;
        }
    };

    class CommandBuffer;

    class Shader
    {
        public:

            virtual ~Shader() = default;

            virtual void Bind(CommandBuffer& cmd) = 0;
            virtual void Unbind() = 0;

            virtual std::vector<UniformInfo> GetActiveUniforms() = 0;

            virtual void SetBool(const std::string& name, bool value) = 0;
            virtual void SetInt(const std::string& name, int value) = 0;
            virtual void SetFloat(const std::string& name, float value) = 0;

            virtual void SetVec2(const std::string& name, const glm::vec2& value) = 0;
            virtual void SetVec3(const std::string& name, const glm::vec3& value) = 0;
            virtual void SetVec4(const std::string& name, const glm::vec4& value) = 0;

            virtual void SetMat2(const std::string& name, const glm::mat2& mat) = 0;
            virtual void SetMat3(const std::string& name, const glm::mat3& mat) = 0;
            virtual void SetMat4(const std::string& name, const glm::mat4& mat) = 0;

            static std::shared_ptr<Shader> Create(
                const Filesystem::Path& vertexPath,
                const Filesystem::Path& fragmentPath,
                const Filesystem::Path& geometryPath = Filesystem::Path(""));


            void SetAssetID(Filesystem::AssetID id)
            {
                m_AssetID = id;
            }

        protected:
        
            Filesystem::AssetID m_AssetID;

            std::string m_VertexFilePath;
            std::string m_FragmentFilePath;
            std::string m_GeometryFilePath;
    };

}