#include "Vitrae/Data/BoundingBox.hpp"

namespace Vitrae
{

glm::vec3 Vitrae::BoundingBox::getCenter() const
{
    return (min + max) / 2.0f;
}

glm::vec3 Vitrae::BoundingBox::getExtent() const
{
    return max - min;
}

void Vitrae::BoundingBox::merge(const BoundingBox &other)
{
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

void BoundingBox::transformLeft(const glm::mat3 &transform)
{
    glm::vec3 vertices[8];
    extractVertices(vertices);

    min = transform * vertices[0];
    max = transform * vertices[0];
    for (int i = 1; i < 8; i++) {
        min = glm::min(min, transform * vertices[i]);
        max = glm::max(max, transform * vertices[i]);
    }
}

void BoundingBox::transformLeft(const glm::mat4 &transform)
{
    glm::vec3 vertices[8];
    extractVertices(vertices);

    auto trans = [&](glm::vec3 v) {
        glm::vec4 tv = transform * glm::vec4(v, 1.0f);
        return glm::vec3(tv.x, tv.y, tv.z) / tv.w;
    };

    min = trans(vertices[0]);
    max = trans(vertices[0]);
    for (int i = 1; i < 8; i++) {
        min = glm::min(min, trans(vertices[i]));
        max = glm::max(max, trans(vertices[i]));
    }
}

void BoundingBox::expand(float scale)
{
    glm::vec3 c = getCenter();
    glm::vec3 e = getExtent();
    min = c - e * scale * 0.5f;
    max = c + e * scale * 0.5f;
}

void BoundingBox::expand(const glm::vec3 &scale)
{
    glm::vec3 c = getCenter();
    glm::vec3 e = getExtent();
    min = c - e * scale * 0.5f;
    max = c + e * scale * 0.5f;
}

void BoundingBox::extractVertices(glm::vec3 verts[8]) const
{
    verts[0] = {min.x, min.y, min.z};
    verts[1] = {min.x, min.y, max.z};
    verts[2] = {min.x, max.y, min.z};
    verts[3] = {min.x, max.y, max.z};
    verts[4] = {max.x, min.y, min.z};
    verts[5] = {max.x, min.y, max.z};
    verts[6] = {max.x, max.y, min.z};
    verts[7] = {max.x, max.y, max.z};
}

BoundingBox merged(const BoundingBox &a, const BoundingBox &b)
{
    BoundingBox result = a;
    result.merge(b);
    return result;
}

BoundingBox transformed(const glm::mat3 &transform, const BoundingBox &a)
{
    BoundingBox result = a;
    result.transformLeft(transform);
    return result;
}

BoundingBox transformed(const glm::mat4 &transform, const BoundingBox &a)
{
    BoundingBox result = a;
    result.transformLeft(transform);
    return result;
}

BoundingBox expanded(const BoundingBox &a, float scale)
{
    BoundingBox result = a;
    result.expand(scale);
    return result;
}

BoundingBox expanded(const BoundingBox &a, const glm::vec3 &scale)
{
    BoundingBox result = a;
    result.expand(scale);
    return result;
}

} // namespace Vitrae