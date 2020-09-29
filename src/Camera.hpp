#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Camera {
    // View properties
    glm::vec3 position;
    glm::quat rotation;

    // Projection properties
    float fovy;
    float aspectRatio;
    float nearPlane;
    float farPlane;
};
