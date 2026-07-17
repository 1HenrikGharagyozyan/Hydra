#pragma once

#include "Hydra/Core/Base.h"
#include "Hydra/Scene/Scene.h"
#include "Hydra/Scene/Entity.h"


namespace Hydra
{

    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(const Ref<Scene>& context);

        void SetContext(const Ref<Scene>& context);

        void OnImGuiRender();

        Entity GetSelectedEntity() const { return m_SelectionContext; }
        void SetSelectedEntity(Entity entity);

    private:
        template<typename T>
        void DisplayAddComponentEntry(const std::string& entryName);
        
        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);

    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;
    };

}