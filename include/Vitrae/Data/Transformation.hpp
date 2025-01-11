#pragma once

#include "glm/gtc/quaternion.hpp"

namespace Vitrae
{
struct SimpleTransformation
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scaling;

    inline float getPitch() const { return glm::pitch(rotation); }

    inline float getYaw() const { return glm::yaw(rotation); }

    inline float getRoll() const { return glm::roll(rotation); }

    inline void setPitch(float pitch)
    {
        rotation = glm::quat(glm::vec3(pitch, getYaw(), getRoll()));
    }

    inline void setYaw(float yaw) { rotation = glm::quat(glm::vec3(getPitch(), yaw, getRoll())); }

    inline void setRoll(float roll) { rotation = glm::quat(glm::vec3(getPitch(), getYaw(), roll)); }

    inline glm::mat4 getModelMatrix() const
    {
        return glm::translate(glm::mat4(1), position) * glm::mat4_cast(rotation) *
               glm::scale(glm::mat4(1), scaling);
    }

    inline glm::mat4 getViewMatrix() const { return glm::inverse(getModelMatrix()); }

    inline void move(glm::vec3 pos) { this->position += pos; }

    inline void rotate(glm::vec3 rotPiYaRo)
    {
        this->rotation = glm::quat(glm::eulerAngles(this->rotation) + rotPiYaRo);
    }

    inline void scale(glm::vec3 scaling)
    {
        this->scaling = glm::vec3(this->scaling.x * scaling.x, this->scaling.y * scaling.y,
                                  this->scaling.z * scaling.z);
    }
};
} // namespace Vitrae