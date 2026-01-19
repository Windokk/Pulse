#include "physics_manager.hpp"

#include "engine/levels/level_manager.hpp"

#include "engine/ecs/components/physics/physics_body.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Physics {

    // Constants
    static constexpr uint cMaxBodies = 1024;
    static constexpr uint cNumBodyMutexes = 0;
    static constexpr uint cMaxBodyPairs = 1024;
    static constexpr uint cMaxContactConstraints = 1024;
    static constexpr uint cTempAllocatorSize = 10 * 1024 * 1024;

    void PhysicsManager::Init(glm::vec3 gravity)
    {
        JPH::RegisterDefaultAllocator();

        JPH::Trace = &TraceImpl;

        JPH_IF_ENABLE_ASSERTS(
            JPH::AssertFailed = [](const char* inExpr, const char* inMsg, const char* inFile, uint inLine)
            {
                std::cerr << inFile << ":" << inLine << ": (" << inExpr << ") " << (inMsg ? inMsg : "") << std::endl;
                return true;
            };
        )

        // Required factory setup
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        // Allocator & job system
        m_tempAllocator = new JPH::TempAllocatorImpl(cTempAllocatorSize);
        m_jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        // Initialize physics system
        m_physicsSystem.Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            m_broadPhaseLayer,
            m_objectVsBroadphase,
            m_objectLayerFilter
        );
        

        m_physicsSystem.SetBodyActivationListener(&m_activationListener);
        m_physicsSystem.SetContactListener(&m_contactListener);
        m_physicsSystem.SetGravity(JPH::Vec3Arg(gravity.x, gravity.y, gravity.z));

        this->initialized = true;
    }

    void PhysicsManager::Shutdown()
    {
        delete m_jobSystem;
        m_jobSystem = nullptr;

        delete m_tempAllocator;
        m_tempAllocator = nullptr;

        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsManager::OnContactAdded(const Body &body1, const Body &body2, const ContactManifold &contactManifold, ContactSettings &contactSettings)
    {   
        ECS::Components::PhysicsBody* comp1 = bodyIDToComponentMap[body1.GetID()];
        ECS::Components::PhysicsBody* comp2 = bodyIDToComponentMap[body2.GetID()];

        if(!comp1 || !comp2)
            return;

        for(auto& script : comp1->parent->GetComponents<ECS::Components::Script>()){
            Core::GetEngine().GetEventDispatcher()->emitToComponent(
                script->parent->GetComponentIDInScene(script->GetLocalId()),
                Events::ContactAddedEvent(*comp2, contactManifold, contactSettings, ECS::ObjectID(0))
            );
        }

        for(auto& script : comp2->parent->GetComponents<ECS::Components::Script>()){
            Core::GetEngine().GetEventDispatcher()->emitToComponent(
                script->parent->GetComponentIDInScene(script->GetLocalId()),
                Events::ContactAddedEvent(*comp1, contactManifold, contactSettings, ECS::ObjectID(0))
            );
        }
    }

    void PhysicsManager::OnContactPersisted(const Body &body1, const Body &body2, const ContactManifold &contactManifold, ContactSettings &contactSettings)
    {
        ECS::Components::PhysicsBody* comp1 = bodyIDToComponentMap[body1.GetID()];
        ECS::Components::PhysicsBody* comp2 = bodyIDToComponentMap[body2.GetID()];

        if(!comp1 || !comp2)
            return;

        for(auto& script : comp1->parent->GetComponents<ECS::Components::Script>()){
            Core::GetEngine().GetEventDispatcher()->emitToComponent(
                script->parent->GetComponentIDInScene(script->GetLocalId()),
                Events::ContactPersistedEvent(*comp2, contactManifold, contactSettings, ECS::ObjectID(0))
            );
        }

        for(auto& script : comp2->parent->GetComponents<ECS::Components::Script>()){
            Core::GetEngine().GetEventDispatcher()->emitToComponent(
                script->parent->GetComponentIDInScene(script->GetLocalId()),
                Events::ContactPersistedEvent(*comp1, contactManifold, contactSettings, ECS::ObjectID(0))
            );
        }
    }
    
    void PhysicsManager::OnContactRemoved(const SubShapeIDPair &pair)
    {
        ECS::Components::PhysicsBody* comp1 = bodyIDToComponentMap[pair.GetBody1ID()];
        ECS::Components::PhysicsBody* comp2 = bodyIDToComponentMap[pair.GetBody2ID()];

        if(!comp1 || !comp2)
            return;

        for(auto& script : comp1->parent->GetComponents<ECS::Components::Script>()){
            Core::GetEngine().GetEventDispatcher()->emitToComponent(
                script->parent->GetComponentIDInScene(script->GetLocalId()),
                Events::ContactRemovedEvent(*comp2, ECS::ObjectID(0))
            );
        }

        for(auto& script : comp2->parent->GetComponents<ECS::Components::Script>()){
            Core::GetEngine().GetEventDispatcher()->emitToComponent(
                script->parent->GetComponentIDInScene(script->GetLocalId()),
                Events::ContactRemovedEvent(*comp1, ECS::ObjectID(0))
            );
        }

    }

    void PhysicsManager::StepSimulation(float deltaTime)
    {
        m_physicsSystem.Update(deltaTime, 1, m_tempAllocator, m_jobSystem);

        if(int levelCount = Core::GetEngine().GetLevelManager()->GetLoadedLevelCount() > 0){
                    for(int i = 0; i < levelCount; i++){
                for (auto& physicsBody : Core::GetEngine().GetLevelManager()->GetLevelAt(i)->physicsBodies){
                    physicsBody->Tick();
                }
            }
        }
    }

    JPH::BodyID PhysicsManager::CreateBody(const JPH::BodyCreationSettings &settings, ECS::Components::PhysicsBody *component, JPH::EActivation activation)
    {
        JPH::BodyInterface& bodyInterface = m_physicsSystem.GetBodyInterface();

        // Create body
        JPH::Body* body = bodyInterface.CreateBody(settings);
        if (body == nullptr)
        {
            DEBUG_ERROR("Failed to create body!");
            return JPH::BodyID();
        }

        // Add to worlds
        bodyInterface.AddBody(body->GetID(), activation);
        bodyIDToComponentMap.emplace(body->GetID(), component);
        return body->GetID();
    }

    void PhysicsManager::RemoveBody(JPH::BodyID id)
    {
        JPH::BodyInterface& bodyInterface = m_physicsSystem.GetBodyInterface();
        bodyInterface.RemoveBody(id);
        bodyInterface.DestroyBody(id);
    }

    ValidateResult PhysicsContactListener::OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult)
    {
        return ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void PhysicsContactListener::OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings)
    {
        Core::GetEngine().GetPhysicsManager()->OnContactAdded(inBody1, inBody2, inManifold, ioSettings);
    }

    void PhysicsContactListener::OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings)
    {
        Core::GetEngine().GetPhysicsManager()->OnContactPersisted(inBody1, inBody2, inManifold, ioSettings);
    }

    void PhysicsContactListener::OnContactRemoved(const SubShapeIDPair &inSubShapePair)
    {
        Core::GetEngine().GetPhysicsManager()->OnContactRemoved(inSubShapePair);
    }
}