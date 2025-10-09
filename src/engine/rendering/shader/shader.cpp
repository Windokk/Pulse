#include "shader.hpp"

#include "engine/debugging/debugger.hpp"

#include <stdexcept>
#include <iostream>


namespace Epoch::Engine::Rendering {

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
            GLuint vertexShader = GetGL().CreateShader(GL_VERTEX_SHADER);
            // Attach Vertex Shader source to the Vertex Shader Object
            GetGL().ShaderSource(vertexShader, 1, &vertexSource, NULL);
            // Compile the Vertex Shader into machine code
            GetGL().CompileShader(vertexShader);
            // Checks if Shader compiled succesfully
            CompileErrors(vertexShader, "VERTEX");
    
            // Create Fragment Shader Object and get its reference
            GLuint fragmentShader = GetGL().CreateShader(GL_FRAGMENT_SHADER);
            // Attach Fragment Shader source to the Fragment Shader Object
            GetGL().ShaderSource(fragmentShader, 1, &fragmentSource, NULL);
            // Compile the Vertex Shader into machine code
            GetGL().CompileShader(fragmentShader);
            // Checks if Shader compiled succesfully
            CompileErrors(fragmentShader, "FRAGMENT");
    
            GLuint geometryShader = 0;
            bool hasGeometry = !geometryFilePath.full.empty();

            if (hasGeometry) {
                this->geometryFilePath = geometryFilePath.full;
                std::string geometryCode = geometryFilePath.ReadFile();
                const char* geometrySource = geometryCode.c_str();

                geometryShader = GetGL().CreateShader(GL_GEOMETRY_SHADER);
                GetGL().ShaderSource(geometryShader, 1, &geometrySource, NULL);
                GetGL().CompileShader(geometryShader);
                CompileErrors(geometryShader, "GEOMETRY");
            }

            ID = GetGL().CreateProgram();
            GetGL().AttachShader(ID, vertexShader);
            GetGL().AttachShader(ID, fragmentShader);
            if (hasGeometry)
                GetGL().AttachShader(ID, geometryShader);

            GetGL().LinkProgram(ID);
            CompileErrors(ID, "PROGRAM");

            GetGL().DeleteShader(vertexShader);
            GetGL().DeleteShader(fragmentShader);
            if (hasGeometry)
                GetGL().DeleteShader(geometryShader);
        }
    }

    /// @brief Getter for the uniforms names and types of the shader
    /// @return A std::vector of uniforms
    std::vector<UniformInfo> Shader::GetActiveUniforms() {
        std::vector<UniformInfo> uniforms;

        GLint uniformCount;
        GetGL().GetProgramiv(ID, GL_ACTIVE_UNIFORMS, &uniformCount);
    
        GLint maxNameLength = 0;
        GetGL().GetProgramiv(ID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
    
        std::vector<char> nameData(maxNameLength);
    
        for (GLint i = 0; i < uniformCount; ++i) {
            GLsizei length = 0;
            GLint size = 0;
            GLenum type = 0;
    
            GetGL().GetActiveUniform(ID, i, maxNameLength, &length, &size, &type, nameData.data());

            std::string name(nameData.data(), length);
    
            GLint location = GetGL().GetUniformLocation(ID, name.c_str());
    
            if (name.find("gl_") == 0) continue;
    
            uniforms.push_back({ name, type, location });
        }
        
        return uniforms;
    }

    /// @brief Activates the Shader Program (bind)
    void Shader::Activate()
    {
        GetGL().UseProgram(ID);
    }

    /// @brief Deactivates the Shader Program (unbind)
    void Shader::Deactivate()
    {
        GetGL().UseProgram(0);
    }

    /// @brief Deletes the Shader Program
    void Shader::Cleanup()
    {
        GetGL().DeleteProgram(ID);
    }

    /// @brief Checks if the different Shaders have compiled properly
    void Shader::CompileErrors(unsigned int shader, const char* type)
    {
        // Stores status of compilation
        GLint hasCompiled;
        // Character array to store error message in
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            GetGL().GetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
            if (hasCompiled == GL_FALSE)
            {
                GetGL().GetShaderInfoLog(shader, 1024, NULL, infoLog);
                DEBUG_ERROR("Couldn't compile shader : " + std::string(type) + "\n" + infoLog);
                return;
            }
        }
        else
        {
            GetGL().GetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
            if (hasCompiled == GL_FALSE)
            {
                GetGL().GetProgramInfoLog(shader, 1024, NULL, infoLog);
                DEBUG_ERROR("Couldn't link shader : " + std::string(type) + "\n" + infoLog);
                return;
            } 
        }
    }
}


