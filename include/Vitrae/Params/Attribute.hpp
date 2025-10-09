#pragma once

namespace Vitrae
{

/**
 * @brief A polymorphic base class of all ParamAttribute classes
 * @note It isn't a pure virtual class, thus it can be instanced as-is
 */
struct PolymorphicParamAttribute
{
    virtual ~PolymorphicParamAttribute() = default;
};

/**
 * @brief A shortcut for defining a ParamAttribute class that inherits from multiple Metas,
 * along with PolymorphicParamAttribute
 */
template <class... CompAttrT>
struct CompoundParamAttribute : public PolymorphicParamAttribute, public CompAttrT...
{
    constexpr CompoundParamAttribute(const CompAttrT &&...attrInit)
        : PolymorphicParamAttribute(), CompAttrT(attrInit)...
    {}
};

/**
 * An inline global variable holding a CompoundParamAttribute value for ensuring maximum lifetime
 */
template <auto... ATTR_VALUE> constexpr CompoundParamAttribute GLOBAL_ATTRIBUTE = {ATTR_VALUE...};

/**
 * A wrapper type of references to ParamAttribute values,
 * That can be constructred from a list of attributes
 */
class AttributeWrapper
{
    const PolymorphicParamAttribute *p_attributes;

  public:
    constexpr AttributeWrapper(AttributeWrapper &&) = default;
    constexpr AttributeWrapper(const AttributeWrapper &) = default;
    constexpr AttributeWrapper(const PolymorphicParamAttribute &attr) : p_attributes(&attr) {}

    /**
     * A constructor that takes ParamAttribute values and constructs a reference to a global
     * PolymorphicParamAttribute value that holds all these ParamAttribute values
     *
     * @example @code
     *  AttributeWrapper attr = {Default{0}, ShadingStage{ShadingDataStage::SHAPE}};
     *
     * ParamSpec spec = {
     *      .name = "someParameter",
     *      .typeInfo = TYPE_INFO<int>,
     *      .attributes{
     *          Default{0},
     *          InherentShadingStage{ShadingStage::SHAPE},
     *      },
     * };
     * @endcode
     */
    template <class... CompAttrT>
    consteval AttributeWrapper(const CompAttrT &...attr) : p_attributes{&GLOBAL_ATTRIBUTE<attr...>}
    {}

    constexpr AttributeWrapper &operator=(const AttributeWrapper &) = default;
    constexpr AttributeWrapper &operator=(AttributeWrapper &&) = default;
};

} // namespace Vitrae