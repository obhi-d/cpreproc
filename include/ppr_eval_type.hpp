
#pragma once

#include <cstdint>
#include <tuple>

namespace ppr
{

struct eval_type
{
  std::int64_t value       = 0;
  bool         null_type   = false;
  bool         is_unsigned = false;

  eval_type(eval_type const&) = default;
  eval_type(eval_type&&)      = default;
  eval_type& operator=(eval_type const&) = default;
  eval_type& operator=(eval_type&&) = default;

  eval_type(bool v) : value(v), is_unsigned(true) {}
  eval_type(std::int64_t v) : value(v) {}
  eval_type(std::uint64_t v) : value(v), is_unsigned(true) {}
  eval_type() : null_type(true) {}

  std::uint64_t uval() const
  {
    return static_cast<std::uint64_t>(value);
  }

  std::int64_t val() const
  {
    return static_cast<std::int64_t>(value);
  }

#define PPR_BINARY_OP(op)                                                                                              \
  return null_type || o.null_type                                                                                      \
           ? eval_type{}                                                                                               \
           : (is_unsigned || o.is_unsigned ? eval_type{uval() op o.uval()} : eval_type{val() op o.val()})

#define PPR_BINARY_OP_U(op)                                                                                            \
  return null_type || o.null_type ? eval_type{} : eval_type                                                            \
  {                                                                                                                    \
    uval() op o.uval()                                                                                                 \
  }

#define PPR_BINARY_OP_S(op)                                                                                            \
  return null_type || o.null_type ? eval_type{} : eval_type                                                            \
  {                                                                                                                    \
    val() op o.val()                                                                                                   \
  }

#define PPR_UNARY_OP(op) return null_type ? eval_type{} : (is_unsigned ? eval_type{op uval()} : eval_type{op val()})

#define PPR_UNARY_OP_U(op)                                                                                             \
  return null_type ? eval_type{} : eval_type                                                                           \
  {                                                                                                                    \
    op uval()                                                                                                          \
  }
#define PPR_UNARY_OP_S(op)                                                                                             \
  return null_type ? eval_type{} : eval_type                                                                           \
  {                                                                                                                    \
    op val()                                                                                                           \
  }

  // clang-format off
  inline eval_type operator+(eval_type const& o) const
  {
    PPR_BINARY_OP(+);
  }

  inline eval_type operator-(eval_type const& o) const
  {
    PPR_BINARY_OP(-);
  }

  inline eval_type operator*(eval_type const& o) const
  {
    PPR_BINARY_OP(*);
  }

  inline eval_type operator/(eval_type const& o) const
  {
    PPR_BINARY_OP(/);
  }

  inline eval_type operator%(eval_type const& o) const
  {
    PPR_BINARY_OP(%);
  }

  inline eval_type operator<<(eval_type const& o) const
  {
    PPR_BINARY_OP_U(<<);
  }

  inline eval_type operator>>(eval_type const& o) const
  {
    PPR_BINARY_OP_U(>>);
  }

  inline eval_type operator<=(eval_type const& o) const
  {
    PPR_BINARY_OP(<=);
  }

  inline eval_type operator>=(eval_type const& o) const
  {
    PPR_BINARY_OP(>=);
  }

  inline eval_type operator==(eval_type const& o) const
  {
    PPR_BINARY_OP(==);
  }

  inline eval_type operator!=(eval_type const& o) const
  {
    PPR_BINARY_OP(!=);
  }

  inline eval_type operator&&(eval_type const& o) const
  {
    return uval() && o.uval();
  }

  inline eval_type operator||(eval_type const& o) const
  {
    return uval() || o.uval();
  }
  
  inline eval_type operator<(eval_type const& o) const
  {
    PPR_BINARY_OP(<);
  }

  inline eval_type operator>(eval_type const& o) const
  {
    PPR_BINARY_OP(>);
  }

  
  inline eval_type operator!() const
  {
    PPR_UNARY_OP(!);
  }
    
  inline eval_type operator&(eval_type const& o) const
  {
    return uval() & o.uval();
  }

    
  inline eval_type operator|(eval_type const& o) const
  {
    return uval() | o.uval();
  }

  inline eval_type operator^(eval_type const& o) const
  {
    PPR_BINARY_OP_U(^);
  }

  inline operator bool() const
  {
    return uval() != 0;
  }

  inline eval_type operator ~() const
  {
    PPR_UNARY_OP_U(~);
  }

  inline eval_type operator -() const
  {
    PPR_UNARY_OP_S(-);
  }
  // clang-format on
};

} // namespace ppr