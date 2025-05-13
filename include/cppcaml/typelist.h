#if !defined(_CPPCAML_TYPELIST_H_)
#define _CPPCAML_TYPELIST_H_

namespace Cppcaml
{

namespace TypeList
{

template<typename... Ts> struct TL;

template<> struct TL<> {
  static constexpr const size_t size = 0;
};

template<typename T, typename... Ts> struct TL<T, Ts...> {
  using car = T;
  using cdr = TL<Ts...>;
  static constexpr const size_t size = 1 + sizeof...(Ts);
};


template<size_t N, typename T> struct Elt;

template<size_t N, typename T, typename... Ts>
struct Elt<N,TL<T,Ts...>>
  : Elt<N-1,TL<Ts...>> {};

template<typename T, typename... Ts>
struct Elt<0,TL<T, Ts...>> { using type = T; };

template <size_t N, typename T>
using Nth = typename Elt<N, T>::type;


template <template<typename> class F, typename T>
struct ForAll;

template <template<typename> class F, typename... Ts>
struct ForAll<F, TL<Ts...>> {
  static constexpr const bool value = (... && F<Ts>::value);
};


template <template<typename> class F, typename T>
struct Map;

template <template<typename> class F, typename... Ts>
struct Map<F, TL<Ts...>> {
  using type = TL<typename F<Ts>::type...>;
};


template <auto f, typename T>
struct MapToTuple;

template <auto f, typename... Ts>
struct MapToTuple<f, TL<Ts...>> {
  using type = std::tuple<decltype(f(std::declval<Ts>()))...>;
};


template<typename T, typename Ts> struct Cons;

template<typename T>
struct Cons<T, TL<>>
{
  using type = TL<T>;
};

template<typename T, typename... Ts>
struct Cons<T, TL<Ts...>>
{
  using type = TL<T, Ts...>;
};


template<template<typename> class F, typename T>
struct Filter;

template<template<typename> class F>
struct Filter<F, TL<>>
{
  using type = TL<>;
};

template<template<typename> class F, typename T, typename... Ts>
struct Filter<F, TL<T, Ts...>> {
  using rest = Filter<F, TL<Ts...>>::type;
  using type =
    std::conditional
      < F<T>::value
      , typename Cons<T, rest>::type
      , rest
      >::type;
};

template<int I, template<typename> class F, typename T>
struct ApplyAt;

template<int I, template<typename> class F>
struct ApplyAt<I, F, TL<>> {
  using type = TL<>;
};

template<int I, template<typename> class F, typename T, typename... Ts>
struct ApplyAt<I, F, TL<T, Ts...>> {
  using rest = ApplyAt<I-1, F, TL<Ts...>>::type;
  using type =
    std::conditional
      < I == 0
      , typename Cons<typename F<T>::type, rest>::type
      , typename Cons<T, rest>::type
      >::type;
};



}

};
#endif
