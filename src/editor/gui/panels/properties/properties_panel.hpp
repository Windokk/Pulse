#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QFrame>
#include <QScrollArea>
#include <QToolButton>
#include <QCheckBox>

#include <nlohmann/json.hpp>

#include "engine/levels/level.hpp"
#include "engine/ecs/objects/actors/actor.hpp"

#include <memory>
#include <typeinfo>

#include "engine/core/reflection_fields.hpp"

namespace Pulse::Editor::GUI{

    class EditorMainWindow;

    struct ComponentHeader
    {
        QWidget* root;
        QToolButton* foldout;
        QCheckBox* activeToggle;
    };

    struct PropertyBinding
    {
        FieldInfo field;
        std::shared_ptr<Engine::ECS::Components::Component> comp;
        QWidget* widget = nullptr;
    };

    class PropertiesPanel : public QWidget{

        public:
            PropertiesPanel(QWidget *parent = nullptr);

            void Update(std::shared_ptr<Engine::ECS::Objects::Actor> selectedActor);

        private:

            QWidget* containerWidget;
            QVBoxLayout* mainLayout;
            QScrollArea* scrollArea;

            void Clear();
            QWidget *CreatePropertyRow(const QString &name, QWidget *field);
            ComponentHeader CreateComponentHeader(const QString &name, bool active);
            QWidget *CreateComponentBody();
            void AddSeparator();
            QWidget *AddPropertyWidget(QVBoxLayout *targetLayout, const QString &name, const FieldInfo *field, void *value, std::shared_ptr<Engine::ECS::Components::Component> comp, int rowIndex = -1, const Container* container = nullptr, void* elementPtr = nullptr);

            void AddComponent(const std::string &name, std::shared_ptr<Engine::ECS::Components::Component> comp, const std::vector<FieldInfo*> data);

            std::vector<PropertyBinding> properties;
    };
}