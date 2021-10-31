
#pragma once

#include <cstdint>
#include <variant>

namespace cpreproc
{

  struct eval_type
  {
    std::variant<std::uint64_t, std::int64_t, double, bool, std::monostate> value;

    eval_type() = default;
    eval_type(eval_type const&) = default;
    eval_type(eval_type&&) = default;
    eval_type& operator=(eval_type const&) = default;
    eval_type& operator=(eval_type&&) = default;
    
    eval_type(bool v) : value(v) {}
    eval_type(std::uint64_t v) : value(v) {}
    eval_type(std::int64_t v) : value(v) {}
    eval_type(double v) : value(v) {}
  };

}