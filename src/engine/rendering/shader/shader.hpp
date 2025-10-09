#pragma once

#include "engine/rendering/utils.hpp"

#include "engine/filesystem/filesystem.hpp"

#include <string>
#include <iostream>

#include "engine/rendering/opengl/opengl.hpp"

namespace Epoch::Engine::Rendering {

    struct UniformInfo {
        std::string name;
        GLenum type;
        GLint location;
    };

    class Shader{
        
        public:
            // Reference ID of the Shader Program
            GLuint ID;
        
            Shader(const Filesystem::Path vertexFilePath = Filesystem::Path(""), const Filesystem::Path fragmentFilePath = Filesystem::Path(""), const Filesystem::Path geometryFilePath = Filesystem::Path(""));

            std::vector<UniformInfo> GetActiveUniforms();

            void Activate();
            void Deactivate();
            void Cleanup();

            void setBool(const std::string& name, bool value) const
            {
                GetGL().Uniform1i(GetGL().GetUniformLocation(ID, name.c_str()), (int)value);
            }
            // ------------------------------------------------------------------------
            void setInt(const std::string& name, int value) const
            {
                GetGL().Uniform1i(GetGL().GetUniformLocation(ID, name.c_str()), value);
            }
            // ------------------------------------------------------------------------
            void setFloat(const std::string& name, float value) const
            {
                GetGL().Uniform1f(GetGL().GetUniformLocation(ID, name.c_str()), value);
            }
            // ------------------------------------------------------------------------
            void setVec2(const std::string& name, const glm::vec2& value) const
            {
                GetGL().Uniform2fv(GetGL().GetUniformLocation(ID, name.c_str()), 1, &value[0]);
            }
            void setVec2(const std::string& name, float x, float y) const
            {
                GetGL().Uniform2f(GetGL().GetUniformLocation(ID, name.c_str()), x, y);
            }
            // ------------------------------------------------------------------------
            void setVec3(const std::string& name, const glm::vec3& value) const
            {
                GetGL().Uniform3fv(GetGL().GetUniformLocation(ID, name.c_str()), 1, &value[0]);
            }
            void setVec3(const std::string& name, float x, float y, float z) const
            {
                GetGL().Uniform3f(GetGL().GetUniformLocation(ID, name.c_str()), x, y, z);
            }
            // ------------------------------------------------------------------------
            void setVec4(const std::string& name, const glm::vec4& value) const
            {
                GetGL().Uniform4fv(GetGL().GetUniformLocation(ID, name.c_str()), 1, &value[0]);
            }
            void setVec4(const std::string& name, float x, float y, float z, float w) const
            {
                GetGL().Uniform4f(GetGL().GetUniformLocation(ID, name.c_str()), x, y, z, w);
            }
            // ------------------------------------------------------------------------
            void setMat2(const std::string& name, const glm::mat2& mat) const
            {
                GetGL().UniformMatrix2fv(GetGL().GetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
            }
            // ------------------------------------------------------------------------
            void setMat3(const std::string& name, const glm::mat3& mat) const
            {
                GetGL().UniformMatrix3fv(GetGL().GetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
            }
            // ------------------------------------------------------------------------
            void setMat4(const std::string& name, const glm::mat4& mat) const
            {
                GetGL().UniformMatrix4fv(GetGL().GetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
            }
            
            std::string fragmentFilePath;

        private:
            void CompileErrors(unsigned int shader, const char* type);
        
            std::string vertexFilePath;
            std::string geometryFilePath;
    };

}