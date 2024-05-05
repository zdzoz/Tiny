#include <cinttypes>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>



#define LOG_ERROR(...) (fprintf(stderr, __VA_ARGS__))
#define LOG_WARN(...) (fprintf(stdout,  __VA_ARGS__))

#define INFO(...) (fprintf(stdout, "Info: " __VA_ARGS__))
#define WARN(...) (LOG_WARN("Warning: " __VA_ARGS__))
#define TODO(...)                               \
    do {                                        \
        fprintf(stderr, "[TODO] " __VA_ARGS__); \
    } while (0)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
