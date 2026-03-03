#include "transform.hpp"


#include <glm/gtx/matrix_decompose.hpp>

#include "engine/rendering/renderer/renderer.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/ecs/components/rendering/light_component.hpp"

#include "engine/core/engine.hpp"

#include "transform.reflection.hpp"

namespace Pulse::Engine::ECS::Components{

	bool Transform::IsDirty(DirtyFlags flag) const
	{
		return dirtyFlags & flag;
	}

	void Transform::ClearDirty(DirtyFlags flag)
	{
		dirtyFlags = static_cast<DirtyFlags>(
			static_cast<uint8_t>(dirtyFlags) &
			~static_cast<uint8_t>(flag)
		);
	}

    Transform::Transform(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
        this->position = glm::vec3(0,0,0);
        this->rotation = glm::quat(glm::vec3(0,0,0));
        this->scale = glm::vec3(1,1,1);
    }

    void Transform::Deserialize(json componentData)
    {
		SetPosition(glm::vec3(componentData["position"]["x"], componentData["position"]["y"], componentData["position"]["z"]));
        SetRotation(glm::vec3(componentData["rotation"]["x"], componentData["rotation"]["y"], componentData["rotation"]["z"]));
        SetScale(glm::vec3(componentData["scale"]["x"], componentData["scale"]["y"], componentData["scale"]["z"]));
    }

    ordered_json Transform::Serialize()
    {
        ordered_json comp;

        comp["type"] = "transform";

        comp["active"] = activated;

		comp["position"]["x"] = position.x;
		comp["position"]["y"] = position.y;
		comp["position"]["z"] = position.z;

		comp["rotation"]["x"] = GetRotation().x;
		comp["rotation"]["y"] = GetRotation().y;
		comp["rotation"]["z"] = GetRotation().z;
		
		comp["scale"]["x"] = scale.x;
		comp["scale"]["y"] = scale.y;
		comp["scale"]["z"] = scale.z;

		return comp;
    }

    void Transform::SetPosition(glm::vec3 position, bool updateDirty)
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

		if(updateDirty)
			dirtyFlags |= DirtyFlags::Position;
    }

    void Transform::SetRotation(glm::vec3 rotation, bool updateDirty)
    {
        if(!activated)
            return;

		this->rotation = glm::quat(glm::radians(rotation));
		
		if(parent->HasComponent<Light>()){
			for(auto& light : parent->GetComponents<Light>()){
				light->SetDirection(this->GetForward());
			}
		}
		UpdateMeshReferencesInLevel();

		if(updateDirty)
			dirtyFlags |= DirtyFlags::Rotation;
    }

    void Transform::SetRotation(glm::quat rotation, bool updateDirty)
    {
		if(!activated)
            return;

		this->rotation = rotation;
		
		if(parent->HasComponent<Light>()){
			for(auto& light : parent->GetComponents<Light>()){
				light->SetDirection(this->GetForward());
			}
		}
		UpdateMeshReferencesInLevel();

		if(updateDirty)
			dirtyFlags |= DirtyFlags::Rotation;
    }

    void Transform::SetScale(glm::vec3 scale, bool updateDirty)
    {
        if(!activated)
            return;

        this->scale = scale;
		UpdateMeshReferencesInLevel();

		if(updateDirty)
			dirtyFlags |= DirtyFlags::Scale;
    }

    void Transform::Translate(glm::vec3 deltaPosition, bool updateDirty)
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

		if(updateDirty)
			dirtyFlags |= DirtyFlags::Position;
    }

    void Transform::Rotate(glm::vec3 angle, bool updateDirty)
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
				light->SetDirection(this->GetForward());
			}
		}
		UpdateMeshReferencesInLevel();

		if(updateDirty)
			dirtyFlags |= DirtyFlags::Rotation;
    }

    void Transform::Scale(glm::vec3 deltaScale, bool updateDirty)
    {
        if(!activated)
            return;

        this->scale += deltaScale;
		
		UpdateMeshReferencesInLevel();
		
		if(updateDirty)
			dirtyFlags |= DirtyFlags::Scale;
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
			this->SetPosition(glm::vec3(localMatrix[3]));
		}
		
		localMatrix[3] = glm::vec4(0, 0, 0, localMatrix[3].w);

		glm::vec3 Row[3], Pdum3;

		for (glm::length_t i = 0; i < 3; ++i)
			for (glm::length_t j = 0; j < 3; ++j)
				Row[i][j] = localMatrix[i][j];

		glm::vec3 sca;

		sca.x = length(Row[0]);
		Row[0] = glm::detail::scale(Row[0], static_cast<T>(1));
		sca.y = length(Row[1]);
		Row[1] = glm::detail::scale(Row[1], static_cast<T>(1));
		sca.z = length(Row[2]);
		Row[2] = glm::detail::scale(Row[2], static_cast<T>(1));

		SetScale(sca);

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

        glm::mat3 rotationMatrix;
		rotationMatrix[0] = Row[0];
		rotationMatrix[1] = Row[1];
		rotationMatrix[2] = Row[2];

		glm::quat q = glm::quat_cast(rotationMatrix);

		if(q != this->rotation){
			SetRotation(q);
		}
		
		
		UpdateMeshReferencesInLevel();

		return true;
    }

    std::shared_ptr<Component> Transform::Clone() const
    {
        auto cloned = std::make_shared<Transform>(*this);

        return cloned;
    }

    void Transform::OnFieldChanged(const FieldChangedEvent &event)
    {
		if(event.field->name == "position"){
			SetPosition(position);
		}
		else if(event.field->name == "rotation"){
			SetRotation(rotation);
		}
		else if(event.field->name == "scale"){
			SetScale(scale);
		}
    }
}
