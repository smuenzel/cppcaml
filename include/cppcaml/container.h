#if !defined(_CPPCAML_CONTAINER_H_)
#define _CPPCAML_CONTAINER_H_

#include <memory>

#include <cppcaml/conversion.h>

namespace Cppcaml
{

template<typename T, auto name>
struct CamlTypeSharedPtrContainer {
  static constexpr const auto typename_caml = name;
  static constexpr bool to_caml_allocates = true;

  using CppType = T*;
  using Representative = std::shared_ptr<T>;
  using Value = std::shared_ptr<T>;
  using ValueExtraParameters = std::tuple<>;

  static value to_caml(const std::shared_ptr<T>& sp) {
    return SharedPtrCustomValue(std::move(sp));
  }

  static std::shared_ptr<T> to_representative(T* t) {
    return std::shared_ptr<T>(t);
  }

};


template<>
struct CamlType<int*> : CamlTypeSharedPtrContainer<int, to_array("int_ptr")> { };

static_assert(HasToCaml<int*>);


}



#endif
