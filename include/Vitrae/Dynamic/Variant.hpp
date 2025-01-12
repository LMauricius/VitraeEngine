#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"

#include <any>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>

namespace Vitrae
{

class Variant;

/**
 * A structure that contains type info and function pointers to compare and convert properties.
 * Used to implement type-specific functionality in a generic way.
 * Similar to a vtable, except for Variant type
 */
class VariantVTable
{
    friend Variant;

    bool hasShortObjectOptimization; // prefer first place in memory

  public:
    const TypeInfo *p_typeinfo;
    std::size_t size;
    std::size_t alignment;

    constexpr VariantVTable()
        : p_typeinfo(nullptr), size(0), alignment(0), hasShortObjectOptimization(false),
          emptyConstructor(nullptr), copyConstructor(nullptr), moveConstructor(nullptr),
          destructor(nullptr), isEqual(nullptr), isLessThan(nullptr), toBool(nullptr),
          toString(nullptr), hash(nullptr) {};
    ~VariantVTable() = default;

  protected:
    constexpr VariantVTable(const VariantVTable &) = default;
    constexpr VariantVTable(VariantVTable &&) = default;
    VariantVTable &operator=(const VariantVTable &) = default;
    VariantVTable &operator=(VariantVTable &&) = default;

    bool operator==(const VariantVTable &o) const { return *p_typeinfo == *o.p_typeinfo; }
    bool operator!=(const VariantVTable &o) const { return *p_typeinfo != *o.p_typeinfo; }
    bool operator<(const VariantVTable &o) const { return *p_typeinfo < *o.p_typeinfo; }
    bool operator>(const VariantVTable &o) const { return *p_typeinfo > *o.p_typeinfo; }
    bool operator<=(const VariantVTable &o) const { return *p_typeinfo <= *o.p_typeinfo; }
    bool operator>=(const VariantVTable &o) const { return *p_typeinfo >= *o.p_typeinfo; }

    // memory management
    void (*emptyConstructor)(Variant &self);
    void (*copyConstructor)(Variant &self, const Variant &other);
    void (*moveConstructor)(Variant &self, Variant &other);
    void (*destructor)(Variant &self);

    // type comparison functions
    bool (*isEqual)(const Variant &lhs, const Variant &rhs);
    bool (*isLessThan)(const Variant &lhs, const Variant &rhs);

    // type conversions
    bool (*toBool)(const Variant &p);
    std::string (*toString)(const Variant &p);
    std::size_t (*hash)(const Variant &p);
};

/**
 * @brief A class that allows you to store and retrieve properties of any type.
 * @note Functionally similar to std::any, except with dynamic comparison method support
 */
class Variant
{
  private:
    // vtable variable and function

