#pragma once

#include "Vitrae/Assets/FrameStore.hpp"

#include "glad/glad.h"
// must be after glad.h
#include "GLFW/glfw3.h"

namespace Vitrae
{
/**
 * A FrameStore is a single image-like resource
 */
class OpenGLFrameStore : public FrameStore
{
  public:
    OpenGLFrameStore(const FrameStore::TextureBindParams &params);
    OpenGLFrameStore(const WindowDisplayParams &params);
    ~OpenGLFrameStore() override;

    std::size_t memory_cost() const override;
    void resize(glm::vec2 size) override;

    void startRender();
    void finishRender();

  protected:
    struct FramebufferContextSwitcher
    {
        void enterContext();
        void exitContext();
        void destroyContext();

        GLuint glFramebufferId;
    };
    struct WindowContextSwitcher
    {
        void enterContext();
        void exitContext();
        void destroyContext();

        GLFWwindow *window;

        std::function<void()> onClose;
        std::function<void(glm::vec2 motion, bool bLeft, bool bRight, bool bMiddle)> onDrag;

        glm::vec2 lastPos = {0.0f, 0.0f};
        bool bLeft = false, bRight = false, bMiddle = false;
    };

    std::variant<FramebufferContextSwitcher, WindowContextSwitcher> m_contextSwitcher;
};

} // namespace Vitrae