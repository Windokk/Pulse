#pragma once

#include "engine/rendering/utils.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Pulse::Engine::Rendering::UI
{
    
    struct Character {
		unsigned int textureID; // ID handle of the glyph texture
		glm::ivec2   size;      // Size of glyph
		glm::ivec2   bearing;   // Offset from baseline to left/top of glyph
		unsigned int advance;   // Horizontal offset to advance to next glyph
	};

    class Font{
        
        std::unordered_map<GLchar, Character> characters;
        unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
        unsigned int VAO, VBO, EBO;
        int size;
        std::string fontData;
        
        public:
        Font(const char* path, int _size);
        void Cleanup();

        int GetSize() const { return size; };
        int GetVAO() const { return VAO; };
        int GetVBO() const { return VBO; };
        int GetEBO() const { return EBO; };

        std::unordered_map<GLchar, Character> GetCharacters() const { return characters; };
    };

}