    /**
     * @brief Creates a PropertyFuncTable with the function pointers for the specified type.
     */
    template <class T> static constexpr VariantVTable makeVTable()
    {
        VariantVTable table;

        // internal
        if constexpr (requires {
                          { sizeof(T) };
                          { alignof(T) };
                      }) {
            table.hasShortObjectOptimization =
                (sizeof(T) <= sizeof(decltype(Variant::m_val)::m_shortBufferVal) &&
                 alignof(T) <= alignof(Variant::m_val));
            table.size = sizeof(T);
            table.alignment = alignof(T);
        } else {
            table.hasShortObjectOptimization = true;
            table.size = 0;
            table.alignment = 0;
        }

        // public info
        table.p_typeinfo = &TYPE_INFO<T>;

        // memory management

        // empty constructor
        if constexpr (requires { new T(); }) {
            table.emptyConstructor = [](Variant &self) { new (&self.getUnsafe<T>()) T(); };
        } else {
            table.emptyConstructor = [](Variant &self) {};
        }

        // copy constructor
        if constexpr (requires(const T &t) { new T(t); }) {
            table.copyConstructor = [](Variant &self, const Variant &other) {
                new (&self.getUnsafe<T>()) T(other.getUnsafe<T>());
            };
        } else {
            table.copyConstructor = [](Variant &self, const Variant &other) {};
        }

        // move constructor
        if constexpr (requires(T &&t) { new T(t); }) {
            table.moveConstructor = [](Variant &self, Variant &other) {
                new (&self.getUnsafe<T>()) T(std::move(other.getUnsafe<T>()));
            };
        } else {
            table.moveConstructor = [](Variant &self, Variant &other) {};
        }

        // destructor
        if constexpr (requires(T v) { v.~T(); }) {
            table.destructor = [](Variant &self) { self.getUnsafe<T>().~T(); };
        } else {
            table.destructor = [](Variant &self) {};
        }

        // operator==
        if constexpr (requires(T lhs, T rhs) {
                          { lhs == rhs } -> std::convertible_to<bool>;
                      }) {
            table.isEqual = [](const Variant &lhs, const Variant &rhs) -> bool {
                return (bool)(lhs.getUnsafe<T>() == rhs.get<T>());
            };
        } else {
            table.isEqual = [](const Variant &lhs, const Variant &rhs) -> bool {
                std::stringstream ss;
                ss << "operator== is not implemented for type " << typeid(T).name();
                throw std::runtime_error(ss.str());
            };
        }

        // operator<
        if constexpr (requires(T lhs, T rhs) {
                          { lhs < rhs } -> std::convertible_to<bool>;
                      }) {
            table.isLessThan = [](const Variant &lhs, const Variant &rhs) -> bool {
                return (bool)(lhs.getUnsafe<T>() < rhs.get<T>());
            };
        } else {
            table.isLessThan = [](const Variant &lhs, const Variant &rhs) -> bool {
                std::stringstream ss;
                ss << "operator< is not implemented for type " << typeid(T).name();
                throw std::runtime_error(ss.str());
            };
        }

        // operator (bool)
        if constexpr (requires(T v) {
                          { bool(v) } -> std::convertible_to<bool>;
                      }) {
            table.toBool = [](const Variant &p) -> bool { return (bool)(p.getUnsafe<T>()); };
        } else {
            table.toBool = [](const Variant &p) -> bool {
                std::stringstream ss;
                ss << "operator bool is not implemented for type " << typeid(T).name();
                throw std::runtime_error(ss.str());
            };
        }

        // operator (std::string)
        if constexpr (requires(T v, std::stringstream ss) {
                          { ss << v };
                      }) {
            table.toString = [](const Variant &p) -> std::string {
                std::stringstream ss;
                ss << p.getUnsafe<T>();
                return ss.str();
            };
        } else {
            table.toString = [](const Variant &p) -> std::string {
                std::stringstream ss;
                ss << "operator std::string is not implemented for type " << typeid(T).name();
                throw std::runtime_error(ss.str());
            };
        }

        // hash function
        if constexpr (requires(T v) {
                          { std::hash<T>{}(v) } -> std::convertible_to<std::size_t>;
                      }) {
            table.hash = [](const Variant &p) -> std::size_t {
                return std::hash<T>{}(p.getUnsafe<T>());
            };
        } else {
            table.hash = [](const Variant &p) -> std::size_t {
                std::stringstream ss;
                ss << "std::hash is not implemented for type " << typeid(T).name();
                throw std::runtime_error(ss.str());
            };
        }

        return table;
    }

    /**
     * @brief Contains a PropertyFuncTable with the function pointers for the specified type.
     */
    template <class T> static constexpr VariantVTable V_TABLE = makeVTable<T>();

  public:
    // static functions

    /**
     * @returns a compile-time reference to the PropertyFuncTable for the specified type.
     */
    template <class T> static constexpr const TypeInfo &getTypeInfo()
    {
        return TYPE_INFO<std::decay_t<T>>;
    }

    // constructors

    /// @brief default constructor
    Variant() : m_val(), m_table(&V_TABLE<void>) {}

    /// @brief constructor with a value
    template <class T> Variant(T val) : m_table(&V_TABLE<std::decay_t<T>>)
    {
        allocateBuffer<T>();
        new (&get<T>()) T(val);
    }
    /// @brief copy constructor
    inline Variant(const Variant &other) : m_table(other.m_table)
    {
        allocateTBuffer(m_table);
        m_table->copyConstructor(*this, other);
    }
    /// @brief move constructor
    inline Variant(Variant &&other) : m_table(other.m_table)
    {
        allocateTBuffer(m_table);
        m_table->moveConstructor(*this, other);
    }

    /// @brief destructor
    ~Variant()
    {
        m_table->destructor(*this);
        freeBuffer(m_table);
    }

    // assignment operators

    /// @brief assignment operator with a value
    template <class T> Variant &operator=(T val)
    {
        m_table->destructor(*this);

        reallocateBuffer<T>(m_table);

        m_table = &V_TABLE<std::decay_t<T>>;
        new (&get<T>()) T(val);

        return *this;
    }
    /// @brief assignment operator
    inline Variant &operator=(const Variant &other)
    {
        m_table->destructor(*this);

        reallocateTBuffer(m_table, other.m_table);

        m_table = other.m_table;
        m_table->copyConstructor(*this, other);

        return *this;
    }
    /// @brief move assignment
    inline Variant &operator=(Variant &&other)
    {
        m_table->destructor(*this);

        reallocateTBuffer(m_table, other.m_table);

        m_table = other.m_table;
        m_table->moveConstructor(*this, other);

        return *this;
    }

