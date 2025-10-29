#include "text.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering::UI
{
    Text::Text(std::shared_ptr<Font> font, std::string text, glm::vec4 color, Transform transform) : font(font), transform(transform) 
    {
        this->text = text;
        this->color = color;
    }

    void Text::Draw(std::shared_ptr<Shader> shader, glm::mat4 projection, glm::mat4 view){

        glm::vec2 cursor = glm::vec2(transform.GetPosition().x-transform.GetScale().x/2.f, transform.GetPosition().y-transform.GetScale().y/2.f);
        // activate shader and send uniforms
        shader->Activate();
        shader->setMat4("model", transform.GetTransformMatrix());
        shader->setMat4("view", view);
        shader->setMat4("projection", projection);
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindVertexArray(font->GetVAO());

        // iterate through all characters of the text
        std::string::const_iterator c;
        for (c = text.begin(); c != text.end(); c++) 
        {

            if(*c == '\n'){
                cursor.y -= font->GetSize();
                cursor.x = transform.GetPosition().x;
            }
            else{

                Character ch = font->GetCharacters()[*c];

                float xpos = cursor.x + ch.bearing.x * transform.GetScale().x;
                float ypos = cursor.y - (ch.size.y - ch.bearing.y) * transform.GetScale().y;

                float w = ch.size.x * transform.GetScale().x;
                float h = ch.size.y * transform.GetScale().y;
                // update VBO for each character
                glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
                std::vector<Vertex> vertices = {
                    { glm::vec3(xpos,     ypos + h, 0.0f), normal, color, glm::vec2(0.0f, 0.0f), glm::vec3(0) },
                    { glm::vec3(xpos,     ypos,     0.0f), normal, color, glm::vec2(0.0f, 1.0f), glm::vec3(0) },
                    { glm::vec3(xpos + w, ypos,     0.0f), normal, color, glm::vec2(1.0f, 1.0f), glm::vec3(0) },
                    { glm::vec3(xpos + w, ypos + h, 0.0f), normal, color, glm::vec2(1.0f, 0.0f), glm::vec3(0) }
                };
                // render glyph texture over quad
                Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, ch.textureID);
                // update content of VBO memory
                Core::GetEngine().GetGL()->BindBuffer(GL_ARRAY_BUFFER, font->GetVBO());
                Core::GetEngine().GetGL()->BufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
                Core::GetEngine().GetGL()->BindBuffer(GL_ARRAY_BUFFER, 0);
                // render quad
                Core::GetEngine().GetGL()->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
                cursor.x += (ch.advance >> 6) * transform.GetScale().x; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            }
        }
        Core::GetEngine().GetGL()->BindVertexArray(0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, 0);
        shader->Deactivate();
    }
}
