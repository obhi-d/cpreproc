
#pragma once

#include <cstdint>

namespace cpreproc
{

  struct location
  {
    std::uint32_t line = 0;
    std::uint32_t column = 0;
  };
}