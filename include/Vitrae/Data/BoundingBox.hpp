#pragma once

#include "glm/glm.hpp"

namespace Vitrae
{

struct BoundingBox
{
    glm::vec3 min;
    glm::vec3 max;

    glm::vec3 getCenter() const;
    glm::vec3 getExtent() const;

    void merge(const BoundingBox &other);
    void transformLeft(const glm::mat3 &transform);
    void transformLeft(const glm::mat4 &transform);
    void expand(float scale);
    void expand(const glm::vec3 &scale);

    void extractVertices(glm::vec3 verts[8]) const;
};

BoundingBox merged(const BoundingBox &a, const BoundingBox &b);
BoundingBox transformed(const glm::mat3 &transform, const BoundingBox &a);
BoundingBox transformed(const glm::mat4 &transform, const BoundingBox &a);
BoundingBox expanded(const BoundingBox &a, float scale);
BoundingBox expanded(const BoundingBox &a, const glm::vec3 &scale);

} // namespace Vitrae