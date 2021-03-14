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

    // Add bunnies
    for (i32 i = 0; i < 10; ++i) {
        EntityHandle bunny = scene_.createEntity();
        bunny->setTag("bunny");
        bunny->setComponent(Transform(glm::vec3((i - 5) * 2, 1, 0)));
        bunny->setComponent(Model(resourceManager.getModel("models/bunny.obj")));
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
        camera->setComponent(Transform(glm::vec3(0, 1, 5)));
        camera->setComponent<Camera>();
    }

    // Add red directional light
    {
        EntityHandle light = scene_.createEntity();
        light->setTag("red_light");
        light->setComponent(DirectionalLight(glm::vec3(-0.1f, -1.0f, 0.1f), glm::vec3(1, 0, 0)));
    }

    // Add blue directional light
    {
        EntityHandle light = scene_.createEntity();
        light->setTag("blue_light");
        light->setComponent(DirectionalLight(glm::vec3(-0.1f, -1.0f, -0.1f), glm::vec3(0, 0, 1)));
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

    // Delete nearest bunny
    if (input.isKeyPressed(GLFW_KEY_BACKSPACE)) {
        if (EntityHandle camera = scene_.findEntityWithAllComponents<Transform, Camera>()) {
            Transform ct = *camera->getComponent<Transform>();

            // Find nearest bunny
            EntityHandle nearestBunny(scene_);
            f32 nearest = 0.0f;
            for (EntityHandle bunny : scene_.findEntitiesWithTag("bunny")) {
                Transform bt = *bunny->getComponent<Transform>();
                f32 dist = glm::distance(bt.getPosition(), ct.getPosition());
                if (!nearestBunny || dist < nearest) {
                    nearest = dist;
                    nearestBunny = bunny;
                }
            }

            scene_.deleteEntity(nearestBunny);
        }
    }

    // Spawn bunny at camera location
    if (input.isKeyPressed(GLFW_KEY_ENTER)) {
        if (EntityHandle camera = scene_.findEntityWithAllComponents<Transform, Camera>()) {
            EntityHandle bunny = scene_.createEntity();
            bunny->setTag("bunny");
            bunny->setComponent(*camera->getComponent<Transform>());
            bunny->setComponent(Model(engine_.getResourceManager().getModel("models/bunny.obj")));
        }
    }

    // Update entities
    f32 time = (f32) glfwGetTime();
    for (EntityHandle entity : scene_) {
        // Move lights
        if (entity->hasTag("red_light")) {
            DirectionalLight *light = entity->getComponent<DirectionalLight>();
            light->setDirection(glm::vec3(-0.1f, -1, std::sin(time) * 0.1));
        }
        if (entity->hasTag("blue_light")) {
            DirectionalLight *light = entity->getComponent<DirectionalLight>();
            light->setDirection(glm::vec3(-0.1f, -1, -std::sin(time) * 0.1));
        }

        Transform *transform = entity->getComponent<Transform>();
        if (!transform) {
            continue;
        }

        // Move bunnies
        if (entity->hasTag("bunny")) {
            // Update the rotation and scale for the bunny
            glm::vec3 pos = transform->getPosition();
            transform->setRotation(glm::vec3(0, time, 0));
            transform->setScale(glm::vec3(cosf(glm::length(pos) + time) * 0.5f + 0.5f));
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
    options.renderWidth = 800;
    options.renderHeight = 600;

    return options;
}
