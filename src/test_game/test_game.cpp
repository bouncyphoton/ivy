#include "test_game.h"
#include "ivy/types.h"
#include "ivy/log.h"
#include "ivy/entity/entity.h"
#include "ivy/entity/components/transform.h"
#include "ivy/entity/components/model.h"
#include "ivy/entity/components/camera.h"
#include "ivy/resources/resource_manager.h"
#include "ivy/platform/input_state.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using namespace ivy;

TestGame::TestGame()
    : engine_(getOptions()), renderer_(engine_.getRenderDevice()) {
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
    for (u32 i = 0; i < 10; ++i) {
        entities_.emplace_back();
        entities_.back().setTag("bunny");
        entities_.back().setComponent<Transform>();
        entities_.back().setComponent<Model>(resourceManager.getModel("models/bunny.obj"));
    }

    // Add sponza
    entities_.emplace_back();
    entities_.back().setComponent<Transform>();
    entities_.back().setComponent<Model>(resourceManager.getModel("models/sponza/sponza.obj"));

    // Add camera
    entities_.emplace_back();
    entities_.back().setComponent<Transform>(Transform(glm::vec3(0, 1, 5)));
    entities_.back().setComponent<Camera>();
}

void TestGame::update() {
    Platform &platform = engine_.getPlatform();
    f32 dt = platform.getDt();
    const InputState &input = platform.getInputState();

    // Exit on escape
    if (platform.getInputState().isKeyPressed(GLFW_KEY_ESCAPE)) {
        engine_.stop();
    }

    // Update entities
    for (u32 i = 0; i < entities_.size(); ++i) {
        const Entity &entity = entities_[i];

        Transform *transform = entity.getComponent<Transform>();
        if (!transform) {
            continue;
        }

        // Move bunnies
        if (entity.hasTag("bunny")) {
            // Update the position for this entity
            f32 time = (f32) glfwGetTime();
            f32 x = i - entities_.size() * 0.5f + 0.5f;
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
        }
    }
}

void TestGame::render() {
    renderer_.render(entities_);
}

Options TestGame::getOptions() {
    Options options;

    options.appName = "Vulkan Deferred Rendering Demo | ivy engine";
    options.renderWidth = 800;
    options.renderHeight = 600;

    return options;
}
