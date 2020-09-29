#include <glm/glm.hpp>

namespace Splines {
    
    /** Hermite spline interpolation with points p1, p2 and tangents m1, m2 */ 
    glm::vec3 hermite(glm::vec3 p1, glm::vec3 p2, glm::vec3 m1, glm::vec3 m2, float t);

    /**
     * Kochanek-Bartels spline interpolation with points p1, p2, p3, p4 and tangent parameters for tension (t1, t2),
     * bias (b1, b2) and continuity (c1, c2) at time t.
     */
    glm::vec3 kochanekBartels(
        glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4,
        float t1, float t2, float b1, float b2, float c1, float c2, float t
    );

    /**
     * Kochanek-Bartels spline interpolation with points p1, p2, p3, p4 at time t (Kochanek-bartels with all zero parameters).
     */
    glm::vec3 catmullRom(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, float t);
    
}
