#pragma once

#include "engine/rendering/shader/shader.hpp"

namespace Pulse::Engine::Rendering{

    class GLShader : public Shader{
        public:
            GLShader(const Filesystem::Path &vertexPath, const Filesystem::Path &fragmentPath, const Filesystem::Path &geometryPath);

            ~GLShader() = default;

            void Bind(CommandBuffer& cmd)  override;
            void Unbind() override;

            std::vector<UniformInfo> GetActiveUniforms() override;

            void SetBool(const std::string& name, bool value) override;
            void SetInt(const std::string& name, int value) override;
            void SetFloat(const std::string& name, float value) override;

            void SetVec2(const std::string& name, const glm::vec2& value) override;
            void SetVec3(const std::string& name, const glm::vec3& value) override;
            void SetVec4(const std::string& name, const glm::vec4& value) override;

            void SetMat2(const std::string& name, const glm::mat2& mat) override;
            void SetMat3(const std::string& name, const glm::mat3& mat) override;
            void SetMat4(const std::string& name, const glm::mat4& mat) override;

        private:

            void CompileErrors(unsigned int shader, const char *type);

            uint32_t m_Program;
    };

}