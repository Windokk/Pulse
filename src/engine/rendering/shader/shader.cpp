#include "shader.hpp"

#include "engine/debugging/logger.hpp"

#include <stdexcept>
#include <iostream>

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering {

    using namespace Filesystem;

    bool IsSamplerType(GLenum type) {
        return type == GL_SAMPLER_1D || type == GL_SAMPLER_2D ||
               type == GL_SAMPLER_3D || type == GL_SAMPLER_CUBE ||
               type == GL_SAMPLER_1D_SHADOW || type == GL_SAMPLER_2D_SHADOW ||
               type == GL_SAMPLER_1D_ARRAY || type == GL_SAMPLER_2D_ARRAY ||
               type == GL_SAMPLER_1D_ARRAY_SHADOW || type == GL_SAMPLER_2D_ARRAY_SHADOW ||
               type == GL_SAMPLER_2D_MULTISAMPLE || type == GL_SAMPLER_2D_MULTISAMPLE_ARRAY ||
               type == GL_SAMPLER_CUBE_SHADOW || type == GL_SAMPLER_BUFFER ||
               type == GL_SAMPLER_2D_RECT || type == GL_SAMPLER_2D_RECT_SHADOW;
    }

    /// @brief Constructor that build a Shader Program from 2 (vert and frag) or 3 (vert, frag and geom) different shaders path
    Shader::Shader(const Path vertexFilePath, const Path fragmentFilePath, const Path geometryFilePath)
    {
        if (vertexFilePath.full != "" && fragmentFilePath.full != "") {
		
            this->vertexFilePath = vertexFilePath.full;
            this->fragmentFilePath = fragmentFilePath.full;

            // Read vertexFile and fragmentFile and store the strings
            std::string vertexCode = vertexFilePath.ReadFile();
            std::string fragmentCode = fragmentFilePath.ReadFile();
    
            // Convert the shader source strings into character arrays
            const char* vertexSource = vertexCode.c_str();
            const char* fragmentSource = fragmentCode.c_str();
    
            // Create Vertex Shader Object and get its reference
            GLuint vertexShader = Core::GetEngine().GetGL()->CreateShader(GL_VERTEX_SHADER);
            // Attach Vertex Shader source to the Vertex Shader Object
            Core::GetEngine().GetGL()->ShaderSource(vertexShader, 1, &vertexSource, NULL);
            // Compile the Vertex Shader into machine code
            Core::GetEngine().GetGL()->CompileShader(vertexShader);
            // Checks if Shader compiled succesfully
            CompileErrors(vertexShader, "VERTEX");
    
            // Create Fragment Shader Object and get its reference
            GLuint fragmentShader = Core::GetEngine().GetGL()->CreateShader(GL_FRAGMENT_SHADER);
            // Attach Fragment Shader source to the Fragment Shader Object
            Core::GetEngine().GetGL()->ShaderSource(fragmentShader, 1, &fragmentSource, NULL);
            // Compile the Vertex Shader into machine code
            Core::GetEngine().GetGL()->CompileShader(fragmentShader);
            // Checks if Shader compiled succesfully
            CompileErrors(fragmentShader, "FRAGMENT");
    
            GLuint geometryShader = 0;
            bool hasGeometry = !geometryFilePath.full.empty();

            if (hasGeometry) {
                this->geometryFilePath = geometryFilePath.full;
                std::string geometryCode = geometryFilePath.ReadFile();
                const char* geometrySource = geometryCode.c_str();

                geometryShader = Core::GetEngine().GetGL()->CreateShader(GL_GEOMETRY_SHADER);
                Core::GetEngine().GetGL()->ShaderSource(geometryShader, 1, &geometrySource, NULL);
                Core::GetEngine().GetGL()->CompileShader(geometryShader);
                CompileErrors(geometryShader, "GEOMETRY");
            }

            ID = Core::GetEngine().GetGL()->CreateProgram();
            Core::GetEngine().GetGL()->AttachShader(ID, vertexShader);
            Core::GetEngine().GetGL()->AttachShader(ID, fragmentShader);
            if (hasGeometry)
                Core::GetEngine().GetGL()->AttachShader(ID, geometryShader);

            Core::GetEngine().GetGL()->LinkProgram(ID);
            CompileErrors(ID, "PROGRAM");

            Core::GetEngine().GetGL()->DeleteShader(vertexShader);
            Core::GetEngine().GetGL()->DeleteShader(fragmentShader);
            if (hasGeometry)
                Core::GetEngine().GetGL()->DeleteShader(geometryShader);
        }
    }

    /// @brief Getter for the uniforms names and types of the shader
    /// @return A std::vector of uniforms
    std::vector<UniformInfo> Shader::GetActiveUniforms() {
        std::vector<UniformInfo> uniforms;

        GLint uniformCount;
        Core::GetEngine().GetGL()->GetProgramiv(ID, GL_ACTIVE_UNIFORMS, &uniformCount);
    
        GLint maxNameLength = 0;
        Core::GetEngine().GetGL()->GetProgramiv(ID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
    
        std::vector<char> nameData(maxNameLength);
    
        for (GLint i = 0; i < uniformCount; ++i) {
            GLsizei length = 0;
            GLint size = 0;
            GLenum type = 0;
    
            Core::GetEngine().GetGL()->GetActiveUniform(ID, i, maxNameLength, &length, &size, &type, nameData.data());

            std::string name(nameData.data(), length);
    
            GLint location = Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str());
    
            if (name.find("gl_") == 0) continue;

            switch(tagLC_ID}
    
            uniforms.push_back({ name, type, location, size });
        }
        
        return uniforms;
    }

    /// @brief Activates the Shader Program (bind)
    void Shader::Activate()
    {
        Core::GetEngine().GetGL()->UseProgram(ID);
    }

    /// @brief Deactivates the Shader Program (unbind)
    void Shader::Deactivate()
    {
        Core::GetEngine().GetGL()->UseProgram(0);
    }

    /// @brief Deletes the Shader Program
    void Shader::Cleanup()
    {
        Core::GetEngine().GetGL()->DeleteProgram(ID);
    }

    void Shader::setBool(const std::string &name, bool value) const
    {
        Core::GetEngine().GetGL()->Uniform1i(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), (int)value);
    }

    void Shader::setInt(const std::string &name, int value) const
    {
        Core::GetEngine().GetGL()->Uniform1i(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), value);
    }

    void Shader::setFloat(const std::string &name, float value) const
    {
        Core::GetEngine().GetGL()->Uniform1f(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), value);
    }

    void Shader::setVec2(const std::string &name, const glm::vec2 &value) const
    {
        Core::GetEngine().GetGL()->Uniform2fv(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec2(const std::string &name, float x, float y) const
    {
        Core::GetEngine().GetGL()->Uniform2f(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), x, y);
    }

    void Shader::setVec3(const std::string &name, const glm::vec3 &value) const
    {
        Core::GetEngine().GetGL()->Uniform3fv(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec3(const std::string &name, float x, float y, float z) const
    {
        Core::GetEngine().GetGL()->Uniform3f(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void Shader::setVec4(const std::string &name, const glm::vec4 &value) const
    {
        Core::GetEngine().GetGL()->Uniform4fv(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec4(const std::string &name, float x, float y, float z, float w) const
    {
        Core::GetEngine().GetGL()->Uniform4f(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

    void Shader::setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        Core::GetEngine().GetGL()->UniformMatrix2fv(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        Core::GetEngine().GetGL()->UniformMatrix3fv(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        Core::GetEngine().GetGL()->UniformMatrix4fv(Core::GetEngine().GetGL()->GetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    /// @brief Checks if the different shaders have compiled properly
    void Shader::CompileErrors(unsigned int shader, const char* type)
    {
        // Stores status of compilation
        GLint hasCompiled;
        // Character array to store error message in
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            Core::GetEngine().GetGL()->GetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
            if (hasCompiled == GL_FALSE)
            {
                Core::GetEngine().GetGL()->GetShaderInfoLog(shader, 1024, NULL, infoLog);
                DEBUG_ERROR("Couldn't compile shader : " + std::string(type) + "\n" + infoLog);
                return;
            }
        }
        else
        {
            Core::GetEngine().GetGL()->GetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
            if (hasCompiled == GL_FALSE)
            {
                Core::GetEngine().GetGL()->GetProgramInfoLog(shader, 1024, NULL, infoLog);
                DEBUG_ERROR("Couldn't link shader : " + std::string(type) + "\n" + infoLog);
                return;
            } 
        }
    }
}


