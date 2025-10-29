#include "transform.hpp"

#include <glm/gtx/matrix_decompose.hpp>

#include "engine/rendering/renderer/renderer.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/ecs/components/rendering/light_component.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::ECS::Components{

    Transform::Transform(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
        this->position = glm::vec3(0,0,0);
        this->rotation = glm::quat(glm::vec3(0,0,0));
        this->scale = glm::vec3(1,1,1);
    }

    void Transform::SetPosition(glm::vec3 position)
    {
        if(!activated)
            return;

		glm::vec3 oldpos = this->position;
        this->position = position;

		if(parent->HasComponent<Light>()){
			for(auto& light : parent->GetComponents<Light>()){
				light->SetPosition(glm::vec3(this->position.x, this->position.y, this->position.z));
			}
		}

		if(oldpos != position){
			Core::GetEngine().GetRenderer()->ReorderDrawList();
		}
		UpdateMeshReferencesInLevel();
    }

    void Transform::SetRotation(glm::vec3 rotation)
    {
        if(!activated)
            return;

		this->rotation = glm::quat(glm::radians(rotation));
		
		if(parent->HasComponent<Light>()){
			for(auto& light : parent->GetComponents<Light>()){
				light->SetDirection(-this->GetForward());
			}
		}
		UpdateMeshReferencesInLevel();
    }

    void Transform::SetRotation(glm::quat rotation)
    {
		if(!activated)
            return;

		this->rotation = rotation;
		
		if(parent->HasComponent<Light>()){
			for(auto& light : parent->GetComponents<Light>()){
				light->SetDirection(-this->GetForward());
			}
		}
		UpdateMeshReferencesInLevel();
    }

    void Transform::SetScale(glm::vec3 scale)
    {
        if(!activated)
            return;

        this->scale = scale;
		UpdateMeshReferencesInLevel();
    }

    void Transform::Translate(glm::vec3 deltaPosition)
    {
        if(!activated)
            return;

        this->position += deltaPosition;

		if(parent->HasComponent<Light>()){
			for(auto& light : parent->GetComponents<Light>()){
				light->SetPosition(glm::vec3(this->position.x, this->position.y, this->position.z));
			}
		}
		
		if(deltaPosition.z != 0){
			Core::GetEngine().GetRenderer()->ReorderDrawList();
		} 
		UpdateMeshReferencesInLevel();
    }

    void Transform::Rotate(glm::vec3 angle)
    {
        if(!activated)
            return;

        glm::vec3 radians = glm::radians(angle);

		glm::quat qPitch = glm::angleAxis(radians.x, glm::vec3(1, 0, 0));
		glm::quat qYaw   = glm::angleAxis(radians.y, glm::vec3(0, 1, 0));
		glm::quat qRoll  = glm::angleAxis(radians.z, glm::vec3(0, 0, 1));

		this->rotation = qYaw * qPitch * qRoll * this->rotation;
		
		if(parent->HasComponent<Light>()){
			for(auto& light : parent->GetComponents<Light>()){
				light->SetDirection(-this->GetForward());
			}
		}
		UpdateMeshReferencesInLevel();
    }

    void Transform::Scale(glm::vec3 deltaScale)
    {
        if(!activated)
            return;

        this->scale += deltaScale;
		UpdateMeshReferencesInLevel();
    }

	void Transform::UpdateMeshReferencesInLevel()
	{
        if(!activated)
            return;

		if(parent->HasComponent<ECS::Components::Model>()){
			for(std::shared_ptr<ECS::Components::Model> comp : parent->GetComponents<ECS::Components::Model>()){
				comp->UpdateReferenceInLevel();
			}
		}
	}

    glm::mat4 Transform::GetTransformMatrix()
    {
        return glm::translate(glm::mat4(1.0f), position)
            * glm::toMat4(rotation)
            * glm::scale(glm::mat4(1.0f), scale);
    }

    bool Transform::SetFromTransformMatrix(const glm::mat4& m)
    {
        if(!activated)
            return false;

		using T = float;

		glm::mat4 localMatrix(m);

		if (glm::epsilonEqual(localMatrix[3][3], static_cast<float>(0), glm::epsilon<T>()))
			return false;

		if (
			glm::epsilonNotEqual(localMatrix[0][3], static_cast<T>(0), glm::epsilon<T>()) ||
			glm::epsilonNotEqual(localMatrix[1][3], static_cast<T>(0), glm::epsilon<T>()) ||
			glm::epsilonNotEqual(localMatrix[2][3], static_cast<T>(0), glm::epsilon<T>()))
		{
			// Clear the perspective partition
			localMatrix[0][3] = localMatrix[1][3] = localMatrix[2][3] = static_cast<T>(0);
			localMatrix[3][3] = static_cast<T>(1);
		}

		if(glm::vec3(localMatrix[3]) != this->position){
			position = glm::vec3(localMatrix[3]);

			if(parent->HasComponent<Light>()){
				for(auto& light : parent->GetComponents<Light>()){
					light->SetPosition(glm::vec3(this->position.x, this->position.y, this->position.z));
					
					Core::GetEngine().GetRenderer()->ReorderDrawList();
				}
			}
		}
		

		localMatrix[3] = glm::vec4(0, 0, 0, localMatrix[3].w);

		glm::vec3 Row[3], Pdum3;

		for (glm::length_t i = 0; i < 3; ++i)
			for (glm::length_t j = 0; j < 3; ++j)
				Row[i][j] = localMatrix[i][j];

		scale.x = length(Row[0]);
		Row[0] = glm::detail::scale(Row[0], static_cast<T>(1));
		scale.y = length(Row[1]);
		Row[1] = glm::detail::scale(Row[1], static_cast<T>(1));
		scale.z = length(Row[2]);
		Row[2] = glm::detail::scale(Row[2], static_cast<T>(1));

#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

        glm::vec3 localRot;

		localRot.y = asin(-Row[0][2]);
		if (cos(localRot.y) != 0) {
			localRot.x = atan2(Row[1][2], Row[2][2]);
			localRot.z = atan2(Row[0][1], Row[0][0]);
		}
		else {
			localRot.x = atan2(-Row[2][0], Row[1][1]);
			localRot.z = 0;
		}

		if(glm::quat(localRot) != this->rotation){
			rotation = glm::quat(localRot);

			if(parent->HasComponent<Light>()){
				for(auto& light : parent->GetComponents<Light>()){
					light->SetDirection(-this->GetForward());
				}
			}
		}
		
		UpdateMeshReferencesInLevel();

		return true;
    }

    std::shared_ptr<Component> Transform::Clone() const
    {
        auto cloned = std::make_shared<Transform>(*this);

        return cloned;
    }
}
