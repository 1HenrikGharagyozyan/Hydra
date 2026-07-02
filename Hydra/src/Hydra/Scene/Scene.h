#pragma once

#include "Hydra/Renderer/EditorCamera.h"
#include "Hydra/Core/Timestep.h"
#include "Hydra/Core/UUID.h"

#include <entt.hpp>

#include <box2d/box2d.h>


namespace Hydra
{

    class Entity;

    class Scene
    {
    public:
        Scene();
        ~Scene();

        static Ref<Scene> Copy(const Ref<Scene>& other);

        Entity CreateEntity(const std::string& name = std::string());
        Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
        void DestroyEntity(Entity entity);

        void OnRuntimeStart();
		void OnRuntimeStop();

        void OnUpdateRuntime(Timestep ts);
        void OnUpdateEditor(Timestep ts, EditorCamera& camera);
        void OnViewportResize(uint32_t width, uint32_t height);

        void DuplicateEntity(Entity entity);

        Entity GetPrimaryCameraEntity();
        
    private:
        template<typename T>
        void OnComponentAdded(Entity entity, T& component);

    private:
        entt::registry m_Registry;
        uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

        b2WorldId m_PhysicsWorld = b2_nullWorldId;

        friend class Entity;
        friend class SceneSerializer;
        friend class SceneHierarchyPanel;
    };

}