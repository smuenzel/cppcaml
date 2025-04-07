#if !defined (_CPPCAML_INVOKE_H_)
#define _CPPCAML_INVOKE_H_

#include <cppcaml/conversion.h>

namespace Cppcaml
{

template<typename T> struct SubVoid {
  typedef T type;
};

template<> struct SubVoid<void> {
  typedef Void type;
};

// Invoke a function, and possibly substitute void return arguments

// https://stackoverflow.com/a/51454205
template<typename F, typename... Ps, size_t... is, typename Result = std::invoke_result_t<F,Ps...>>
requires (!std::same_as<Result,void>)
inline Result invoke_seq_void(F&& f, std::tuple<Ps&...>& ps, std::index_sequence<is...> iseq){
  return std::invoke(std::forward<F>(f), get<is>(ps)...);
};

template<typename F, typename... Ps, size_t... is, typename Result = std::invoke_result_t<F,Ps...>>
requires (std::same_as<Result,void>)
inline Void invoke_seq_void(F&& f, std::tuple<Ps&...>& ps, std::index_sequence<is...> iseq){
  std::invoke(std::forward<F>(f), get<is>(ps)...);
  return {};
};

/* CR smuenzel: weird that this is needed?  why can't the concept be used directly?*/
template<typename T>
  struct HasOfCamlV : std::bool_constant<HasOfCaml<T>> { };

template<typename F>
concept ArgumentsConvertible =
TypeListForAll<HasOfCamlV,typename FunctionTraits<F>::ArgTypes>::value
;

template<typename F>
concept ResultConvertible =
HasToCaml<typename SubVoid<typename FunctionTraits<F>::RetType>::type>
;

template<typename F>
concept FunctionConvertible = ArgumentsConvertible<F> && ResultConvertible<F>;

template<auto f, auto... Properties>
requires FunctionConvertible<decltype(f)>
struct Invoke
{
  using ResultType = typename SubVoid<typename FunctionTraits<decltype(f)>::RetType>::type;
  
  template<
    typename ArgTypes = FunctionTraits<decltype(f)>::ArgTypes
  , typename Seq = FunctionTraits<decltype(f)>::Sequence
  > struct InnerInvoke;

  template<typename... Args, size_t... Is>
  struct InnerInvoke<TypeList<Args...>, std::index_sequence<Is...>> {

    static inline value operator()(decltype(Is, value{})... args) {
      auto args_tuple = std::make_tuple<Args...>(CamlType<Args>::of_caml(args)...);
      std::tuple<Args&...> args_tuple_ref(args_tuple);
      auto result = invoke_seq_void(f, args_tuple_ref, std::index_sequence<Is...>{});
      auto v_result = CamlType<ResultType>::to_caml(result);
      return v_result;
    }
  };

  static constexpr const auto Caml = InnerInvoke<>();
};

template<
  auto f
, typename Seq = FunctionTraits<decltype(decltype(f)::operator())>::Sequence
> struct
InvokeBytes;

template<
  auto f
, size_t... Is
>
struct InvokeBytes<f, std::index_sequence<Is...>>
{
  static value invoke(value *argv, int argn) {
    return f(argv[Is]...);
  }
};

template<auto f>
CAMLprim value invoke_bytes(value *argv, int argn)
{
  return InvokeBytes<f>::invoke(argv, argn);
}


}

#endif
