#pragma once

#include "Vitrae/Data/GlobalConst.hpp"

#include <concepts>
#include <memory>
#include <utility>
#include <variant>

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
 * A wrapper type of references to ParamAttribute values,
 * That can be constructred from a list of attributes
 */
class AttributeWrapper
{
    // Pointer is used if we take the global value, shared_ptr for dynamicly constructed values
    std::shared_ptr<const PolymorphicParamAttribute> p_attributes;

  public:
    AttributeWrapper(AttributeWrapper &&) = default;
    AttributeWrapper(const AttributeWrapper &) = default;

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
    AttributeWrapper(CompAttrT &&...attr)
        : p_attributes{
              std::make_shared<const CompoundParamAttribute<CompAttrT...>>(
                  std::forward<CompAttrT...>(attr)...),
          }
    /*std::make_shared<const CompoundParamAttribute<CompAttrT...>>(
      std::forward<CompAttrT...>(attr)...)*/
    {}

    AttributeWrapper &operator=(const AttributeWrapper &) = default;
    AttributeWrapper &operator=(AttributeWrapper &&) = default;

    /**
     * @tparam AttrT The type of the attribute we want
     * @returns Pointer to the attribute object of type AttrT if it is set, nullptr otherwise
     * @note AttrT is set if the attribute passed to this wrapper derives from AttrT
     * @note Attribute types can be added to the contained object through this type's constructor
     */
    template <class AttrT> constexpr const AttrT *p_attribute() const
    {
        return dynamic_cast<const AttrT *>(p_attributes.get());
    }
};

} // namespace Vitrae