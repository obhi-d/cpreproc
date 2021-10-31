

#pragma once

#include <string>
#include <string_view>
#include <istream>
#include <ostream>
#include <cstdint>
#include <cpreproc_context.hpp>

namespace cpreproc
{
  class impl
  {
  public:
    static constexpr std::uint32_t empty_lines = 1;
    static constexpr std::uint32_t delete_comments = 2;
    static constexpr std::uint32_t debug = 2;
    static constexpr std::uint32_t no_replace = 2;

    // @remarks Evaluate a conditon after #if and return true or false
    bool evaluate_condition(std::string_view condition) const;

    // @remarks Preprocess a code segment, must be complete (no missing #endif)
    // This will modify the internal macro table
    void preprocess_segment(std::ostream& output, std::string_view segment);
  };
}
