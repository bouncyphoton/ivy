#include "test_game.h"
#include "ivy/types.h"
#include "ivy/log.h"
#include "ivy/scene/entity.h"
#include "ivy/scene/components/transform.h"
#include "ivy/scene/components/model.h"
#include "ivy/scene/components/camera.h"
#include "ivy/scene/components/light.h"
#include "ivy/resources/resource_manager.h"
#include "ivy/platform/input_state.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using namespace ivy;

TestGame::TestGame()
    : engine_(getOptions()), renderer_(engine_.getRenderDevice()) {

    // Set logging level
    Log::logLevel = Log::LogLevel::DEBUG;

    // Run engine with our init, update and render
    engine_.run(
    [&]() {
        init();
    },
    [&]() {
        update();
    },
    [&]() {
        render();
    });
}

void TestGame::init() {
    ResourceManager &resourceManager = engine_.getResourceManager();

    // Add helmets
    for (i32 i = 0; i < 10; ++i) {
        EntityHandle helmet = scene_.createEntity();
        helmet->setComponent(Transform(glm::vec3((i - 5) * 2, 1, 0), glm::vec3(0), glm::vec3(2)));
        helmet->setTag("helmet");
        helmet->setComponent(Model(resourceManager.getModel("models/glTF-Sample-Models/2.0/FlightHelmet/glTF/FlightHelmet.gltf")));
    }

    // Add lights
    for (i32 i = 0; i < 2; ++i) {
        f32 x = (i - 1) * 5;
        f32 y = 2;
        f32 z = -5;

        // Blue and pink looks nice
        glm::vec3 colors[] = {
            glm::vec3(10, 100, 255) / 255.0f,
            glm::vec3(255, 60, 240) / 255.0f
        };

        EntityHandle light = scene_.createEntity();
        light->setComponent(Transform(glm::vec3(x, y, z)));
        light->setComponent(PointLight(colors[i], 400));
        light->setTag("pnt_light");
    }

    // Add sponza
    {
        EntityHandle sponza = scene_.createEntity();
        sponza->setTag("sponza");
        sponza->setComponent<Transform>();
        sponza->setComponent(Model(resourceManager.getModel("models/sponza/sponza.obj")));
    }

    // Add camera
    {
        EntityHandle camera = scene_.createEntity();
        camera->setTag("camera");
        camera->setComponent(Transform(glm::vec3(0.25, 2, -1), glm::vec3(6.2, 2.5, 0)));
        camera->setComponent<Camera>();
    }
}

void TestGame::update() {
    Platform &platform = engine_.getPlatform();
    f32 dt = platform.getDt();
    const InputState &input = platform.getInputState();

    // Exit on escape
    if (platform.getInputState().isKeyPressed(GLFW_KEY_ESCAPE)) {
        engine_.stop();
    }

    // Set debug render mode
    if (input.isKeyPressed(GLFW_KEY_1)) {
        debugMode_ = Renderer::DebugMode::FULL;
        Log::debug("Rendering with full shading");
    } else if (input.isKeyPressed(GLFW_KEY_2)) {
        debugMode_ = Renderer::DebugMode::DIFFUSE;
        Log::debug("Rendering diffuse gbuffer");
    }  else if (input.isKeyPressed(GLFW_KEY_3)) {
        debugMode_ = Renderer::DebugMode::NORMAL;
        Log::debug("Rendering normal gbuffer");
    }  else if (input.isKeyPressed(GLFW_KEY_4)) {
        debugMode_ = Renderer::DebugMode::WORLD;
        Log::debug("Rendering derived world position");
    }  else if (input.isKeyPressed(GLFW_KEY_5)) {
        debugMode_ = Renderer::DebugMode::SHADOW_MAP;
        Log::debug("Rendering shadow map");
    }

    // Delete nearest light
    if (input.isKeyPressed(GLFW_KEY_BACKSPACE)) {
        if (EntityHandle camera = scene_.findEntityWithAllComponents<Transform, Camera>()) {
            Transform ct = *camera->getComponent<Transform>();

            // Find nearest light
            EntityHandle nearestEntity(scene_);
            f32 nearest = 0.0f;
            for (EntityHandle light : scene_.findEntitiesWithAllComponents<Transform, PointLight>()) {
                Transform lt = *light->getComponent<Transform>();
                f32 dist = glm::distance(lt.getPosition(), ct.getPosition());
                if (!nearestEntity || dist < nearest) {
                    nearest = dist;
                    nearestEntity = light;
                }
            }

            scene_.deleteEntity(nearestEntity);
        }
    }

    // Spawn light at camera location
    if (input.isKeyPressed(GLFW_KEY_ENTER)) {
        if (EntityHandle camera = scene_.findEntityWithAllComponents<Transform, Camera>()) {
            EntityHandle light = scene_.createEntity();
            light->setComponent(*camera->getComponent<Transform>());
            light->setComponent(PointLight(glm::vec3(rand(), rand(), rand()) / (f32)RAND_MAX, 400));
        }
    }

    // Update entities
    f32 time = (f32) glfwGetTime();
    for (EntityHandle entity : scene_) {
        Transform *transform = entity->getComponent<Transform>();
        if (!transform) {
            continue;
        }

        // Update the rotation for the helmet
        if (entity->hasTag("helmet")) {
            transform->setRotation(glm::vec3(0, time * 0.5f, 0));
        }

        // Update camera
        if (entity->getComponent<Camera>()) {
            f32 moveSpeed = 5.0f * dt;
            f32 rotateSpeed = glm::half_pi<f32>() * dt;

            // Movement
            if (input.isKeyDown(GLFW_KEY_W)) {
                transform->addPosition(moveSpeed * transform->getForward());
            }
            if (input.isKeyDown(GLFW_KEY_S)) {
                transform->addPosition(-moveSpeed * transform->getForward());
            }
            if (input.isKeyDown(GLFW_KEY_D)) {
                transform->addPosition(moveSpeed * transform->getRight());
            }
            if (input.isKeyDown(GLFW_KEY_A)) {
                transform->addPosition(-moveSpeed * transform->getRight());
            }
            if (input.isKeyDown(GLFW_KEY_SPACE)) {
                transform->addPosition(moveSpeed * Transform::UP);
            }
            if (input.isKeyDown(GLFW_KEY_LEFT_CONTROL)) {
                transform->addPosition(-moveSpeed * Transform::UP);
            }

            // Rotation
            if (input.isKeyDown(GLFW_KEY_RIGHT)) {
                transform->addRotation(Transform::UP * -rotateSpeed);
            }
            if (input.isKeyDown(GLFW_KEY_LEFT)) {
                transform->addRotation(Transform::UP * rotateSpeed);
            }
            if (input.isKeyDown(GLFW_KEY_UP)) {
                transform->addRotation(Transform::RIGHT * rotateSpeed);
            }
            if (input.isKeyDown(GLFW_KEY_DOWN)) {
                transform->addRotation(Transform::RIGHT * -rotateSpeed);
            }
        }
    }
}

void TestGame::render() {
    renderer_.render(scene_, debugMode_);
}

Options TestGame::getOptions() {
    Options options;

    options.appName = "Vulkan Deferred Rendering Demo | ivy engine";
    options.renderWidth = 1600;
    options.renderHeight = 900;

    return options;
}
