#include "renderer.h"
#include "ivy/log.h"

namespace ivy {

Renderer::Renderer(const Platform &platform)
    : ctx_(platform) {
    LOG_CHECKPOINT();
    // TODO: choose device, create pipelines etc. via a "render context"?
}

Renderer::~Renderer() {
    LOG_CHECKPOINT();
    // TODO
}

void Renderer::render() {
    // TODO
}

}
