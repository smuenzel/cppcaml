#if !defined(_CPPCAML_UTILS_H_)
#define _CPPCAML_UTILS_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <array>
#include <algorithm>
#include <tuple>
#include <functional>
#include <span>
#include <string>
#include <string_view>

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/fail.h>

#include <cppcaml/typelist.h>

namespace Cppcaml
{

static void __attribute__((noreturn)) caml_failwith_printf(const char * fmt, ...)
{
  int len = 0;
  va_list args;
  va_start(args, fmt);
  len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  value v_string = caml_alloc_string(len);
  char* buf = (char*)Bytes_val(v_string);
  va_start(args, fmt);
  vsnprintf(buf, len+1, fmt, args);
  va_end(args);
  caml_failwith_value(v_string);
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

template<size_t N, size_t... I>
const constexpr std::array<char, N + 1> make_string_view_array_internal(const std::string_view& view, const std::index_sequence<I...>&) {
  return std::array<char, N + 1>{ view[I]..., 0 };
};

template<size_t N>
const constexpr std::array<char, N + 1> make_string_view_array(std::string_view view) {
  const constexpr auto seq = std::make_index_sequence<N>();
  return make_string_view_array_internal<N>(view, seq);
}

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

template<typename T0, typename T1, std::size_t N, typename F>
constexpr auto map2_array(const std::array<T0, N>& arr0, const std::array<T1, N>& arr1, F&& f) {
    using result_type = std::array<
        std::remove_cvref_t<std::invoke_result_t<F&, const T0&, const T1&>>,
        N
    >;
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return result_type{ f(arr0[I], arr1[I])... };
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

  /*
  StaticCamlValueBase(const StaticCamlValueBase&) = delete;
  */

  constexpr StaticCamlValueBase(header_t h) : header(h)
  {
  }

  constexpr const header_t* start_address() const { return &header + 1; }

  constexpr explicit operator value() const
  {
    return reinterpret_cast<value>(this->start_address());
  }

};
static_assert(sizeof(StaticCamlValueBase) == sizeof(value));

union __attribute__((packed)) ValueUnion
{
  const value as_value;
  const StaticCamlValueBase* as_static_caml_value;

  constexpr ValueUnion(const StaticCamlValueBase* v) : as_static_caml_value(v + 1) {}
};

static_assert(sizeof(ValueUnion) == sizeof(value));


template<typename T>
// requires std::is_base_of_v<StaticCamlValueBase, T>
struct WoSizeC 
{
  static const constexpr size_t v = (sizeof(T) - sizeof(StaticCamlValueBase))/sizeof(value);
};

template<typename T>
using WoSize = typename WoSizeC<T>::v;

template<uint8_t p_tag, size_t p_size>
struct __attribute__((packed))
StaticCamlValue
: public StaticCamlValueBase
{
  static constexpr const auto tag = p_tag;
  static constexpr const auto size = p_size;
  constexpr StaticCamlValue() : StaticCamlValueBase(Caml_out_of_heap_header(size, tag)) {}

};

constexpr size_t caml_string_wosize(size_t len)
{
  return (len + sizeof(value))/sizeof(value);
}

template<size_t tail_length, uint8_t offset_value>
struct __attribute__((packed))
  StaticCamlStringTail
{
  const std::array<uint8_t, tail_length - 1>
    padding = {};

  const uint8_t final_char = offset_value;
};

template<uint8_t offset_value>
struct __attribute__((packed))
  StaticCamlStringTail<1, offset_value> {
  const uint8_t final_char = offset_value;
  };

template<>
struct __attribute__((packed))
  StaticCamlStringTail<0, 0> {
  };

template<auto s>
struct __attribute__((packed))
  StaticCamlString : public StaticCamlValue<String_tag, caml_string_wosize(s.size() - 1)>
{
  static constexpr auto size_no_null = s.size() - 1;
  static constexpr auto wosize = caml_string_wosize(size_no_null);
  static constexpr auto offset_index = Bsize_wsize(wosize) - 1;
  static constexpr auto offset_value = offset_index - size_no_null;
  static constexpr auto tail_length = Bsize_wsize(wosize) - size_no_null - 1;

  const std::array<char, s.size()> string_value = to_array(s);

  const StaticCamlStringTail<tail_length, offset_value> tail = {};
};


template<typename... Tvs>
struct __attribute__((packed))
  StaticCamlList {};

template<>
struct __attribute__((packed))
  StaticCamlList<> {
    constexpr operator value() const { return Val_emptylist; }
  };

template<typename Tv0>
struct __attribute__((packed))
  StaticCamlList<Tv0> : public StaticCamlValue<0,2> {
    static constexpr const Tv0 v = {};

    const header_t* head = v.start_address();
    const value tail = Val_int(0);
  };

template <typename Tv0, typename Tv1, typename... Tvs>
struct __attribute__((packed))
  StaticCamlList<Tv0, Tv1, Tvs...> : public StaticCamlValue<0, 2>
{
  static constexpr const Tv0 v = {};
  static constexpr const StaticCamlList<Tv1, Tvs...> rest = {};

  static constexpr const auto vrest = rest.start_address();

  const header_t* head = v.start_address();
  const header_t* tail = vrest;
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

/*
template<size_t N>
struct __attribute__((packed))
  StaticCamlArray : StaticCamlValue<0, N>
{
  const std::array<value, N> values;
};
*/

template<typename... T>
 struct __attribute__((packed))
  StaticCamlArray : StaticCamlValue<0, sizeof...(T)>
 {
  static const constexpr std::tuple<T...> array_value_raw = {};
  const std::array<value, sizeof...(T)> array_value =
    map_tuple_to_value_array(array_value_raw);
 };

/*
template<auto t>
 struct __attribute__((packed))
  StaticCamlArrayDirect : StaticCamlValue<0, std::tuple_size_v<decltype(t)>>
 {
  static const constexpr auto array_value_raw = t;
  const std::array<value, std::tuple_size_v<decltype(t)>> array_value =
    map_tuple_to_value_array(array_value_raw);
 };
 */

template<size_t N>
struct __attribute__((packed))
  StaticCamlArrayDirect : StaticCamlValue<0, N>
{
  const std::array<value, N> values;
};


// https://devblogs.microsoft.com/oldnewthing/20200713-00/?p=103978
template<typename F> struct FunctionTraits;

template<typename R, typename Arg0, typename... Args>
struct FunctionTraits<R(*)(Arg0, Args...)>
{
  using Pointer = R(*)(Arg0, Args...);
  using RetType = R;
  using ArgTypes = TypeList::TL<Arg0, Args...>;
  using ArgTypesNoFirst = TypeList::TL<Args...>;
  using ArgTypesFirst = Arg0;
  static constexpr std::size_t ArgCount = 1 + sizeof...(Args);
  template<std::size_t N>
    using NthArg = TypeList::Nth<N,ArgTypes>;
  using Sequence = std::index_sequence_for<Arg0, Args...>;
  using SequenceNoFirst = std::index_sequence_for<Args...>;
};

template<typename R>
struct FunctionTraits<R(*)()>
{
  using Pointer = R(*)();
  using RetType = R;
  using ArgTypes = TypeList::TL<>;
  using ArgTypesNoFirst = TypeList::TL<>;
  static constexpr std::size_t ArgCount = 0;
  template<std::size_t N>
    using NthArg = TypeList::Nth<N,ArgTypes>;
  using Sequence = std::index_sequence_for<>;
  using SequenceNoFirst = std::index_sequence_for<>;
};

template<typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...)>
{
  using Pointer = R (C::*)(Args...);
  using RetType = R;
  using ArgTypes = TypeList::TL<C,Args...>;
  using ArgTypesNoFirst = TypeList::TL<Args...>;
  using ArgTypesFirst = C; // CR smuenzel: should this be a pointer?
  static constexpr std::size_t ArgCount = sizeof...(Args) + 1;
  template<std::size_t N>
    using NthArg = TypeList::Nth<N,ArgTypes>;
  using Sequence = std::index_sequence_for<C,Args...>;
  using SequenceNoFirst = std::index_sequence_for<Args...>;
};
}

#endif
