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

namespace Pulse::Editor{

    class EditorMainWindow;

    struct ComponentHeader
    {
        QWidget* root;
        QToolButton* foldout;
        QCheckBox* activeToggle;
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
            void AddComponent(const std::string &name, const nlohmann::ordered_json &data);
            void AddSeparator();
            QWidget* AddPropertyWidget(QVBoxLayout *targetLayout, const QString &name, const nlohmann::ordered_json &value, int rowIndex = -1);
    };
}