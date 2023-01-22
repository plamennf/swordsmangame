#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t  s64;
typedef int32_t  s32;
typedef int16_t  s16;
typedef int8_t   s8;

// Copy-paste from https://gist.github.com/andrewrk/ffb272748448174e6cdb4958dae9f3d8
// Defer macro/thing.

#define CONCAT_INTERNAL(x,y) x##y
#define CONCAT(x,y) CONCAT_INTERNAL(x,y)

template<typename T>
struct ExitScope {
    T lambda;
    ExitScope(T lambda):lambda(lambda){}
    ~ExitScope(){lambda();}
    ExitScope(const ExitScope&);
  private:
    ExitScope& operator =(const ExitScope&);
};
 
class ExitScopeHelp {
  public:
    template<typename T>
        ExitScope<T> operator+(T t){ return t;}
};
 
#define defer const auto& CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()

#define ArrayCount(array) (sizeof(array)/sizeof((array)[0]))
#define Square(x) ((x)*(x))
#define Min(x, y) ((x) < (y) ? (x) : (y))
#define Max(x, y) ((x) > (y) ? (x) : (y))
//#define Clamp(x, a, b) if ((x) < (a)) (x) = (a); if ((x) > (b)) (x) = (b);

template <typename T>
inline T Clamp(T x, T a, T b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

#define Kilobytes(x) ((x)*1024ULL)
#define Megabytes(x) (Kilobytes(x)*1024ULL)
#define Gigabytes(x) (Megabytes(x)*1024ULL)
#define Terabytes(x) (Gigabytes(x)*1024ULL)

const float PI = 3.14159265359f;
const float TAU = 6.28318530718f;

struct Memory_Arena {
    s64 occupied = 0;
    s64 size = 0;
    u8 *data = 0;

    void init(s64 size);
    void *get(s64 size);
};

void init_temporary_storage(s64 size);
void reset_temporary_storage();
s64 get_temporary_storage_mark();
void set_temporary_storage_mark(s64 mark);
void *talloc(s64 mark, s64 alignment = 8);

char *sprint(char *fmt, ...);
char *sprint_valist(char *fmt, va_list args);
char *tprint(char *fmt, ...);
char *tprint_valist(char *fmt, va_list args);

s64 string_length(char *s);
char *copy_string(char *s, bool use_temporary_storage = false);
bool strings_match(char *a, char *b);
int get_codepoint(char *text, int *bytes_processed);

char *eat_spaces(char *s);
bool starts_with(char *string, char *substring);
char *consume_next_line(char **text_ptr);

float fract(float value);

char *concatenate_with_newlines(char **array, s64 array_count, bool use_temporary_storage = false);
char *concatenate_with_commas(char **array, s64 array_count, bool use_temporary_storage = false);
char *concatenate(char *a, char *b);

void log(char *string, ...);
void log_error(char *string, ...);
void print(char *string, ...);

char *find_character_from_left(char *string, char c);
char *eat_trailing_spaces(char *string);
bool contains(char *string, char *substring);

char *replace_backslash_with_forwardslash(char *string);
char *replace_forwardslash_with_backslash(char *string);

char *lowercase(char *string);
char *copy_strip_extension(char *filename, bool use_temporary_storage = false);

float round_to_two_decimal_places(float var);
