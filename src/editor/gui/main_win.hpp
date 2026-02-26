#pragma once

#include <QMainWindow>
#include <QWindow>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QApplication>
#include <QScreen>
#include <QtGui>


#include <DockManager.h>

#include "panels/viewport/viewport_window.hpp"
#include "panels/properties/properties_panel.hpp"
#include "panels/level_tree/level_tree.hpp"

#include "engine/debugging/logger.hpp"
#include "engine/ecs/objects/actors/actor.hpp"

#include <vector>
#include <memory>
#include <iostream>
#include <thread>

namespace Pulse::Editor::GUI{

    static QPalette createDarkPalette()
    {
        QPalette palette;

        // Base dark color tones
        QColor background("#1e1e1e");    // Main background
        QColor foreground("#e0e0e0");    // Main text
        QColor midlight("#8a8989ff");      // Slightly lighter for toolbars, etc.
        QColor highlight("#bdd5e9ff");     // Blue accent (select, hover)
        QColor disabledText("#5c5c5c");  // For disabled elements

        palette.setColor(QPalette::Window, background);
        palette.setColor(QPalette::WindowText, foreground);
        palette.setColor(QPalette::Base, QColor("#252525"));  // Input field background
        palette.setColor(QPalette::AlternateBase, background.darker(110));
        palette.setColor(QPalette::ToolTipBase, foreground);
        palette.setColor(QPalette::ToolTipText, foreground);
        palette.setColor(QPalette::Text, foreground);
        palette.setColor(QPalette::Button, background);
        palette.setColor(QPalette::ButtonText, foreground);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Highlight, highlight);
        palette.setColor(QPalette::HighlightedText, Qt::white);

        // Disabled states
        palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::Highlight, background.darker(150));

        return palette;
    }

    class EditorMainWindow : public QMainWindow, public Engine::Core::Platform::IWindow {

        public:

            void Init(const std::string &title, const int &width, const int &height,
                      const bool &fullscreen, const int &vsync) override;


            void InitGui();

            void PollEvents() override;
            void SwapBuffers() override;
            bool ShouldClose() const override;

            int GetFramebufferWidth() const override;
            int GetFramebufferHeight() const override;

            void* GetNativeHandle() const override;

            void Destroy() const override;
            void ToggleFullscreen() override;

            Engine::Core::Platform::SystemInfos GetSystemInfos() const override;

            void SetTitle(const std::string& title) override {
                this->setWindowTitle(QString::fromStdString(title));
            }

            void SetQTInputManager(Core::Platform::QTInput* inputManager) {
                glViewportWindow->SetQTInputManager(inputManager);
            }

            void SetSelectedActor(std::shared_ptr<Engine::ECS::Objects::Actor> newPtr){
                this->selectedActor = newPtr;

                /// Commented out because right now there's a bug making the whole editor crash in it : 
                /// terminate called after throwing an instance of 'std::out_of_range'
                /// what():  vector::_M_range_check: __n (which is 18446744072294738688) >= this->size() (which is 3)
                if(propertiesPanel)
                    propertiesPanel->Update(selectedActor);

                if(!newPtr)
                    treeWidget->ClearSelection();
                else
                    treeWidget->SetSelection(newPtr);
            }

            std::shared_ptr<Engine::ECS::Objects::Actor> GetSelectedActor(){
                return selectedActor;
            }

            void ProcessInputs() const override;

            int GetBytesPerPixel() const override{
                QSurfaceFormat format = QSurfaceFormat::defaultFormat();
                int redBits = format.redBufferSize();
                int greenBits = format.greenBufferSize();
                int blueBits = format.blueBufferSize();
                int alphaBits = format.alphaBufferSize();

                return (redBits + greenBits + blueBits + alphaBits) / 8;
            };

            PropertiesPanel* propertiesPanel = nullptr;

            LevelTree* treeWidget = nullptr;

        private:
                
            QtGLViewportWindow* glViewportWindow = nullptr;

            bool shouldClose = false;

            bool vsync = true;
            
            void closeEvent(QCloseEvent* event) override;

            std::unique_ptr<ads::CDockManager> dockManager;

            std::shared_ptr<Engine::ECS::Objects::Actor> selectedActor = nullptr;
    };
}

