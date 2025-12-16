#include "material_serializer.hpp"

#include "engine/core/resources/resources_manager.hpp"

#include "engine/core/engine.hpp"

#include <nlohmann/json.hpp>


using namespace nlohmann;

namespace Pulse::Engine::Serialization{

    using namespace Core::Resources;
    using namespace Rendering;

    std::shared_ptr<Material> DeserializeMaterial(Filesystem::Path path)
    {
        if(!path.Exists()){
            DEBUG_ERROR("Level at path : \"" + path.full +"\" doesn't exist !");
            return nullptr; 
        }

        std::string src = path.ReadFile();

        try {
            json data = json::parse(src);

            std::shared_ptr<Shader> shader = Core::GetEngine().GetResourcesManager()->GetShader(data["shader"]);

            if(!shader){
                DEBUG_ERROR("No shader found : " + ((std::string)data["shader"]));
                return nullptr;
            }

            if(!data.contains("recievesShadows")){
                DEBUG_ERROR("Material doesn't specify required field : recievesShadows");
                return nullptr;
            }
            
            if (!data.contains("renderMode") || !data["renderMode"].is_string()) {
                DEBUG_ERROR("Material doesn't specify required field: renderMode");
                return nullptr;
            }

            const std::string& mode = data["renderMode"];
            if (mode != "opaque" && mode != "translucent" && mode != "masked") {
                DEBUG_ERROR("Unknown value \"" + mode + "\" for field \"renderMode\"");
                return nullptr;
            }

            RenderMode renderMode = OPAQUE;
            if(mode == "translucent")
                renderMode = TRANSLUCENT;
            else if(mode == "masked")
                renderMode = MASKED;
            
            std::shared_ptr<Material> mat = std::make_shared<Material>(shader, data["recievesShadows"], renderMode);

            for(auto& uniform : data["uniforms"]){
                for (auto it = uniform.begin(); it != uniform.end(); ++it) {
                    const std::string& name = it.key();
                    const auto& value = it.value();

                    // Texture
                    if (value.is_string()) {
                        GLuint tex = Core::GetEngine().GetResourcesManager()->GetImage(value.get<std::string>())->texture->GetID();
                        if(tex){
                            mat->SetTextureParameter(name, tex);
                        }
                        else{
                            DEBUG_ERROR("No texture found : " + value.get<std::string>());
                            return nullptr;
                        }
                    }
                    else if(value.is_boolean()){
                        mat->SetScalarParameter(name, value.get<bool>());
                    }
                    // Float
                    else if (value.is_number_float()) {
                        mat->SetScalarParameter(name, value.get<float>());
                    }
                    // Int
                    else if (value.is_number_integer()) {
                        mat->SetScalarParameter(name, value.get<int>());
                    }
                    // Vectors
                    else if (value.is_array()) {
                        size_t len = value.size();
                        if (len == 2) {
                            mat->SetScalarParameter(name, glm::vec2(value[0].get<float>(), value[1].get<float>()));
                        }
                        else if (len == 3) {
                            mat->SetScalarParameter(name, glm::vec3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>()));
                        }
                        else if (len == 4) {
                            mat->SetScalarParameter(name, glm::vec4(value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value[3].get<float>()));
                        }
                        else if (len == 16) {
                            glm::mat4 mat4;
                            for (int i = 0; i < 16; ++i)
                                mat4[i / 4][i % 4] = value[i].get<float>();
                            mat->SetScalarParameter(name, mat4);
                        } else {
                            DEBUG_ERROR("Unknown array size for uniform: " + name);
                        }
                    }
                    else {
                        DEBUG_ERROR("Unsupported uniform value type for: " + name);
                    }
                }

                
            }

            return mat;

        } catch (const json::parse_error& e) {
            DEBUG_ERROR("JSON parse error: " + (std::string)e.what());
            return nullptr;
        }
    }
}

