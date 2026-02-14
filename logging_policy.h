#pragma once

#include <Arduino.h>

// Runtime/build-time logging policy.
enum dbg_log_level_t : uint8_t {
  DBG_LOG_OFF = 0,
  DBG_LOG_ERROR,
  DBG_LOG_WARN,
  DBG_LOG_INFO,
  DBG_LOG_TRACE,
};

#ifndef DBG_BUILD_LOG_LEVEL
  #ifdef NDEBUG
    #define DBG_BUILD_LOG_LEVEL DBG_LOG_WARN
  #else
    #define DBG_BUILD_LOG_LEVEL DBG_LOG_INFO
  #endif
#endif

#ifndef DBG_RUNTIME_LOG_LEVEL_DEFAULT
  #define DBG_RUNTIME_LOG_LEVEL_DEFAULT DBG_BUILD_LOG_LEVEL
#endif

// Single shared runtime log level across translation units.
extern dbg_log_level_t g_dbg_runtime_log_level;

static inline dbg_log_level_t dbg_runtime_log_level() {
  return g_dbg_runtime_log_level;
}

static inline void dbg_set_runtime_log_level(dbg_log_level_t level) {
  if (level > DBG_LOG_TRACE) level = DBG_LOG_TRACE;
  g_dbg_runtime_log_level = level;
}

#define DBG_LOG_ENABLED(level) ((level) <= dbg_runtime_log_level())

#define DBG_LOGE(fmt, ...) do { if (DBG_LOG_ENABLED(DBG_LOG_ERROR)) Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__); } while (0)
#define DBG_LOGW(fmt, ...) do { if (DBG_LOG_ENABLED(DBG_LOG_WARN))  Serial.printf("[WARN] "  fmt "\n", ##__VA_ARGS__); } while (0)
#define DBG_LOGI(fmt, ...) do { if (DBG_LOG_ENABLED(DBG_LOG_INFO))  Serial.printf("[INFO] "  fmt "\n", ##__VA_ARGS__); } while (0)
#define DBG_LOGT(fmt, ...) do { if (DBG_LOG_ENABLED(DBG_LOG_TRACE)) Serial.printf("[TRACE] " fmt "\n", ##__VA_ARGS__); } while (0)
