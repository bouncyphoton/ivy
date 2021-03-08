#ifndef IVY_MATERIAL_H
#define IVY_MATERIAL_H

namespace ivy::gfx {

class Texture2D;

/**
 * \brief Describes a surface
 */
class Material {
public:
    explicit Material(const Texture2D &albedo_texture)
        : albedoTexture_(albedo_texture) {}

    [[nodiscard]] const Texture2D &getAlbedoTexture() const {
        return albedoTexture_;
    }

private:
    const Texture2D &albedoTexture_;
};

}

#endif // IVY_MATERIAL_H
