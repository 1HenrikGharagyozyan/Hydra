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

        void OnSimulationStart();
        void OnSimulationStop();

        void OnUpdateRuntime(Timestep ts);
        void OnUpdateSimulation(Timestep ts, EditorCamera& camera);
        void OnUpdateEditor(Timestep ts, EditorCamera& camera);
        void OnViewportResize(uint32_t width, uint32_t height);

        void DuplicateEntity(Entity entity);

        Entity GetEntityByUUID(UUID uuid);

        Entity GetPrimaryCameraEntity();

        bool IsRunning() const { return m_IsRunning; }

        template<typename... Components>
        auto GetAllEntitiesWith()
        {
            return m_Registry.view<Components...>();
        }

    private:
        template<typename T>
        void OnComponentAdded(Entity entity, T& component);

        void OnPhysics2DStart();
        void OnPhysics2DStop();

        void RenderScene(EditorCamera& camera);

        void StepPhysics2D(Timestep ts);

    private:
        entt::registry m_Registry;
        uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
        bool m_IsRunning = false;

        b2WorldId m_PhysicsWorld = b2_nullWorldId;

        std::unordered_map<UUID, entt::entity> m_EntityMap;

        friend class Entity;
        friend class SceneSerializer;
        friend class SceneHierarchyPanel;
    };

}