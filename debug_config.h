#pragma once

// Runtime logging defaults (build profile can override with DBG_BUILD_LOG_LEVEL).
#ifndef DBG_RUNTIME_LOG_LEVEL_DEFAULT
  #define DBG_RUNTIME_LOG_LEVEL_DEFAULT 3  // DBG_LOG_INFO
#endif

// Use custom Orbitron fonts (LVGL v8 font .c files must be in your sketch folder)
#define USE_ORBITRON          1
