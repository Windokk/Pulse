#pragma once

class QWidget;
class QWindow;

namespace QtImGui {

typedef void* RenderRef;

#ifdef QT_WIDGETS_LIB
RenderRef Initialize(QWidget *window, bool defaultRender = true);
#endif

RenderRef Initialize(QWindow *window, bool defaultRender = true);
void NewFrame(RenderRef ref = nullptr);
void Render(RenderRef ref = nullptr);
void Shutdown(RenderRef ref = nullptr);
}
