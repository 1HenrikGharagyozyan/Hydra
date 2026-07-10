#include "hdpch.h"
#include "Scene.h"

#include "Hydra/Scene/Components.h"
#include "Hydra/Scene/ScriptableEntity.h"
#include "Hydra/Scene/SceneSerializer.h"
#include "Hydra/Renderer/Renderer2D.h"

#include <glm/glm.hpp>

#include <box2d/box2d.h>

#include "Entity.h"


namespace Hydra
{

    static b2BodyType Rigidbody2DTypeToBox2DBodyType(Rigidbody2DComponent::BodyType type)
    {
        switch (type)
        {
            case Rigidbody2DComponent::BodyType::Static:    return b2_staticBody;
            case Rigidbody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
            case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
        }

        HD_CORE_ASSERT(false, "Unknown Rigidbody2DComponent::BodyType");
        return b2_staticBody;
    }

    template<typename Component>
    static void CopyComponentIfExists(Entity dst, Entity src)
    {
        if (src.HasComponent<Component>())
            dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
    }

    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
        if (!B2_IS_NULL(m_PhysicsWorld))
        {
            auto view = m_Registry.view<Rigidbody2DComponent>();
            for (auto e : view)
            {
                Entity entity = { e, this };
                auto& rb2d = view.get<Rigidbody2DComponent>(e);

                if (entity.HasComponent<BoxCollider2DComponent>())
                {
                    auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
                    if (bc2d.RuntimeFixture)
                    {
                        delete (b2ShapeId*)bc2d.RuntimeFixture;
                        bc2d.RuntimeFixture = nullptr;
                    }
                }

                if (entity.HasComponent<CircleCollider2DComponent>())
                {
                    auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
                    if (cc2d.RuntimeFixture)
                    {
                        delete (b2ShapeId*)cc2d.RuntimeFixture;
                        cc2d.RuntimeFixture = nullptr;
                    }
                }

                delete (b2BodyId*)rb2d.RuntimeBody;
                rb2d.RuntimeBody = nullptr;
            }
            b2DestroyWorld(m_PhysicsWorld);
        }
    }

    Ref<Scene> Scene::Copy(const Ref<Scene>& other)
    {
        Ref<Scene> newScene = CreateRef<Scene>();
        newScene->m_ViewportWidth = other->m_ViewportWidth;
        newScene->m_ViewportHeight = other->m_ViewportHeight;

        std::stringstream ss;
        SceneSerializer serializer(other);
        serializer.SerializeToStream(ss);
        
        SceneSerializer deserializer(newScene);
        deserializer.DeserializeFromStream(ss);
        
        return newScene;
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityWithUUID(UUID(), name);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string & name)
    {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<TransformComponent>();
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity);
    }

    void Scene::OnRuntimeStart()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = { 0.0f, -9.8f };
        m_PhysicsWorld = b2CreateWorld(&worldDef);

        auto view = m_Registry.view<Rigidbody2DComponent>();
        for (auto e : view)
        {
            Entity entity = { e, this };
            auto& transform = entity.GetComponent<TransformComponent>();
            auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
            rb2d.RuntimeBody = nullptr;

            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = Rigidbody2DTypeToBox2DBodyType(rb2d.Type);

            if (bodyDef.type == b2_dynamicBody)
                bodyDef.isBullet = true;

            bodyDef.position = { transform.Translation.x, transform.Translation.y };
            bodyDef.rotation = b2MakeRot(transform.Rotation.z);
            bodyDef.fixedRotation = rb2d.FixedRotation;

            b2BodyId bodyId = b2CreateBody(m_PhysicsWorld, &bodyDef);
            rb2d.RuntimeBody = new b2BodyId(bodyId);

            if (entity.HasComponent<BoxCollider2DComponent>())
            {
                auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
                bc2d.RuntimeFixture = nullptr;

                // bc2d.Size is already a half-extent — do NOT multiply by 0.5 again.
                b2Vec2 center = { bc2d.Offset.x, bc2d.Offset.y };
                b2Polygon shape = b2MakeOffsetBox(
                    bc2d.Size.x * transform.Scale.x,
                    bc2d.Size.y * transform.Scale.y,
                    center, 0.0f);
                shape.radius = 0.03f;

                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = bc2d.Density;

                shapeDef.friction = bc2d.Friction;
                shapeDef.restitution = bc2d.Restitution;

                b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &shape);
                bc2d.RuntimeFixture = new b2ShapeId(shapeId);
            }

            if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
                cc2d.RuntimeFixture = nullptr;

                b2Circle circle;
                circle.center = { cc2d.Offset.x, cc2d.Offset.y };
                circle.radius = cc2d.Radius * transform.Scale.x; // TODO: Support non-uniform scaling

                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = cc2d.Density;
                shapeDef.friction = cc2d.Friction;
                shapeDef.restitution = cc2d.Restitution;

                b2ShapeId shapeId = b2CreateCircleShape(bodyId, &shapeDef, &circle);
                cc2d.RuntimeFixture = new b2ShapeId(shapeId);
			}
        }
	}

	void Scene::OnRuntimeStop()
	{
        auto view = m_Registry.view<Rigidbody2DComponent>();
        for (auto e : view)
        {
            Entity entity = { e, this };
            auto& rb2d = view.get<Rigidbody2DComponent>(e);

            if (entity.HasComponent<BoxCollider2DComponent>())
            {
                auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
                if (bc2d.RuntimeFixture)
                {
                    delete (b2ShapeId*)bc2d.RuntimeFixture;
                    bc2d.RuntimeFixture = nullptr;
                }
            }
            
            if (entity.HasComponent<CircleCollider2DComponent>())
            {
                auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
                if (cc2d.RuntimeFixture)
                {
                    delete (b2ShapeId*)cc2d.RuntimeFixture;
                    cc2d.RuntimeFixture = nullptr;
                }
            }

            if (rb2d.RuntimeBody)
            {
                delete (b2BodyId*)rb2d.RuntimeBody;
                rb2d.RuntimeBody = nullptr;
            }
        }

        b2DestroyWorld(m_PhysicsWorld);
        m_PhysicsWorld = b2_nullWorldId;
	}

    void Scene::OnUpdateRuntime(Timestep ts)
    {

        // Update Scripts
        {
            m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
            {
                // TODO: Move to Scene::OnScenePlay
                if (!nsc.Instance)
                {
                    nsc.Instance = nsc.InstantiateScript();
                    nsc.Instance->m_Entity = Entity{ entity, this };
                    nsc.Instance->OnCreate();
                }

                nsc.Instance->OnUpdate(ts); 
            });
        }

        // Physics
        if (!B2_IS_NULL(m_PhysicsWorld))
        {
            const float timeStep = glm::min((float)ts, 1.0f / 30.0f);
            const int32_t subStepCount = 8;
            b2World_Step(m_PhysicsWorld, timeStep, subStepCount);

            auto view = m_Registry.view<Rigidbody2DComponent>();
            for (auto e : view)
            {
                Entity entity = { e, this };
                auto& transform = entity.GetComponent<TransformComponent>();
                auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

                if (!rb2d.RuntimeBody)
                    continue;

                b2BodyId bodyId = *(b2BodyId*)rb2d.RuntimeBody;

                if (b2Body_IsValid(bodyId))
                {
                    b2Vec2 position = b2Body_GetPosition(bodyId);
                    float angle = b2Rot_GetAngle(b2Body_GetRotation(bodyId));

                    transform.Translation.x = position.x;
                    transform.Translation.y = position.y;
                    transform.Rotation.z = angle;
                }
            }
        }

        // Render 2D
        Camera* mainCamera = nullptr;
        glm::mat4 cameraTransform;
        {
            auto view = m_Registry.view<TransformComponent, CameraComponent>();
            for (auto entity : view)
            {
                auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

                if (camera.Primary)
                {
                    mainCamera = &camera.Camera;
                    cameraTransform = transform.GetTransform();
                    break;
                }
            }
        }

        if (mainCamera)
        {
            Renderer2D::BeginScene(*mainCamera, cameraTransform);

            // Draw sprites
            // NOTE: This group owns TransformComponent storage. Do NOT create a second owning
            // group<TransformComponent> anywhere else in this registry — EnTT will assert at runtime.
            // Circle rendering intentionally uses a non-owning view for this reason.
            {
                auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
                for (auto entity : group)
                {
                    auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

                    Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
                }
            }

            // Draw circles
            {
                auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
                for (auto entity : view)
                {
                    auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                    Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
                }
            }

            Renderer2D::EndScene();
        }
    }

    void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera)
    {
        Renderer2D::BeginScene(camera);

        // Draw sprites
        {
            auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
            for (auto entity : view)
            {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
            }
        }

        // Draw circles
        {
            auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
            for (auto entity : view)
            {
                auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
            }
        }

        Renderer2D::EndScene();
    }   

    void Scene::OnViewportResize(uint32_t width, uint32_t height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;

        // Resize our non-FixedAspectRatio cameras
        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto& cameraComponent = view.get<CameraComponent>(entity);
            if (!cameraComponent.FixedAspectRatio)
                cameraComponent.Camera.SetViewportSize(width, height);
        }
    }

    void Scene::DuplicateEntity(Entity entity)
    {
        std::string name = entity.GetName();
        Entity newEntity = CreateEntity(name);

        CopyComponentIfExists<TransformComponent>(newEntity, entity);
        CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
        CopyComponentIfExists<CircleRendererComponent>(newEntity, entity);
        CopyComponentIfExists<CameraComponent>(newEntity, entity);
        CopyComponentIfExists<NativeScriptComponent>(newEntity, entity);
        CopyComponentIfExists<Rigidbody2DComponent>(newEntity, entity);
        CopyComponentIfExists<BoxCollider2DComponent>(newEntity, entity);
        CopyComponentIfExists<CircleCollider2DComponent>(newEntity, entity);
    }

    Entity Scene::GetPrimaryCameraEntity()
    {
        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view)
        {
            const auto& cameraComponent = view.get<CameraComponent>(entity);
            if (cameraComponent.Primary)
                return Entity{ entity, this };
        }
        return {};
    }

    template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		// static_assert(false);
	}

    template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
        if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
            component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

    template<>
    void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
    {
    }

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

    template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}

    template<>
    void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
    {
    }

}