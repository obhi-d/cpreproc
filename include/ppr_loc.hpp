
#pragma once

#include <cstdint>

namespace ppr
{

  struct loc
  {
    std::int32_t line = 0;
    std::int32_t column = 0;
  };
}