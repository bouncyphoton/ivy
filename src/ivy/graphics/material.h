#ifndef IVY_MATERIAL_H
#define IVY_MATERIAL_H

namespace ivy::gfx {

class Texture;

/**
 * \brief Describes a surface
 */
class Material {
public:
    explicit Material(const Texture &diffuse_texture)
        : diffuseTexture_(diffuse_texture) {}

    [[nodiscard]] const Texture &getDiffuseTexture() const {
        return diffuseTexture_;
    }

private:
    const Texture &diffuseTexture_;
};

}

#endif // IVY_MATERIAL_H
