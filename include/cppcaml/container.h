#if !defined(_CPPCAML_CONTAINER_H_)
#define _CPPCAML_CONTAINER_H_

#include <memory>

#include <cppcaml/conversion.h>

namespace Cppcaml
{

template<auto f>
struct FunDeleter {
  using Traits = FunctionTraits<decltype(f)>;

  void operator()(typename Traits::ArgTypesFirst p) const {
    f(p);
  }
};

struct FreeDeleter {
  void operator()(void* p) const {
    free(p);
  }
};

template<typename T, auto name, typename Deleter = FreeDeleter>
struct CamlTypeSharedPtrContainer {
  static constexpr const auto typename_caml = name;
  static constexpr bool to_caml_allocates = true;

  using CppType = T*;
  using Representative = std::shared_ptr<T>;
  using Value = std::shared_ptr<T>;
  using ValueExtraParameters = std::tuple<>;

  static value to_caml(const std::shared_ptr<T>& sp) {
    return SharedPtrCustomValue<T>::allocate(std::move(sp));
  }

  static std::shared_ptr<T> to_representative(T* t) {
    CAMLassert(t != nullptr);
    return std::shared_ptr<T>(t, Deleter());
  }

  static T* of_caml(value v) {
    return SharedPtrCustomValue<T>::from_value(v).get();
  }

};


template<>
struct CamlType<int*> : CamlTypeSharedPtrContainer<int, to_array("int_ptr")> { };

static_assert(HasToCaml<int*>);
static_assert(HasOfCaml<int*>);


}



#endif
