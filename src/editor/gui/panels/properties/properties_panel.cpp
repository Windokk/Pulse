#include "properties_panel.hpp"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QFile>

namespace Pulse::Editor{


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
    }

    void PropertiesPanel::AddSeparator()
    {
        QFrame* line = new QFrame(containerWidget);
        line->setFrameShape(QFrame::HLine);
        line->setObjectName("PropertiesSeparator");
        line->setFrameShadow(QFrame::Sunken);
        mainLayout->addWidget(line);
    }

    QWidget* PropertiesPanel::AddPropertyWidget(QVBoxLayout* targetLayout, const QString& name, const nlohmann::ordered_json& value, int rowIndex)
    {
        if(name == "type" || name == "active"){
            return nullptr;
        }

        QWidget* field = nullptr;

        if (value.is_string())
        {
            auto* edit = new QLineEdit(QString::fromStdString(value.dump()));
            edit->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            field = edit;
        }
        else if(value.is_number_float()){
            auto* edit = new QLineEdit(QString::fromStdString(value.dump()));
            field = edit;
            QObject::connect(edit, &QLineEdit::editingFinished, [edit]() {
                if (edit->text().isEmpty()) {
                    edit->setText("0.0");
                }
            });
        }
        else if(value.is_number_integer() || value.is_number_unsigned()){
            auto* edit = new QLineEdit(QString::fromStdString(value.dump()));
            field = edit;
            QObject::connect(edit, &QLineEdit::editingFinished, [edit]() {
                if (edit->text().isEmpty()) {
                    edit->setText("0");
                }
            });
        }
        else if (value.is_boolean())
        {
            auto* combo = new QComboBox();
            combo->addItems({ "False", "True" });
            combo->setCurrentIndex(value.get<bool>() ? 1 : 0);
            field = combo;
        }
        else if(value.is_object()){
            //Vector
            if(value.contains("x") && value.contains("y") && value.contains("z")){
                QWidget* vectorRow = new QWidget();
                QHBoxLayout* vectorLayout = new QHBoxLayout(vectorRow);
                vectorLayout->setContentsMargins(0, 0, 0, 0);
                vectorLayout->setSpacing(4);

                auto* xEdit = new QLineEdit(QString::number(value["x"].get<float>()));
                auto* yEdit = new QLineEdit(QString::number(value["y"].get<float>()));
                auto* zEdit = new QLineEdit(QString::number(value["z"].get<float>()));

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

                field = vectorRow;
            }
            else{
                // Lists
                bool isNumericKeys = true;
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    for (char c : it.key()) {
                        if (!isdigit(c)) { isNumericKeys = false; break; }
                    }
                    if (!isNumericKeys) break;
                }

                if (isNumericKeys)
                {
                    QWidget* arrayRow = new QWidget();
                    QVBoxLayout* arrayLayout = new QVBoxLayout(arrayRow);
                    arrayLayout->setContentsMargins(0, 0, 0, 0);
                    arrayLayout->setSpacing(2);

                    QWidget* arrayContent = new QWidget();
                    QVBoxLayout* arrayContentLayout = new QVBoxLayout(arrayContent);
                    arrayContentLayout->setContentsMargins(0, 0, 0, 0);
                    arrayContentLayout->setSpacing(2);

                    int index = 0;
                    for (auto it = value.begin(); it != value.end(); ++it, ++index)
                    {
                        QWidget* childRow = AddPropertyWidget(arrayContentLayout,
                                                            QString("[%1]").arg(QString::fromStdString(it.key())),
                                                            it.value(),
                                                            index);
                        if (childRow)
                        {
                            if (index % 2 == 0)
                                childRow->setStyleSheet("background-color: rgba(50,50,50,255);");
                            else
                                childRow->setStyleSheet(""); // keep default
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
                    foldButton->setArrowType(Qt::DownArrow);
                    foldButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
                    QObject::connect(foldButton, &QToolButton::toggled, [arrayContent, foldButton, headerText](bool checked) {
                        arrayContent->setVisible(checked);            // Hide/show content
                        arrayContent->setSizePolicy(QSizePolicy::Preferred, checked ? QSizePolicy::Minimum : QSizePolicy::Ignored); // Adjust size policy
                        arrayContent->updateGeometry();               // Force layout update
                        arrayContent->parentWidget()->updateGeometry(); // Update parent container
                        foldButton->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
                        headerText->setText(checked ? "Collapse" : "Expand");
                    });


                    headerLayout->addWidget(foldButton);
                    headerLayout->addWidget(headerText);

                    // Add header and content to main layout
                    arrayLayout->addWidget(headerRow);
                    arrayLayout->addWidget(arrayContent);

                    field = arrayRow;
                }
                else
                {
                    // Normal object, recursion
                    QWidget* objectRow = new QWidget();
                    QVBoxLayout* objectLayout = new QVBoxLayout(objectRow);
                    objectLayout->setContentsMargins(0, 0, 0, 0);
                    objectLayout->setSpacing(2);

                    for (auto it = value.begin(); it != value.end(); ++it)
                    {
                        AddPropertyWidget(objectLayout, QString::fromStdString(it.key()), it.value());
                    }

                    field = objectRow;
                }
            }
        }

        QWidget* rowWidget = nullptr;
        if (field)
            rowWidget = CreatePropertyRow(name, field);

        if(rowWidget)
            targetLayout->addWidget(rowWidget);

        return rowWidget;
    }

    void PropertiesPanel::AddComponent(const std::string& name, const nlohmann::ordered_json& data)
    {
        const auto& component = data;

        bool active = true;
        if (component.contains("active"))
            active = component["active"].get<bool>();

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
            this, [this, &data](bool enabled)
            {
                // TODO: write back to ECS
                // data["active"] = enabled;
            });

        for (auto& [propName, value] : component.items())
        {
            AddPropertyWidget(
                bodyLayout,
                QString::fromStdString(propName),
                value
            );
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
            nlohmann::ordered_json data = comp->Serialize();
            AddComponent(data["type"], data);
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