#include <gtest/gtest.h>

#include "support/test_engine_context.hpp"

#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/ecs/components/rendering/camera.hpp"
#include "engine/ecs/components/physics/physics_body.hpp"

using namespace Pulse::Engine::Core;
using namespace Pulse::Engine::ECS::Objects;
using namespace Pulse::Engine::ECS::Components;

class ActorTest : public ::testing::Test {
    protected:
        void SetUp() override {
            SetEngine(&context);
        }

        void TearDown() override {
            SetEngine(nullptr);
        }

        Pulse::Tests::TestEngineContext context;
};

TEST_F(ActorTest, CreationAutoAddsTransformComponent) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    ASSERT_NE(actor->transform, nullptr);
    EXPECT_TRUE(actor->HasComponent<Transform>());
    EXPECT_EQ(actor->GetComponents().size(), 1u);
}

TEST_F(ActorTest, GetNameAndSetNameRoundtrip) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    EXPECT_EQ(actor->GetName(), "Player");

    actor->SetName("Renamed");

    EXPECT_EQ(actor->GetName(), "Renamed");
}

TEST_F(ActorTest, AddComponentAppendsToComponentList) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    auto camera = actor->AddComponent<Camera>();

    ASSERT_NE(camera, nullptr);
    EXPECT_TRUE(actor->HasComponent<Camera>());
    EXPECT_EQ(actor->GetComponents().size(), 2u);
}

TEST_F(ActorTest, HasComponentIsFalseForComponentTypeNeverAdded) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    EXPECT_FALSE(actor->HasComponent<PhysicsBody>());
}

TEST_F(ActorTest, GetComponentReturnsNthMatchByInsertionOrder) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    auto first = actor->AddComponent<Camera>();
    auto second = actor->AddComponent<Camera>();

    EXPECT_EQ(actor->GetComponent<Camera>(0), first);
    EXPECT_EQ(actor->GetComponent<Camera>(1), second);
}

TEST_F(ActorTest, GetComponentsReturnsAllMatchesOfType) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    actor->AddComponent<Camera>();
    actor->AddComponent<Camera>();

    EXPECT_EQ(actor->GetComponents<Camera>().size(), 2u);
}

TEST_F(ActorTest, AddingSecondTransformFailsAndReturnsNull) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    auto second = actor->AddComponent<Transform>();

    EXPECT_EQ(second, nullptr);
    EXPECT_EQ(actor->GetComponents().size(), 1u);
}

TEST_F(ActorTest, IsActiveDefaultsToTrue) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    EXPECT_TRUE(actor->IsActive());
}

TEST_F(ActorTest, ComponentParentPointsBackToOwningActor) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    auto camera = actor->AddComponent<Camera>();

    EXPECT_EQ(camera->parent, actor);
}

TEST_F(ActorTest, GetComponentIDInLevelPacksActorAndComponentIndex) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);

    int packed = actor->GetComponentIDInLevel(2);

    EXPECT_EQ(packed, (actor->GetID().GetAsInt() << 12) | 2);
}

TEST_F(ActorTest, ActorAndItsComponentsResolveTheInjectedEngineContext) {
    auto actor = Object::CreateWithContext<Actor>(&context, std::string("Player"), &context);
    auto camera = actor->AddComponent<Camera>();

    EXPECT_EQ(actor->GetEngineContext(), &context);
    EXPECT_EQ(camera->GetEngineContext(), &context);
}
