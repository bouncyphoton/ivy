#ifndef IVY_RESOURCE_H
#define IVY_RESOURCE_H

namespace ivy {

/**
 * \brief Opaque resource handle
 * \tparam T Resource type
 */
template <typename T>
class Resource {
public:
    const T &get() const {
        return resource_;
    }

private:
    friend class ResourceManager;

    explicit Resource(const T &resource)
        : resource_(resource) {}

    const T &resource_;
};

}

#endif //IVY_RESOURCE_H
