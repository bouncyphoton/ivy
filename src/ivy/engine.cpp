#include "engine.h"
#include "ivy/log.h"
#include "ivy/renderer.h"
#include "ivy/platform/platform.h"
#include "ivy/graphics/render_device.h"
#include "ivy/entity/components/transform.h"
#include "ivy/entity/components/model.h"
#include "ivy/entity/components/camera.h"
#include "ivy/resources/resource_manager.h"
#include <GLFW/glfw3.h>

namespace ivy {

// TODO: remove temp
static void game_init(ResourceManager &resourceManager, std::vector<Entity> &entities) {
    for (u32 i = 0; i < 10; ++i) {
        entities.emplace_back();
        entities.back().setTag("bunny");
        entities.back().setComponent<Transform>();
        entities.back().setComponent<Model>(resourceManager.getModel("models/bunny.obj"));
    }
    {
        entities.emplace_back();
        entities.back().setComponent<Transform>();
        entities.back().setComponent<Model>(resourceManager.getModel("models/sponza/sponza.obj"));

        entities.emplace_back();
        entities.back().setComponent<Transform>(Transform(glm::vec3(0, 1, 5)));
        entities.back().setComponent<Camera>();
    }
}

static void game_update(InputState input, f32 dt, std::vector<Entity> &entities) {
    // Update entities
    for (u32 i = 0; i < entities.size(); ++i) {
        const Entity &entity = entities[i];

        Transform *transform = entity.getComponent<Transform>();
        if (!transform) {
            continue;
        }

        // Move bunnies
        if (entity.hasTag("bunny")) {
            // Update the position for this entity
            f32 time = (f32) glfwGetTime();
            f32 x = i - entities.size() * 0.5f + 0.5f;
            transform->setPosition(glm::vec3(x, 1 + sinf(x + time), 0));
            transform->setRotation(glm::vec3(0, time, 0));
            transform->setScale(glm::vec3(cosf(x + time) * 0.5f + 0.5f));
        }

        // Update camera
        if (entity.getComponent<Camera>()) {
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

            Log::info("%f, %f, %f", transform->getRotation().x, transform->getRotation().y, transform->getRotation().z);
        }
    }
}

Engine::Engine() {
    LOG_CHECKPOINT();
}

Engine::~Engine() {
    LOG_CHECKPOINT();
}

void Engine::run() {
    LOG_CHECKPOINT();

    // These are static so that fatal error still calls destructors, not a great solution
    static Platform platform(options_);
    static gfx::RenderDevice renderDevice(options_, platform);
    static Renderer renderer(renderDevice);
    static ResourceManager resourceManager(renderDevice, "../assets");

    std::vector<Entity> entities;
    game_init(resourceManager, entities);

    // Main loop
    while (!platform.isCloseRequested()) {
        // Poll events
        platform.update();

        // Exit on escape
        if (platform.getInputState().isKeyPressed(GLFW_KEY_ESCAPE)) {
            break;
        }

        game_update(platform.getInputState(), platform.getDt(), entities);

        // Render entities
        renderer.render(entities);
    }
}

}
