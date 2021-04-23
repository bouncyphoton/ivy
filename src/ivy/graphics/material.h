#ifndef IVY_MATERIAL_H
#define IVY_MATERIAL_H

namespace ivy::gfx {

class Texture;

/**
 * \brief Describes a surface
 */
class Material {
public:
    explicit Material(const Texture &diffuse_texture, const Texture &occlusion_texture,
                      const Texture &roughness_texture, const Texture &metallic_texture)
        : diffuseTexture_(diffuse_texture), occlusionTexture_(occlusion_texture),
          roughnessTexture_(roughness_texture), metallicTexture_(metallic_texture) {}

    [[nodiscard]] const Texture &getDiffuseTexture() const {
        return diffuseTexture_;
    }

    [[nodiscard]] const Texture &getOcclusionTexture() const {
        return occlusionTexture_;
    }

    [[nodiscard]] const Texture &getRoughnessTexture() const {
        return roughnessTexture_;
    }

    [[nodiscard]] const Texture &getMetallicTexture() const {
        return metallicTexture_;
    }

private:
    const Texture &diffuseTexture_;
    const Texture &occlusionTexture_;
    const Texture &roughnessTexture_;
    const Texture &metallicTexture_;
};

}

#endif // IVY_MATERIAL_H
