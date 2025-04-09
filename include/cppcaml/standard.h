#if !defined(_CPPCAML_STANDARD_H_)
#define _CPPCAML_STANDARD_H_

#include <stdint.h>

#include <cppcaml/conversion.h>

namespace Cppcaml
{

struct Void {};

template<>
struct CamlType<Void>{
  static const constexpr auto typename_caml = to_array("unit");
  static const constexpr bool to_caml_allocates = false;
  using CppType = Void;
  using Representative = Void;
  using ValueExtraParameters = std::tuple<>;

  static Void to_representative(Void value){
    return {};
  }

  static value to_caml(Void){
    return Val_unit;
  }

  static Void of_caml(value){
    return {};
  }
};
static_assert(BidirectionalCaml<Void>);

template<>
struct CamlType<const char*>{
  static const constexpr auto typename_caml = to_array("string");
  static const constexpr bool to_caml_allocates = true;
  using CppType = const char*;
  using Representative = const char*;
  using ValueExtraParameters = std::tuple<>;

  static const char* to_representative(const char* value){
    return value;
  }

  static value to_caml(const char* value){
    return caml_copy_string(value);
  }

  static const char* of_caml(value value){
    return String_val(value);
  }
};
static_assert(BidirectionalCaml<const char*>);

template<>
struct CamlType<int64_t>{
  static const constexpr auto typename_caml = to_array("int64");
  static const constexpr bool to_caml_allocates = true;
  using CppType = int64_t;
  using Representative = int64_t;
  using ValueExtraParameters = std::tuple<>;

  static int64_t to_representative(int64_t value){
    return value;
  }

  static value to_caml(int64_t value){
    return caml_copy_int64(value);
  }

  static int64_t of_caml(value v){
    return Int64_val(v);
  }
};
static_assert(BidirectionalCaml<int64_t>);

template<typename T, auto name>
struct SimpleCamlTypeBase {
  static const constexpr auto typename_caml = name;
  static const constexpr bool to_caml_allocates = false;
  using CppType = T;
  using Representative = T;
  using ValueExtraParameters = std::tuple<>;

  static const T to_representative(T value){
    return value;
  }
};

template<typename T, auto name>
struct IntableCamlType : public SimpleCamlTypeBase<T, name>
{
  static value to_caml(T value){
    return Val_long((long)value);
  }

  static T of_caml(value value){
    return (T)Long_val(value);
  }

};

template<>
struct CamlType<int> : public IntableCamlType<int, to_array("int")> {};
static_assert(BidirectionalCaml<int>);

template<>
struct CamlType<uint8_t> : public IntableCamlType<uint8_t, to_array("uint8")> {};
static_assert(BidirectionalCaml<uint8_t>);

template<>
struct CamlType<int8_t> : public IntableCamlType<int8_t, to_array("int8")> {};
static_assert(BidirectionalCaml<int8_t>);

template<>
struct CamlType<uint16_t> : public IntableCamlType<uint16_t, to_array("uint16")> {};
static_assert(BidirectionalCaml<uint16_t>);

template<>
struct CamlType<int16_t> : public IntableCamlType<int16_t, to_array("int16")> {};
static_assert(BidirectionalCaml<int16_t>);

template<>
struct CamlType<uint32_t> : public IntableCamlType<uint32_t, to_array("uint32")> {};
static_assert(BidirectionalCaml<uint32_t>);

/* This will truncate size_t to a 62 bit size, but seems okay */
template<>
struct CamlType<size_t> : public IntableCamlType<size_t, to_array("int")> {};
static_assert(BidirectionalCaml<size_t>);

template<>
struct CamlType<bool> : public SimpleCamlTypeBase<bool, to_array("bool")>
{
  static value to_caml(bool value){
    return Val_bool(value);
  }

  static bool of_caml(value value){
    return Bool_val(value);
  }
};
static_assert(BidirectionalCaml<bool>);

template<typename E>
requires (std::is_enum_v<E>  && sizeof(E) < sizeof(value))
struct CamlType<E>
  : public IntableCamlType
    < E
    , make_string_view_array<FIX8::conjure_type<E>::name.size() - 1>(FIX8::conjure_type<E>::name)
    >
{
};

}

#endif
