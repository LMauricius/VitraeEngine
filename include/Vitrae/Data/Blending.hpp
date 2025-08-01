#pragma once

#include <compare>

namespace Vitrae
{

/**
 * The blending operation that combines source and destination colors
 */
enum class BlendingOperation {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

/**
 * The color factors that can be used when blending
 */
enum class BlendingFactor {
    Zero,
    One,
    SourceColor,
    OneMinusSourceColor,
    DestinationColor,
    OneMinusDestinationColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationAlpha,
    OneMinusDestinationAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SourceAlphaSaturated,
};

struct BlendingDescription
{
    BlendingOperation operationRGB;
    BlendingOperation operationAlpha;
    BlendingFactor sourceRGB;
    BlendingFactor sourceAlpha;
    BlendingFactor destinationRGB;
    BlendingFactor destinationAlpha;

    /**
     * Full constructor
     */
    inline constexpr BlendingDescription(BlendingOperation operationRGB,
                                         BlendingOperation operationAlpha, BlendingFactor sourceRGB,
                                         BlendingFactor sourceAlpha, BlendingFactor destinationRGB,
                                         BlendingFactor destinationAlpha)
        : operationRGB(operationRGB), operationAlpha(operationAlpha), sourceRGB(sourceRGB),
          sourceAlpha(sourceAlpha), destinationRGB(destinationRGB),
          destinationAlpha(destinationAlpha)
    {}

    /**
     * Simplified constructor that uses same factors for RGB and Alpha components
     */
    inline constexpr BlendingDescription(BlendingOperation operation, BlendingFactor source,
                                         BlendingFactor destination)
        : operationRGB(operation), operationAlpha(operation), sourceRGB(source),
          sourceAlpha(source), destinationRGB(destination), destinationAlpha(destination)
    {}

    BlendingDescription(const BlendingDescription &other) = default;
    BlendingDescription(BlendingDescription &&other) = default;
    BlendingDescription &operator=(const BlendingDescription &other) = default;
    BlendingDescription &operator=(BlendingDescription &&other) = default;

    bool operator==(const BlendingDescription &other) const = default;
    auto operator<=>(const BlendingDescription &other) const = default;
};

namespace BlendingCommon
{

// Common blending modes

constexpr BlendingDescription None{BlendingOperation::Add, BlendingFactor::One,
                                   BlendingFactor::Zero};
constexpr BlendingDescription Alpha{BlendingOperation::Add, BlendingFactor::SourceAlpha,
                                    BlendingFactor::OneMinusSourceAlpha};
constexpr BlendingDescription PremultipliedAlpha{
    BlendingOperation::Add,
    BlendingOperation::Add,
    BlendingFactor::One,
    BlendingFactor::SourceAlpha,
    BlendingFactor::OneMinusSourceAlpha,
    BlendingFactor::OneMinusSourceAlpha,
};
constexpr BlendingDescription Additive{BlendingOperation::Add, BlendingFactor::One,
                                       BlendingFactor::One};
constexpr BlendingDescription Multiplicative{BlendingOperation::Add, BlendingFactor::Zero,
                                             BlendingFactor::SourceColor};

} // namespace BlendingCommon

} // namespace Vitrae