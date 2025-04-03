#if !defined(_CPPCAML_UTILS_H_)
#define _CPPCAML_UTILS_H_

#include <array>

namespace Cppcaml
{

void __attribute__((noreturn)) caml_failwith_printf(const char * fmt, ...)
{
  int len = 0;
  va_list args;
  va_start(args, fmt);
  len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char* buf = (char*)alloca(len+1);
  va_start(args, fmt);
  vsnprintf(buf, len+1, fmt, args);
  va_end(args);
  caml_failwith(buf);
}

template<typename T>
struct always_false : std::false_type {};

/* https://stackoverflow.com/a/75619411
 */ 
template<size_t ...Len>
constexpr auto cat(const std::array<char,Len>&...strings){
  constexpr size_t N = (... + Len) - sizeof...(Len);
  std::array<char, N + 1> result = {};
  result[N] = '\0';

  auto it = result.begin();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
  (void)((it = std::copy_n(strings.cbegin(), Len-1, it), 0), ...);
#pragma GCC diagnostic pop
  return result;
}

using std::to_array;

template<typename T, size_t L>
constexpr const std::array<T,L>& to_array(const std::array<T,L>& a) { return a; }

template<typename ...Ts>
constexpr auto cat(const Ts&...strings){
  return cat(to_array(strings)...);
}

template<typename ...Ts>
static constexpr auto wrap_paren(const Ts&...strings){
  return cat("(", strings..., ")");
}

template<typename T, std::size_t N, typename F>
constexpr auto map_array(const std::array<T, N>& arr, F&& f) {
    using result_type = std::array<
        std::remove_cvref_t<std::invoke_result_t<F&, const T&>>,
        N
    >;
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return result_type{ f(arr[I])... };
    }(std::make_index_sequence<N>());
}

template<auto s> struct StaticString {
  static constexpr const auto va = s;
  static constexpr auto value_len = va.size();
  static constexpr const char * value = &va[0];
};


struct __attribute__((packed))
StaticCamlValueBase
{
  header_t header;

  constexpr StaticCamlValueBase(header_t h) : header(h)
  {
  }

  constexpr explicit operator value() const
  {
    return reinterpret_cast<value>(&header+1);
  }
};


template<uint8_t tag, size_t size>
struct __attribute__((packed))
StaticCamlValue
: StaticCamlValueBase
{
  constexpr StaticCamlValue() : StaticCamlValueBase(Caml_out_of_heap_header(size, tag)) {}

};

constexpr size_t caml_string_wosize(size_t len)
{
  return (len + sizeof(value))/sizeof(value);
}

template<auto s>
struct __attribute__((packed))
  StaticCamlString : StaticCamlValue<String_tag, caml_string_wosize(s.size() - 1)>
{
  static constexpr auto size_no_null = s.size() - 0;
  static constexpr auto wosize = caml_string_wosize(size_no_null);

  const std::array<char, s.size()> string_value = to_array(s);

  const std::array<uint8_t, wosize * sizeof(value) - size_no_null - 1>
    padding = {};

  const uint8_t final_char = sizeof(value) - (size_no_null % sizeof(value));
};

template <typename Tuple>
constexpr auto map_tuple_to_value_array(Tuple&& t) {
  return std::apply(
    [](auto&&... elements) {
      return std::array{
        (value)(std::forward<decltype(elements)>(elements))...
        };
    },
    std::forward<Tuple>(t)
  );
}

template<typename... T>
struct __attribute__((packed))
  StaticCamlArray : StaticCamlValue<0, sizeof...(T)>
{
  static const constexpr std::tuple<T...> array_value_raw = {};
  const std::array<value, sizeof...(T)> array_value =
    map_tuple_to_value_array(array_value_raw);
};

StaticCamlValue<1,0> testCamlValue;

static const StaticCamlString<to_array("hello")> testCamlString;

static const StaticCamlArray
< StaticCamlString<to_array("1")>
, StaticCamlString<to_array("12")>
, StaticCamlString<to_array("123")>
, StaticCamlString<to_array("1234")>
, StaticCamlString<to_array("12345")>
, StaticCamlString<to_array("123456")>
, StaticCamlString<to_array("1234567")>
, StaticCamlString<to_array("12345678")>
, StaticCamlString<to_array("123456789")>
 > testCamlArray;

}


#endif