    inline const TypeInfo &getAssignedTypeInfo() const { return *m_table->p_typeinfo; }

    // getter

    /**
     * @tparam T The type to retrieve the value as.
     * @return The value stored  as type `T`.
     * @throws std::bad_any_cast If the stored value isn't of exact type `T`.
     */
    template <class T> constexpr T &get()
    {
        if (*m_table != V_TABLE<std::decay_t<T>>) {
            throw std::bad_any_cast();
        }
        return getUnsafe<T>();
    }
    template <class T> constexpr const T &get() const
    {
        if (*m_table != V_TABLE<std::decay_t<T>>) {
            throw std::bad_any_cast();
        }
        return getUnsafe<T>();
    }

    // comparison operators

    /**
     * @param other The Variant object to compare with.
     * @return Whether the two Variant objects are equal.
     * @throws std::bad_any_cast If the two Variant objects have different types.
     * @throws std::runtime_error If the stored type is not comparable.
     */
    constexpr bool operator==(const Variant &o) const { return m_table->isEqual(*this, o); }
    /**
     * @param other The Variant object to compare with.
     * @return Whether the first Variant object is less than the second.
     * @throws std::bad_any_cast If the two Variant objects have different types.
     * @throws std::runtime_error If the stored type is not comparable.
     */
    constexpr bool operator<(const Variant &o) const { return m_table->isLessThan(*this, o); }
    /**
     * @return Whether the stored value is truthy.
     * @throws std::runtime_error If the stored type is not convertible to bool.
     */
    constexpr operator bool() const { return m_table->toBool(*this); }
    /**
     * @returns A string representation of the stored value.
     * @throws std::runtime_error If the stored type is not convertible to std::string.
     */
    std::string toString() const { return m_table->toString(*this); }
    /**
     * @returns A hash value for the stored value.
     * @throws std::runtime_error If the stored type is not hashable.
     */
    constexpr std::size_t hash() const { return m_table->hash(*this) ^ (std::size_t)m_table; }

  private:
    // member variables

    /**
     * Underlying storage for any type
     */
    union {
        void *mp_longVal;
        char m_shortBufferVal[sizeof(void *) * 2]; // size of two pointers, because many stored
                                                   // variant data consists of 2 pointers
    } m_val;
    /**
     * Pointer to the function table
     */
    const VariantVTable *m_table;

    template <class T> constexpr T &getUnsafe()
    {
        return *reinterpret_cast<T *>(V_TABLE<std::decay_t<T>>.hasShortObjectOptimization
                                          ? (void *)m_val.m_shortBufferVal
                                          : m_val.mp_longVal);
    }
    template <class T> constexpr const T &getUnsafe() const
    {
        return *reinterpret_cast<const T *>(V_TABLE<std::decay_t<T>>.hasShortObjectOptimization
                                                ? (const void *)m_val.m_shortBufferVal
                                                : m_val.mp_longVal);
    }

    // buffer management

    inline void freeBuffer(const VariantVTable *oldt)
    {
        if (!oldt->hasShortObjectOptimization) {
            std::free(m_val.mp_longVal);
        }
    }

    inline constexpr void reallocateTBuffer(const VariantVTable *oldt, const VariantVTable *newt)
    {
        if (!oldt->hasShortObjectOptimization) {
            if (!newt->hasShortObjectOptimization) {
                if (newt->size != oldt->size) {
                    m_val.mp_longVal = std::realloc(m_val.mp_longVal, newt->size);
                }
            } else {
                std::free(m_val.mp_longVal);
            }
        } else {
            if (!newt->hasShortObjectOptimization) {
                m_val.mp_longVal = std::malloc(newt->size);
            }
        }
    }

    inline constexpr void allocateTBuffer(const VariantVTable *newt)
    {
        if (!newt->hasShortObjectOptimization) {
            m_val.mp_longVal = std::malloc(newt->size);
        }
    }

    template <class T> void reallocateBuffer(const VariantVTable *oldt)
    {
        reallocateTBuffer(oldt, &V_TABLE<std::decay_t<T>>);
    }

    template <class T> void allocateBuffer() { allocateTBuffer(&V_TABLE<std::decay_t<T>>); }
};

} // namespace Vitrae

namespace std
{
template <> struct hash<Vitrae::Variant>
{
    size_t operator()(const Vitrae::Variant &x) const { return x.hash(); }
};
} // namespace std