#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Dynamic/ArgumentScope.hpp"

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

    glm::vec2 getSize() const override;
    dynasma::FirmPtr<const ParamList> getRenderComponents() const override;

    void sync(bool vsync) override;

    void enterRender(glm::vec2 topLeft, glm::vec2 bottomRight);
    void exitRender();

  protected:
    dynasma::FirmPtr<const ParamList> mp_renderComponents;
    struct FramebufferContextSwitcher
    {
        void enterContext(glm::vec2 topLeft, glm::vec2 bottomRight);
        void exitContext();
        void sync(bool vsync);
        void destroyContext();
        glm::vec2 getSize() const;

        int width, height;
        GLuint glFramebufferId;
    };
    struct WindowContextSwitcher
    {
        void enterContext(glm::vec2 topLeft, glm::vec2 bottomRight);
        void exitContext();
        void sync(bool vsync);
        void destroyContext();
        glm::vec2 getSize() const;

        GLFWwindow *window;

        std::function<void()> onClose;
        std::function<void(glm::vec2 motion, bool bLeft, bool bRight, bool bMiddle)> onDrag;

        glm::vec2 lastPos = {0.0f, 0.0f};
        bool bLeft = false, bRight = false, bMiddle = false;
    };

    std::variant<FramebufferContextSwitcher, WindowContextSwitcher> m_contextSwitcher;
};

} // namespace Vitrae