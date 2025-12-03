#pragma once

#include "engine/rendering/utils.hpp"

#include "engine/filesystem/filesystem.hpp"

#include <string>
#include <iostream>

namespace Pulse::Engine::Rendering {

    struct UniformInfo {
        std::string name;
        GLenum type;
        GLint location;
        GLint arraySize;
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

            void setBool(const std::string& name, bool value) const;
            
            void setInt(const std::string& name, int value) const;
            
            void setFloat(const std::string& name, float value) const;
            
            void setVec2(const std::string& name, const glm::vec2& value) const;

            void setVec2(const std::string& name, float x, float y) const;

            void setVec3(const std::string& name, const glm::vec3& value) const;

            void setVec3(const std::string& name, float x, float y, float z) const;
            
            void setVec4(const std::string& name, const glm::vec4& value) const;

            void setVec4(const std::string& name, float x, float y, float z, float w) const;
            
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