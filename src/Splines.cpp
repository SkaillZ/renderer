#include "Splines.hpp"

glm::vec3 Splines::hermite(glm::vec3 p1, glm::vec3 p2, glm::vec3 m1, glm::vec3 m2, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    auto c1 = 2 * t3 - 3 * t2 + 1;
    auto c2 = t3 - 2 * t2 + t;
    auto c3 = t3 - t2;
    auto c4 = (-2 * t3 + 3 * t2);

    return c1 * p1 + c2 * m1 + c3 * m2 + c4 * p2;
}

static glm::vec3 kochanekBartelsIncomingTangent(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t, float b, float c) {
    return ((1 - t) * (1 + b) * (1 - c) * 0.5f) * (p2 - p1) + ((1 - t) * (1 - b) * (1 + c) * 0.5f) * (p3 - p2);
}

static glm::vec3 kochanekBartelsOutgoingTangent(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t, float b, float c) {
    return ((1 - t) * (1 + b) * (1 + c) * 0.5f) * (p2 - p1) + ((1 - t) * (1 - b) * (1 - c) * 0.5f) * (p3 - p2);
}

glm::vec3 Splines::kochanekBartels(
    glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4,
    float t1, float t2, float b1, float b2, float c1, float c2, float t)
{
    auto m1 = kochanekBartelsIncomingTangent(p1, p2, p3, t1, b1, c1);
    auto m2 = kochanekBartelsOutgoingTangent(p2, p3, p4, t2, b2, c2);

    return hermite(p2, p3, m1, m2, t);
}

glm::vec3 Splines::catmullRom(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, float t) {
    return kochanekBartels(p1, p2, p3, p4, 0, 0, 0, 0, 0, 0, t);
}
