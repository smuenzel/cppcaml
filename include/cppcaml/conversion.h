#if !defined(_CPPCAML_CONVERSION_H_)
#define _CPPCAML_CONVERSION_H_

#include <concepts>
#include <tuple>

#include <cppcaml/utils.h>
#include <cppcaml/values.h>

namespace Cppcaml
{

template <typename T, template <typename...> class C>
struct IsSpecializationOf : std::false_type {};

template <template <typename...> class C, typename... Ts>
struct IsSpecializationOf<C<Ts...>, C> : std::true_type {};

template<typename T, template <typename...> class C>
concept Specializes = IsSpecializationOf<T, C>::value;

template<typename T>
struct CamlType {
};

template<typename T>
concept HasCamlConversion = requires {
  typename CamlType<T>;
  typename CamlType<T>::Representative;
  typename CamlType<T>::ValueExtraParameters;
  typename CamlType<T>::CppType;
  requires std::same_as<typename CamlType<T>::CppType, T>;
  requires Specializes<typename CamlType<T>::ValueExtraParameters, std::tuple>;
  { CamlType<T>::typename_caml.data() } -> std::convertible_to<const char*>;
};

template<typename T>
concept HasToCaml =
  HasCamlConversion<T>
  && requires(T& t) {
    { CamlType<T>::to_caml_allocates } -> std::convertible_to<bool>;
    { CamlType<T>::to_representative(t) } -> std::same_as<typename CamlType<T>::Representative>;
    requires requires (CamlType<T>::Representative R, CamlType<T>::ValueExtraParameters P) { 
      { std::apply(CamlType<T>::to_caml, std::tuple_cat(std::make_tuple(R), P)) } -> std::same_as<value>;
    };
  }
;

template<typename T>
concept HasOfCaml =
  HasCamlConversion<T>
  && requires(value v) {
    { CamlType<T>::of_caml(v) } -> std::same_as<typename CamlType<T>::Representative>;
  }
;

template<typename T>
concept BidirectionalCaml =
  HasToCaml<T>
  && HasOfCaml<T>
;

}

#endif
