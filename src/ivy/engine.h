#ifndef IVY_ENGINE_H
#define IVY_ENGINE_H

#include "ivy/options.h"

namespace ivy {

/**
 * \brief The core engine, everything runs from here
 */
class Engine final {
public:
    Engine();
    ~Engine();

    /**
     * \brief Runs the engine
     */
    void run();

    /**
     * \brief Get reference to engine options
     * \return Engine options
     */
    Options &getOptions() {
        return options_;
    }

private:
    Options options_;
};

}

#endif // IVY_ENGINE_H
