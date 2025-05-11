#if !defined(_CPPCAML_CONTAINER_H_)
#define _CPPCAML_CONTAINER_H_

#include <memory>

#include <cppcaml/conversion.h>

#include <caml/memory.h>
#include <caml/alloc.h>

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

template<typename T>
struct NullablePointer
{
  T* p;

  NullablePointer() : p(nullptr) { }
  NullablePointer(T* p) : p(p) { }
  operator T*() const { return p; }
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

template<typename T, auto name, typename Deleter = FreeDeleter>
struct CamlTypeNullableSharedPtrContainer {
  static constexpr const auto typename_caml = cat(name, " option");
  static constexpr bool to_caml_allocates = true;

  using CppType = NullablePointer<T>;
  using Representative = std::shared_ptr<T>;
  using Value = std::shared_ptr<T>;
  using ValueExtraParameters = std::tuple<>;

  static value to_caml(const std::shared_ptr<T>& sp) {
    if(sp == nullptr) {
      return Val_none;
    }
    CAMLparam0();
    CAMLlocal1(res_inner);
    res_inner = SharedPtrCustomValue<T>::allocate(std::move(sp));
    CAMLreturn(caml_alloc_some(res_inner));
  }

  static std::shared_ptr<T> to_representative(NullablePointer<T> t) {
    if(t.p == nullptr) {
      return nullptr;
    } else {
      return std::shared_ptr<T>(t, Deleter());
    }
  }

  static NullablePointer<T> of_caml(value v) {
    if(Is_none(v)) {
      return NullablePointer<T>();
    } else {
      return SharedPtrCustomValue<T>::from_value(Some_val(v)).get();
    }
  }

};


template<>
struct CamlType<int*> : CamlTypeSharedPtrContainer<int, to_array("int_ptr")> { };

template<>
struct CamlType<NullablePointer<int>> : CamlTypeNullableSharedPtrContainer<int, to_array("int_ptr")> { };


static_assert(HasToCaml<int*>);
static_assert(HasOfCaml<int*>);

static_assert(HasToCaml<NullablePointer<int>>);
static_assert(HasOfCaml<NullablePointer<int>>);


}



#endif
