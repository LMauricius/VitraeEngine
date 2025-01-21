#pragma once

#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Util/NonCopyable.hpp"

#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace Vitrae
{
class Renderer;

using BufferUsageHints = std::uint8_t;
struct BufferUsageHint
{
    static constexpr BufferUsageHints HOST_INIT = 1 << 0;
    static constexpr BufferUsageHints HOST_WRITE = 1 << 1;
    static constexpr BufferUsageHints HOST_READ = 1 << 2;
    static constexpr BufferUsageHints GPU_DRAW = 1 << 3;
    static constexpr BufferUsageHints GPU_COMPUTE = 1 << 4;
};

/**
 * A shared buffer is a data storage object that can easily be shared between CPU and GPU.
 * The raw shared buffer can be used to store any type of data.
 */
class RawSharedBuffer : public dynasma::PolymorphicBase
{
  public:
    struct SetupParams
    {
        Renderer &renderer;
        ComponentRoot &root;
        BufferUsageHints usage = BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW;
        std::size_t size = 0;
        String friendlyName = "";
    };

    virtual ~RawSharedBuffer() = default;

    virtual void synchronize() = 0;
    virtual bool isSynchronized() const = 0;
    virtual std::size_t memory_cost() const = 0;

    void resize(std::size_t size);

    inline std::size_t size() const { return m_size; }
    const Byte *data() const;
    Byte *data();

    std::span<const Byte> operator[](std::pair<std::size_t, std::size_t> slice) const;
    std::span<Byte> operator[](std::pair<std::size_t, std::size_t> slice);

  protected:
    /// @brief The current specified size of the buffer
    std::size_t m_size;
    /// @brief A pointer to the underlying buffer. nullptr if it needs to be requested
    mutable Byte *m_bufferPtr;
    /// @brief  The range of the buffer that needs to be synchronized
    mutable std::pair<std::size_t, std::size_t> m_dirtySpan;

    RawSharedBuffer();

    /**
     * @brief This function is called only while m_bufferPtr is nullptr, and has to set m_bufferPtr
     * to valid m_size long span of memory
     */
    virtual void requestBufferPtr() const = 0;
    /**
     * @brief This function is called before m_size gets changed, and ensures m_bufferPtr is
     * either nullptr or points to a valid m_size long span of memory
     */
    virtual void requestResizeBuffer(std::size_t size) const = 0;
};

struct RawSharedBufferKeeperSeed
{
    using Asset = RawSharedBuffer;
    std::variant<RawSharedBuffer::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using RawSharedBufferKeeper = dynasma::AbstractKeeper<RawSharedBufferKeeperSeed>;

} // namespace Vitrae