#pragma once

#include "engine/rendering/utils.hpp"

#include "engine/filesystem/filesystem.hpp"

#include <string>
#include <iostream>

namespace Pulse::Engine::Rendering {

    struct UniformInfo {
        std::string name;
        GLenum glType;
        GLint location;
        GLint arraySize;

        bool IsTexture() const {
            switch (glType) {
                case GL_TEXTURE_1D:
                case GL_TEXTURE_2D:
                case GL_TEXTURE_3D:

                case GL_TEXTURE_CUBE_MAP:
                case GL_TEXTURE_RECTANGLE:

                case GL_TEXTURE_1D_ARRAY:
                case GL_TEXTURE_2D_ARRAY:
                case GL_TEXTURE_CUBE_MAP_ARRAY:

                case GL_TEXTURE_BUFFER:
                case GL_TEXTURE_2D_MULTISAMPLE:
                case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                    return true;

                default:
                    return false;
            }
        }
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

            // Scalar
            void setBool(const std::string& name, bool value) const;
            void setInt(const std::string& name, int value) const;
            void setFloat(const std::string& name, float value) const;
            void setBoolLoc(GLint location, bool value) const;
            void setIntLoc(GLint location, int value) const;
            void setFloatLoc(GLint location, float value) const;
            
            // Vectors
            void setVec2(const std::string& name, const glm::vec2& value) const;
            void setVec2(const std::string& name, float x, float y) const;
            void setVec3(const std::string& name, const glm::vec3& value) const;
            void setVec3(const std::string& name, float x, float y, float z) const;
            void setVec4(const std::string& name, const glm::vec4& value) const;
            void setVec4(const std::string& name, float x, float y, float z, float w) const;
            void setVec2Loc(GLint location, const glm::vec2& value) const;
            void setVec2Loc(GLint location, float x, float y) const;
            void setVec3Loc(GLint location, const glm::vec3& value) const;
            void setVec3Loc(GLint location, float x, float y, float z) const;
            void setVec4Loc(GLint location, const glm::vec4& value) const;
            void setVec4Loc(GLint location, float x, float y, float z, float w) const;
            
            // Matrices
            void setMat2Loc(GLint location, const glm::mat2& mat) const;
            void setMat3Loc(GLint location, const glm::mat3& mat) const;
            void setMat4Loc(GLint location, const glm::mat4& mat) const;
            void setMat2(const std::string& name, const glm::mat2& mat) const;
            void setMat3(const std::string& name, const glm::mat3& mat) const;
            void setMat4(const std::string& name, const glm::mat4& mat) const;

            std::string fragmentFilePath;

            Filesystem::AssetID assetID;

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

        private:
            void CompileErrors(unsigned int shader, const char* type);
        
            std::string vertexFilePath;
            std::string geometryFilePath;
    };

}