#pragma once

#include <glm/glm.hpp>

const size_t MAX_LIGHTS = 4;

struct Light {
    alignas(16) glm::vec4 directionOrPosition;
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec4 attenuation; // x: Range, zw: spot light fade
    alignas(16) glm::vec3 spotLightDirection;
    
    Light(glm::vec4 directionOrPosition, glm::vec3 color, glm::vec4 attenuation = glm::vec4(), glm::vec3 spotLightDirection = glm::vec3())
        : directionOrPosition(directionOrPosition), color(color), attenuation(attenuation), spotLightDirection(spotLightDirection) {}
    
	// Initialized to 1 instead of 0 to avoid NaN in shader due to normalization of a zero vector
    Light() : directionOrPosition(1.0f, 1.0f, 1.0f, 0.0f), color(), attenuation(), spotLightDirection() {}
    
    static Light createDirectionalLight(glm::vec3 direction, glm::vec3 color) {
        glm::vec4 attenuation;
        attenuation.w = 1.0f;
        return Light(glm::vec4(glm::normalize(direction), 0.0f), color);
    }
    
    static Light createPointLight(glm::vec3 position, glm::vec3 color) {
        glm::vec4 attenuation;
        attenuation.w = 1.0f;
        return Light(glm::vec4(position, 1.0f), color);
    }
    
    static Light createSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float spotAngle) {
        // see 4.2 https://catlikecoding.com/unity/tutorials/scriptable-render-pipeline/lights/
        float outerRad = glm::radians(0.5f * spotAngle);
        float outerCos = glm::cos(outerRad);
        float outerTan = glm::tan(outerRad);
        float innerCos = glm::cos(glm::atan(((64.0f - 18.0f) / 64.0f) * outerTan));
        float angleRange = glm::max(innerCos - outerCos, 0.001f);
        
        glm::vec4 attenuation;
        attenuation.z = 1.0f / angleRange;
        attenuation.w = -outerCos * attenuation.z;
        
        return Light(glm::vec4(position, 1.0f), color, attenuation, glm::normalize(direction));
    }
};
