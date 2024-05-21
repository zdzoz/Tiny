#include <cassert>
#include <cinttypes>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#ifndef NDEBUG
#define LOG_ERROR(...) (fprintf(stderr, __VA_ARGS__))
#define LOG_WARN(...) (fprintf(stderr, __VA_ARGS__))
#define INFO(...) (fprintf(stderr, "Info: " __VA_ARGS__))
#define WARN(...) (LOG_WARN("Warning: " __VA_ARGS__))
#define ERROR(...) (LOG_ERROR("Error: " __VA_ARGS__))
#define TODO(...)                               \
    do {                                        \
        fprintf(stderr, "[TODO] " __VA_ARGS__); \
    } while (0)
#else
#define LOG_ERROR(...)
#define LOG_WARN(...)

#define INFO(...)
#define WARN(...)
#define ERROR(...)
#define TODO(...)
#endif
