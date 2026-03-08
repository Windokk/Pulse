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
            void SetBool(const std::string& name, bool value) const;
            void SetInt(const std::string& name, int value) const;
            void SetFloat(const std::string& name, float value) const;
            void SetBoolLoc(GLint location, bool value) const;
            void SetIntLoc(GLint location, int value) const;
            void SetFloatLoc(GLint location, float value) const;
            
            // Vectors
            void SetVec2(const std::string& name, const glm::vec2& value) const;
            void SetVec2(const std::string& name, float x, float y) const;
            void SetVec3(const std::string& name, const glm::vec3& value) const;
            void SetVec3(const std::string& name, float x, float y, float z) const;
            void SetVec4(const std::string& name, const glm::vec4& value) const;
            void SetVec4(const std::string& name, float x, float y, float z, float w) const;
            void SetVec2Loc(GLint location, const glm::vec2& value) const;
            void SetVec2Loc(GLint location, float x, float y) const;
            void SetVec3Loc(GLint location, const glm::vec3& value) const;
            void SetVec3Loc(GLint location, float x, float y, float z) const;
            void SetVec4Loc(GLint location, const glm::vec4& value) const;
            void SetVec4Loc(GLint location, float x, float y, float z, float w) const;
            
            // Matrices
            void SetMat2Loc(GLint location, const glm::mat2& mat) const;
            void SetMat3Loc(GLint location, const glm::mat3& mat) const;
            void SetMat4Loc(GLint location, const glm::mat4& mat) const;
            void SetMat2(const std::string& name, const glm::mat2& mat) const;
            void SetMat3(const std::string& name, const glm::mat3& mat) const;
            void SetMat4(const std::string& name, const glm::mat4& mat) const;

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