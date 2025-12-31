#include "properties_panel.hpp"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QFile>

#include "engine/core/reflection_fields.hpp"

namespace Pulse::Editor::GUI{

    PropertiesPanel::PropertiesPanel(QWidget* parent) : QWidget(parent)
    {
        scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);

        // Container widget
        containerWidget = new QWidget();
        containerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum); // natural height

        mainLayout = new QVBoxLayout(containerWidget);
        mainLayout->setSpacing(6);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

        scrollArea->setWidget(containerWidget);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(scrollArea);
        layout->setContentsMargins(0, 0, 0, 0);

        setObjectName("PropertiesPanel");

        QFile styleSheetFile(":/pulse/default/stylesheets/default_properties.qss");
	    if(styleSheetFile.open(QIODevice::ReadOnly)){
            QTextStream styleSheetStream(&styleSheetFile);
            QString result;
            result = styleSheetStream.readAll();
            styleSheetFile.close();
            setStyleSheet(result);
        }
    }

    void PropertiesPanel::Clear()
    {
        QLayoutItem* item;
        while ((item = mainLayout->takeAt(0)) != nullptr)
        {
            if (item->widget())
                item->widget()->deleteLater();

            if (item->layout())
                delete item->layout();

            delete item;
        }

        properties.clear();
    }

    void PropertiesPanel::AddSeparator()
    {
        QFrame* line = new QFrame(containerWidget);
        line->setFrameShape(QFrame::HLine);
        line->setObjectName("PropertiesSeparator");
        line->setFrameShadow(QFrame::Sunken);
        mainLayout->addWidget(line);
    }

    QWidget* PropertiesPanel::AddPropertyWidget(QVBoxLayout* targetLayout, const QString& name, const FieldInfo* field, void* value, std::shared_ptr<Engine::ECS::Components::Component> comp, int rowIndex)
    {
        QWidget* wField = nullptr;
        
        switch(field->type){
            case TypeID::String:{
                std::string& strVal = *static_cast<std::string*>(value);
                auto* edit = new QLineEdit(QString::fromStdString(strVal));
                edit->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                /*QObject::connect(edit, &QLineEdit::editingFinished,
                    this, [this, field, comp, edit]() {
                        std::string newValue = edit->text().toStdString();
                        FieldWrite(*field, comp.get(), &newValue);
                });*/
                wField = edit;
                break;
            }
            case TypeID::Float:{
                float& floatVal = *static_cast<float*>(value);
                auto* edit = new QLineEdit(QString::number(floatVal));
                wField = edit;
                QObject::connect(edit, &QLineEdit::editingFinished, [this, field, comp, edit]() {
                    if (edit->text().isEmpty()) {
                        edit->setText("0.0");
                    }
                    float newValue = edit->text().toFloat();
                    FieldWrite(*field, comp.get(), &newValue);
                });
                break;
            }
            case TypeID::Int32:{
                int& intVal = *static_cast<int*>(value);
                auto* edit = new QLineEdit(QString::number(intVal));
                wField = edit;
                QObject::connect(edit, &QLineEdit::editingFinished, [this, field, comp, edit]() {
                    if (edit->text().isEmpty()) {
                        edit->setText("0");
                    }
                    float newValue = edit->text().toInt();
                    FieldWrite(*field, comp.get(), &newValue);
                });
                break;
            }
            case TypeID::Bool:{
                bool& boolVal = *static_cast<bool*>(value);
                auto* box = new QCheckBox();
                box->setCheckState(boolVal ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
                wField = box;
                QObject::connect(box, &QCheckBox::toggled,
                this, [this, field, comp](bool checked) {

                    FieldWrite(*field, comp.get(), &checked);
                });
                break;
            }
            case TypeID::Vec3:{
                QWidget* vectorRow = new QWidget();
                QHBoxLayout* vectorLayout = new QHBoxLayout(vectorRow);
                vectorLayout->setContentsMargins(0, 0, 0, 0);
                vectorLayout->setSpacing(4);

                glm::vec3& vec3Val = *static_cast<glm::vec3*>(value);

                auto* xEdit = new QLineEdit(QString::number(vec3Val.x));
                auto* yEdit = new QLineEdit(QString::number(vec3Val.y));
                auto* zEdit = new QLineEdit(QString::number(vec3Val.z));

                auto setDefault = [](QLineEdit* edit){
                    QObject::connect(edit, &QLineEdit::editingFinished, [edit]() {
                        if (edit->text().isEmpty()) {
                            edit->setText("0.0");
                        }
                    });
                };

                setDefault(xEdit);
                setDefault(yEdit);
                setDefault(zEdit);

                xEdit->setStyleSheet("QLineEdit { border-left: 2px solid red; padding-left: 2px; }");
                yEdit->setStyleSheet("QLineEdit { border-left: 2px solid green; padding-left: 2px; }");
                zEdit->setStyleSheet("QLineEdit { border-left: 2px solid blue; padding-left: 2px; }");

                xEdit->setFixedWidth(50);
                yEdit->setFixedWidth(50);
                zEdit->setFixedWidth(50);

                vectorLayout->addWidget(xEdit);
                vectorLayout->addWidget(yEdit);
                vectorLayout->addWidget(zEdit);

                auto updateVec3 = [this, field, comp, xEdit, yEdit, zEdit]() {
                    glm::vec3 v{
                        xEdit->text().toFloat(),
                        yEdit->text().toFloat(),
                        zEdit->text().toFloat()
                    };
                    FieldWrite(*field, comp.get(), &v);
                };

                QObject::connect(xEdit, &QLineEdit::editingFinished, this, updateVec3);
                QObject::connect(yEdit, &QLineEdit::editingFinished, this, updateVec3);
                QObject::connect(zEdit, &QLineEdit::editingFinished, this, updateVec3);

                wField = vectorRow;
                break;
            }
            case TypeID::Vector:{
                
                if(!field->container)
                    return nullptr;

                QWidget* arrayRow = new QWidget();
                QVBoxLayout* arrayLayout = new QVBoxLayout(arrayRow);
                arrayLayout->setContentsMargins(0, 0, 0, 0);
                arrayLayout->setSpacing(2);

                QWidget* arrayContent = new QWidget();
                QVBoxLayout* arrayContentLayout = new QVBoxLayout(arrayContent);
                arrayContentLayout->setContentsMargins(0, 0, 0, 0);
                arrayContentLayout->setSpacing(2);

                int index = 0;

                TypeID vecType = field->container->editorElementType;

                FieldInfo* containedField = new FieldInfo{
                    "",
                    vecType,
                    0,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    field->flags,
                    field->min,
                    field->max,
                    nullptr,
                    nullptr
                };

                for (size_t i = 0; i < field->container->size(value); i++, index++) {
                    const void* vectorValue = field->container->getByIndex(value, i);

                    // Ensure buffer is resized to the correct size for the type
                    std::vector<uint8_t> buffer(GetTypeSize(vecType));

                    void* valuePtr = nullptr;

                    buffer.resize(GetTypeSize(vecType));

                    // Read the element into the buffer
                    field->container->elementRead(vectorValue, buffer.data());

                    valuePtr = buffer.data();

                    if (valuePtr != nullptr) {

                        // Now, let's add the widget to the layout
                        QWidget* childRow = AddPropertyWidget(arrayContentLayout,
                                                            QString("[%1]").arg(QString::number((int)i)),
                                                            containedField,
                                                            valuePtr, comp, index);

                        // Set background color for alternating rows
                        if (childRow) {
                            if (index % 2 == 0)
                                childRow->setStyleSheet("background-color: rgba(50,50,50,255);");
                            else
                                childRow->setStyleSheet("");  // keep default
                        }
                    } else {
                        // Debugging: Output if valuePtr is null
                        std::cerr << "Error: valuePtr is null at index " << i << std::endl;
                    }
                }

                // Row with button and array name
                QWidget* headerRow = new QWidget();
                QHBoxLayout* headerLayout = new QHBoxLayout(headerRow);
                headerLayout->setContentsMargins(0, 0, 0, 0);
                headerLayout->setSpacing(4);

                QLabel* headerText = new QLabel("Collapse");


                // Create fold/unfold button
                QToolButton* foldButton = new QToolButton();
                foldButton->setCheckable(true);
                foldButton->setChecked(true);
                foldButton->setIcon(QIcon(":/pulse/default/icons/down-arrow.svg"));
                foldButton->setIconSize(QSize(16, 16));
                foldButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

                QObject::connect(foldButton, &QToolButton::toggled,
                    [arrayContent, foldButton, headerText](bool checked) {

                    arrayContent->setVisible(checked);
                    arrayContent->setSizePolicy(QSizePolicy::Preferred, checked ? QSizePolicy::Minimum : QSizePolicy::Ignored
                    );

                    foldButton->setIcon(QIcon(
                        checked ? ":/pulse/default/icons/down-arrow.svg"
                                : ":/pulse/default/icons/right-arrow.svg"
                    ));

                    headerText->setText(checked ? "Collapse" : "Expand");

                    if (auto layout = arrayContent->parentWidget()->layout()) {
                        layout->invalidate();
                        layout->activate();
                    }
                });

                headerLayout->addWidget(foldButton);
                headerLayout->addWidget(headerText);

                // Add header and content to main layout
                arrayLayout->addWidget(headerRow);
                arrayLayout->addWidget(arrayContent);

                wField = arrayRow;
                break;
            }
        }

        QWidget* rowWidget = nullptr;
        if (wField)
            rowWidget = CreatePropertyRow(name, wField);

        if(rowWidget)
            targetLayout->addWidget(rowWidget);

        return rowWidget;
    }

    void PropertiesPanel::AddComponent(const std::string& name, std::shared_ptr<Engine::ECS::Components::Component> comp, const std::vector<FieldInfo*> data)
    {
        bool active = comp->Active();

        auto header = CreateComponentHeader(
            QString::fromStdString(name),
            active
        );

        QWidget* body = CreateComponentBody();
        auto* bodyLayout = static_cast<QVBoxLayout*>(body->layout());

        mainLayout->addWidget(header.root);
        mainLayout->addWidget(body);

        // Foldout controls body visibility
        connect(header.foldout, &QToolButton::toggled,
                body, &QWidget::setVisible);

        // Active toggle → component enabled state
        connect(header.activeToggle, &QCheckBox::toggled,
            this, [this, &data, comp](bool enabled)
            {
                enabled ? comp->Activate() : comp->DeActivate();
            });

        for (FieldInfo* field : data)
        {
            std::vector<uint8_t> buffer(GetTypeSize(field->type));

            void* valuePtr = nullptr;

            if (field->container)
            {
                valuePtr = reinterpret_cast<uint8_t*>(comp.get()) + field->offset;
            }
            else
            {
                // non-container → copy value
                buffer.resize(GetTypeSize(field->type));
                FieldRead(*field, comp.get(), buffer.data());
                valuePtr = buffer.data();
            }

            QWidget* w = AddPropertyWidget(
                bodyLayout,
                QString::fromStdString(field->name),
                field,
                valuePtr,
                comp
            );
            
            properties.push_back({*field, comp, w});
        }

        AddSeparator();
    }

    void PropertiesPanel::Update(std::shared_ptr<Engine::ECS::Objects::Actor> selectedActor)
    {
        Clear();

        if (!selectedActor)
            return;

        auto components = selectedActor->GetComponents();
        for (auto& comp : components)
        {
            const ComponentDescriptor* desc = comp->GetDescriptor();
            AddComponent(desc->name, comp, desc->fields);
        }
    }

    QWidget* PropertiesPanel::CreatePropertyRow(const QString& name, QWidget* field)
    {
        QWidget* row = new QWidget(containerWidget);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(4);
        layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        QLabel* label = new QLabel(name, row);
        label->setMinimumWidth(80);
        label->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        layout->addWidget(label);
        layout->addWidget(field);
        return row;
    }

    ComponentHeader PropertiesPanel::CreateComponentHeader(const QString& name, bool active)
    {
        ComponentHeader header{};

        header.root = new QWidget(containerWidget);
        header.root->setObjectName("HeaderRoot");
        header.root->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        QHBoxLayout* layout = new QHBoxLayout(header.root);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(6);
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Foldout arrow (left)
        header.foldout = new QToolButton(header.root);
        header.foldout->setCheckable(true);
        header.foldout->setChecked(true);
        header.foldout->setArrowType(Qt::DownArrow);
        header.foldout->setToolButtonStyle(Qt::ToolButtonIconOnly);

        // Component label
        QLabel* label = new QLabel(name, header.root);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        // Active checkbox (right)
        header.activeToggle = new QCheckBox(header.root);
        header.activeToggle->setChecked(active);

        // Layout: foldout | label | spacer | checkbox
        layout->addWidget(header.foldout);
        layout->addWidget(label);
        layout->addStretch(1);
        layout->addWidget(header.activeToggle);

        // Foldout arrow logic
        connect(header.foldout, &QToolButton::toggled, header.foldout,
            [btn = header.foldout](bool open)
            {
                btn->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
            });

        return header;
    }

    QWidget* PropertiesPanel::CreateComponentBody()
    {
        QWidget* body = new QWidget(containerWidget);
        QVBoxLayout* layout = new QVBoxLayout(body);
        layout->setContentsMargins(14, 6, 6, 6);
        layout->setSpacing(4);
        body->setLayout(layout);
        return body;
    }
}