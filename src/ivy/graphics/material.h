#ifndef IVY_MATERIAL_H
#define IVY_MATERIAL_H

namespace ivy::gfx {

class Texture2D;

/**
 * \brief Describes a surface
 */
class Material {
public:
    explicit Material(const Texture2D &diffuse_texture)
        : diffuseTexture_(diffuse_texture) {}

    [[nodiscard]] const Texture2D &getDiffuseTexture() const {
        return diffuseTexture_;
    }

private:
    const Texture2D &diffuseTexture_;
};

}

#endif // IVY_MATERIAL_H
