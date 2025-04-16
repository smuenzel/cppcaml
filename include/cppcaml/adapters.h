#if !defined(_CPPCAML_ADAPTERS_H_)
#define _CPPCAML_ADAPTERS_H_

#include <string.h>

#include <cppcaml/utils.h>

#include <type_traits>

namespace Cppcaml
{

struct AdapterDummy
{
  template<auto f> struct Adapt
  {
    using ResultType = typename FunctionTraits<decltype(f)>::RetType;
    using ArgTypes = typename FunctionTraits<decltype(f)>::ArgTypes;

    template<
      typename ArgTypes = FunctionTraits<decltype(f)>::ArgTypes
      > struct Inner;

    template<typename... Args>
      struct Inner<TypeList::TL<Args...>> {
        // CR smuenzel: could we use forwarding here?? Using references leads to
        // template instantiation issues right now
        static auto call(Args... args) {
          return f(args...);
        }
      };

    struct Caller : public Inner<> { };
  };
};

struct AdapterNoArg
{
  template<auto f>
    requires (std::same_as<typename FunctionTraits<decltype(f)>::ArgTypes, TypeList::TL<>>)
    struct Adapt
    {
      using ResultType = typename FunctionTraits<decltype(f)>::RetType;

      struct Caller
      {
        static auto call(Void) {
          return f();
        }
      };
    };
};


template <typename T>
concept IsAdapter =
std::same_as<T, AdapterDummy> || std::same_as<T, AdapterNoArg>
;

template <typename T>
struct IsAdapterV : std::bool_constant<IsAdapter<T>> {};


template<typename T>
using AllAdapters = TypeList::Filter<IsAdapterV, T>::type;

static_assert (std::is_same_v<AllAdapters<TypeList::TL<>>, TypeList::TL<>>);
static_assert (IsAdapterV<AdapterNoArg>::value);
static_assert (1 == AllAdapters<TypeList::TL<AdapterNoArg>>::size);
static_assert (std::is_same_v<AllAdapters<TypeList::TL<AdapterNoArg>>, TypeList::TL<AdapterNoArg>>);

template<auto f, typename T>
struct ApplyAdapters;

template<auto f>
struct ApplyAdapters<f, TypeList::TL<>> {
  using Caller = AdapterDummy::template Adapt<f>::Caller;
};


template<auto f, typename T, typename... Ts>
struct ApplyAdapters<f, TypeList::TL<T, Ts...>> {
  using InnerCaller = T::template Adapt<f>::Caller;
  using NextAdapter = ApplyAdapters<&InnerCaller::call, TypeList::TL<Ts...>>;
  using Caller = NextAdapter::Caller;
};


}

#endif
