#ifndef IVY_PLATFORM_H
#define IVY_PLATFORM_H

namespace ivy {

/**
 * \brief Platform abstraction layer
 */
class Platform final {
public:
    Platform();
    ~Platform();

    /**
     * \brief Poll events, update platform specific code
     */
    void update();
};

}

#endif //IVY_PLATFORM_H
