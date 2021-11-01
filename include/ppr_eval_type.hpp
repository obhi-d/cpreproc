
#pragma once

#include <cstdint>
#include <tuple>

namespace ppr
{

struct eval_type
{
  std::tuple<std::uint64_t, std::int64_t, double, std::uint8_t> value;

  eval_type()                 = default;
  eval_type(eval_type const&) = default;
  eval_type(eval_type&&)      = default;
  eval_type& operator=(eval_type const&) = default;
  eval_type& operator=(eval_type&&) = default;

  eval_type(std::uint64_t v) : value((std::uint64_t)v, (std::int64_t)v, (double)v, 0) {}
  eval_type(std::int64_t v) : value((std::uint64_t)v, (std::int64_t)v, (double)v, 1) {}
  eval_type(double v) : value((std::uint64_t)v, (std::int64_t)v, (double)v, 2) {}
  eval_type(std::uint64_t a, std::int64_t b, double c, std::uint8_t d) : value(a, b, c, d) {}

  // clang-format off
  inline eval_type operator+(eval_type const& o) const
  {
    return
    eval_type{
      std::get<0>(value) + std::get<0>(o.value), 
      std::get<1>(value) + std::get<1>(o.value),
      std::get<2>(value) + std::get<2>(o.value), 
      std::max(std::get<3>(value), std::get<3>(o.value))
    };
  }


  inline eval_type operator-(eval_type const& o) const
  {
    return
    eval_type{
      std::get<0>(value) - std::get<0>(o.value), 
      std::get<1>(value) - std::get<1>(o.value),
      std::get<2>(value) - std::get<2>(o.value), 
      std::max(std::get<3>(value), std::get<3>(o.value))
    };
  }

  inline eval_type operator*(eval_type const& o) const
  {
    return
    eval_type{
      std::get<0>(value) * std::get<0>(o.value), 
      std::get<1>(value) * std::get<1>(o.value),
      std::get<2>(value) * std::get<2>(o.value), 
      std::max(std::get<3>(value), std::get<3>(o.value))
    };
  }

  inline eval_type operator/(eval_type const& o) const
  {
    return
    eval_type{
      std::get<0>(value) / std::get<0>(o.value), 
      std::get<1>(value) / std::get<1>(o.value),
      std::get<2>(value) / std::get<2>(o.value), 
      std::max(std::get<3>(value), std::get<3>(o.value))
    };
  }

  inline eval_type operator<<(eval_type const& o) const
  {
    auto v = std::get<0>(value) << std::get<0>(o.value);
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator>>(eval_type const& o) const
  {
    auto v = std::get<0>(value) >> std::get<0>(o.value);
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator<=(eval_type const& o) const
  {
    std::uint64_t v = std::get<2>(value) <= std::get<2>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator>=(eval_type const& o) const
  {
    std::uint64_t v = std::get<2>(value) >= std::get<2>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator==(eval_type const& o) const
  {
    std::uint64_t v;
    if (std::get<3>(value) == 2 || std::get<3>(o.value) == 2)
      v = std::get<2>(value) == std::get<2>(o.value) ? 1 : 0;
    else
      v = std::get<0>(value) == std::get<0>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator!=(eval_type const& o) const
  {
    std::uint64_t v;
    if (std::get<3>(value) == 2 || std::get<3>(o.value) == 2)
      v = std::get<2>(value) != std::get<2>(o.value) ? 1 : 0;
    else
      v = std::get<0>(value) != std::get<0>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator&&(eval_type const& o) const
  {
    std::uint64_t v = std::get<0>(value) && std::get<0>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator||(eval_type const& o) const
  {
    std::uint64_t v = std::get<0>(value) || std::get<0>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }
  
  inline eval_type operator<(eval_type const& o) const
  {
    std::uint64_t v = std::get<2>(value) < std::get<2>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator>(eval_type const& o) const
  {
    std::uint64_t v = std::get<2>(value) > std::get<2>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  
  inline eval_type operator!() const
  {
    std::uint64_t v = !std::get<0>(value);
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }
    
  inline eval_type operator&(eval_type const& o) const
  {
    std::uint64_t v = std::get<0>(value) & std::get<0>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

    
  inline eval_type operator|(eval_type const& o) const
  {
    std::uint64_t v = std::get<0>(value) | std::get<0>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline eval_type operator^(eval_type const& o) const
  {
    std::uint64_t v = std::get<0>(value) ^ std::get<0>(o.value) ? 1 : 0;
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  inline operator bool() const
  {
    return std::get<0>(value) || std::get<1>(value) || (std::get<2>(value) != 0.0);
  }

  friend inline eval_type operator ~(eval_type const& o)
  {
    std::uint64_t v = ~std::get<0>(o.value);
    return
    eval_type{
      v, 
      (std::int64_t)v,
      (double)v, 
      0
    };
  }

  friend inline eval_type operator -(eval_type const& o)
  {
    return
    eval_type{
      -std::get<0>(o.value), 
      -std::get<1>(o.value),
      -std::get<2>(o.value), 
      std::get<3>(o.value)
    };
  }
  // clang-format on
};

} // namespace ppr