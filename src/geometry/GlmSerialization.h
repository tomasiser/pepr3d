#pragma once

#include <glm/glm.hpp>

namespace glm {
template <typename Archive>
void serialize(Archive& archive, glm::vec3& v3) {
    archive(cereal::make_nvp("x", v3.x), cereal::make_nvp("y", v3.y), cereal::make_nvp("z", v3.z));
}

template <typename Archive>
void serialize(Archive& archive, glm::vec4& v4) {
    archive(cereal::make_nvp("x", v4.x), cereal::make_nvp("y", v4.y), cereal::make_nvp("z", v4.z),
            cereal::make_nvp("z", v4.w));
}
}  // namespace glm