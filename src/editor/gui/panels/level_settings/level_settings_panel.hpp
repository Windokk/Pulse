#pragma once

namespace Pulse::Editor::GUI{

    // Editor panel for level-wide (not per-actor) settings - the level-scoped counterpart to
    // PropertiesPanel, which only ever edits the selected actor. Fields are grouped into collapsible
    // categories (General, Rendering, ...) rather than driven by the Component reflection system
    // (see FieldInfo/ClassDescriptor in reflection_fields.hpp) since Level isn't a Component and these
    // fields don't need to be inspectable/serializable through that generic machinery.
    class LevelSettingsPanel
    {
        public:
            void Draw();

        private:
            void DrawGeneralCategory();
            void DrawRenderingCategory();
    };
}
