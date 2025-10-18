#include "font.hpp"

#include "engine/filesystem/filesystem.hpp"
#include "engine/debugging/debugger.hpp"

#include "engine/core/engine.hpp"

namespace Epoch::Engine::Rendering::UI{

    using namespace Filesystem;
    
    Font::Font(const char *path, int size)
    {
        this->size = size;
    
        FT_Library ft;
        if (FT_Init_FreeType(&ft))
        {
            DEBUG_ERROR("Could not init FreeType Library");
            return;
        }
        
        // load font as face
        FT_Face face;
        FT_Error error;
        Path font_path = Path(path);

        if (font_path.Exists()) {
            fontData = font_path.ReadFile();
            if (fontData.empty()) {
                DEBUG_ERROR("Font file is empty or could not be read: " + font_path.full);
                return;
            }

            error = FT_New_Memory_Face(
                ft,                                              // FreeType library instance
                reinterpret_cast<const FT_Byte*>(fontData.data()), // Cast is necessary
                static_cast<FT_Long>(fontData.size()),                                    // Size of the font data
                0,                                               // Face index (for font collections, typically 0)
                &face                                            // Output face object
            );

            if (error || !face) {
                DEBUG_ERROR("FT_New_Memory_Face failed for: " + std::string(path));
                return;
            }
    
        } else {
            DEBUG_ERROR("Couldn't find font : " + std::string(path));
            return;
        }
    
        if (error) {
            DEBUG_ERROR("Couldn't load font : " + std::string(path));
            return;
        }
        else {
            // set size to load glyphs as
            FT_Set_Char_Size(face, 0, size * 64, 300, 300);
    
            // disable byte-alignment restriction
            Core::GetEngine().GetGL()->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
            // load first 128 characters of ASCII set
            for (unsigned char c = 0; c < 128; c++)
            {
                // Load character glyph 
                if (FT_Error err = FT_Load_Char(face, c, FT_LOAD_RENDER))
                {
                    DEBUG_ERROR("Couldn't load glyph (Freetype error : "+std::to_string(err)+")");
                    continue;
                }
                // generate texture
                unsigned int texture;
                Core::GetEngine().GetGL()->GenTextures(1, &texture);
                Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, texture);
                Core::GetEngine().GetGL()->TexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    face->glyph->bitmap.buffer
                );
                // set texture options
                Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                // now store character for later use
                Character character = {
                    texture,
                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    static_cast<unsigned int>(face->glyph->advance.x)
                };
                characters.insert(std::pair<char, Character>(c, character));
            }
            Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, 0);
        }
        // destroy FreeType once we're finished
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    
    
        // configure VAO/VBO for texture quads
        // -----------------------------------
        Core::GetEngine().GetGL()->GenVertexArrays(1, &VAO);
        Core::GetEngine().GetGL()->GenBuffers(1, &VBO);
        Core::GetEngine().GetGL()->BindVertexArray(VAO);
        Core::GetEngine().GetGL()->BindBuffer(GL_ARRAY_BUFFER, VBO);
        Core::GetEngine().GetGL()->BufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
        Core::GetEngine().GetGL()->GenBuffers(1, &EBO);
        Core::GetEngine().GetGL()->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        Core::GetEngine().GetGL()->BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        // Position attribute (location = 0)
        Core::GetEngine().GetGL()->VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        Core::GetEngine().GetGL()->EnableVertexAttribArray(0);

        Core::GetEngine().GetGL()->VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        Core::GetEngine().GetGL()->EnableVertexAttribArray(1);

        Core::GetEngine().GetGL()->VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        Core::GetEngine().GetGL()->EnableVertexAttribArray(2);

        Core::GetEngine().GetGL()->VertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        Core::GetEngine().GetGL()->EnableVertexAttribArray(3);

        Core::GetEngine().GetGL()->VertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        Core::GetEngine().GetGL()->EnableVertexAttribArray(4);
        Core::GetEngine().GetGL()->BindBuffer(GL_ARRAY_BUFFER, 0);
        Core::GetEngine().GetGL()->BindVertexArray(0);
    }

    void Font::Cleanup(){
        for (auto& [c, character] : characters) {
            Core::GetEngine().GetGL()->DeleteTextures(1, &character.textureID);
        }
        characters.clear();
    
        // Supprimer les buffers OpenGL
        if (VBO) {
            Core::GetEngine().GetGL()->DeleteBuffers(1, &VBO);
            VBO = 0;
        }
    
        if (EBO) {
            Core::GetEngine().GetGL()->DeleteBuffers(1, &EBO);
            EBO = 0;
        }
    
        if (VAO) {
            Core::GetEngine().GetGL()->DeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
    }
}