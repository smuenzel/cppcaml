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
HasToCaml<typename SubVoid<typename FunctionTraits<F>::ReturnType>::type>
;

template<typename F>
concept FunctionConvertible = ArgumentsConvertible<F> && ResultConvertible<F>;

}

#endif
