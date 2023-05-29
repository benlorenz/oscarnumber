#include <jlpolymake/jlpolymake.h>

#include <jlpolymake/tools.h>

#include <jlpolymake/functions.h>

#include <jlpolymake/type_modules.h>

#include <jlpolymake/caller.h>

#include <jlpolymake/containers.h>

#include <polymake/common/OscarNumber.h>

#include <cxxabi.h>
#include <typeinfo>


namespace jlpolymake {

jlcxx::ArrayRef<jl_value_t*> get_type_names_oscarnumber() {
    int          status;
    char*        realname;
    int          number_of_types = 6;
    jl_value_t** type_name_tuples = new jl_value_t*[2 * number_of_types];
    int          i = 0;
    type_name_tuples[i] = jl_cstr_to_string("to_oscarnumber");
    realname = abi::__cxa_demangle(typeid(polymake::common::OscarNumber).name(), nullptr, nullptr, &status);
    type_name_tuples[i + 1] = jl_cstr_to_string(realname);
    free(realname);
    i += 2;

    type_name_tuples[i] = jl_cstr_to_string("to_array_oscarnumber");
    realname = abi::__cxa_demangle(typeid(pm::Array<polymake::common::OscarNumber>).name(), nullptr, nullptr, &status);
    type_name_tuples[i + 1] = jl_cstr_to_string(realname);
    free(realname);
    i += 2;

    type_name_tuples[i] = jl_cstr_to_string("to_vector_oscarnumber");
    realname = abi::__cxa_demangle(typeid(pm::Vector<polymake::common::OscarNumber>).name(), nullptr, nullptr, &status);
    type_name_tuples[i + 1] = jl_cstr_to_string(realname);
    free(realname);
    i += 2;

    type_name_tuples[i] = jl_cstr_to_string("to_matrix_oscarnumber");
    realname = abi::__cxa_demangle(typeid(pm::Matrix<polymake::common::OscarNumber>).name(), nullptr, nullptr, &status);
    type_name_tuples[i + 1] = jl_cstr_to_string(realname);
    free(realname);
    i += 2;

    type_name_tuples[i] = jl_cstr_to_string("to_sparsevector_oscarnumber");
    realname = abi::__cxa_demangle(typeid(pm::SparseVector<polymake::common::OscarNumber>).name(), nullptr, nullptr, &status);
    type_name_tuples[i + 1] = jl_cstr_to_string(realname);
    free(realname);
    i += 2;

    type_name_tuples[i] = jl_cstr_to_string("to_sparsematrix_oscarnumber");
    realname = abi::__cxa_demangle(typeid(pm::SparseMatrix<polymake::common::OscarNumber>).name(), nullptr, nullptr, &status);
    type_name_tuples[i + 1] = jl_cstr_to_string(realname);
    free(realname);
    i += 2;

    return jlcxx::make_julia_array(type_name_tuples, 2 * number_of_types);
}

jl_value_t* POLYMAKETYPE_OscarNumber;
jl_value_t* POLYMAKETYPE_Array_OscarNumber;
jl_value_t* POLYMAKETYPE_Vector_OscarNumber;
jl_value_t* POLYMAKETYPE_Matrix_OscarNumber;
jl_value_t* POLYMAKETYPE_SparseVector_OscarNumber;
jl_value_t* POLYMAKETYPE_SparseMatrix_OscarNumber;

template <typename FunType>
bool feed_oscarnumber_types(FunType&& fc, jl_value_t* value)
{
   jl_value_t* current_type = jl_typeof(value);
   if (jl_subtype(current_type, POLYMAKETYPE_OscarNumber)) {
      fc << jlcxx::unbox<const polymake::common::OscarNumber&>(value);
      return true;
   } else if (jl_subtype(current_type, POLYMAKETYPE_Array_OscarNumber)) {
      fc << jlcxx::unbox<const pm::Array<polymake::common::OscarNumber>&>(value);
      return true;
   } else if (jl_subtype(current_type, POLYMAKETYPE_Vector_OscarNumber)) {
      fc << jlcxx::unbox<const pm::Vector<polymake::common::OscarNumber>&>(value);
      return true;
   } else if (jl_subtype(current_type, POLYMAKETYPE_Matrix_OscarNumber)) {
      fc << jlcxx::unbox<const pm::Matrix<polymake::common::OscarNumber>&>(value);
      return true;
   } else if (jl_subtype(current_type, POLYMAKETYPE_SparseVector_OscarNumber)) {
      fc << jlcxx::unbox<const pm::SparseVector<polymake::common::OscarNumber>&>(value);
      return true;
   } else if (jl_subtype(current_type, POLYMAKETYPE_Array_OscarNumber)) {
      fc << jlcxx::unbox<const pm::SparseMatrix<polymake::common::OscarNumber>&>(value);
      return true;
   }
   return false;
}

void add_oscarnumber(jlcxx::Module& jlmodule)
{
   typedef polymake::common::OscarNumber WrappedT;

    jlmodule
        .add_type<WrappedT>(
            "OscarNumber", jlcxx::julia_type("Real", "Base"))

            .constructor<Rational>()
            .constructor<void*, long>();

    jlmodule.set_override_module(jl_base_module);
    jlmodule.method("<", [](const WrappedT& a, const WrappedT& b) { return a < b; });
    jlmodule.method("==", [](const WrappedT& a, const WrappedT& b) { return a == b; });

    jlmodule.method("cmp", [](const WrappedT& a, const WrappedT& b) { return a.cmp(b); });
    jlmodule.method("iszero", [](const WrappedT& a) { return a.is_zero(); });
    jlmodule.method("isone", [](const WrappedT& a) { return a.is_one(); });
    jlmodule.method("abs", [](const WrappedT& a) { return abs(a); });
    jlmodule.method("isinf", [](const WrappedT& a) { return a.is_inf(); });

    jlmodule.method("-", [](const WrappedT& a) { return -a; });

    jlmodule.method("+", [](const WrappedT& a, const WrappedT& b) { return a + b; });
    jlmodule.method("-", [](const WrappedT& a, const WrappedT& b) { return a - b; });
    jlmodule.method("*", [](const WrappedT& a, const WrappedT& b) { return a * b; });
    jlmodule.method("//", [](const WrappedT& a, const WrappedT& b) { return a / b; });
    jlmodule.method("^", [](const WrappedT& a, long b) { return pow(a, b); });
    jlmodule.unset_override_module();

    jlmodule.method("_sign", [](const WrappedT& a) { return a.sign(); });

    jlmodule.method("show_small_obj", [](const WrappedT& S) {
                return show_small_object<WrappedT>(S);
            });

    jlmodule.method("_uses_rational", [](const WrappedT& S) {
                return S.uses_rational();
          });

    jlmodule.method("_get_rational", [](const WrappedT& S) {
                return static_cast<Rational>(S);
          });

    jlmodule.method("_unsafe_get_ptr", [](const WrappedT& S) {
                return S.unsafe_get();
          });

    jlmodule.method("to_oscarnumber", [](pm::perl::PropertyValue v) {
        return to_SmallObject<WrappedT>(v);
    });

    jlmodule.method("_register_oscar_number", [](void* dispatch, long index) {
        polymake::common::OscarNumber::register_oscar_number(dispatch, index);
    });
}


JLCXX_MODULE define_module_polymake_oscarnumber(jlcxx::Module& jlmodule)
{

    add_oscarnumber(jlmodule);

    wrap_array<polymake::common::OscarNumber>(jlmodule);
    wrap_vector<polymake::common::OscarNumber>(jlmodule);
    wrap_matrix<polymake::common::OscarNumber>(jlmodule);
    wrap_sparsevector<polymake::common::OscarNumber>(jlmodule);
    wrap_sparsematrix<polymake::common::OscarNumber>(jlmodule);

    jlmodule.method("to_array_oscarnumber", [](pm::perl::PropertyValue pv) {
        return to_SmallObject<pm::Array<polymake::common::OscarNumber>>(pv);
    });
    jlmodule.method("to_vector_oscarnumber", [](pm::perl::PropertyValue pv) {
        return to_SmallObject<pm::Vector<polymake::common::OscarNumber>>(pv);
    });
    jlmodule.method("to_matrix_oscarnumber", [](pm::perl::PropertyValue pv) {
        return to_SmallObject<pm::Matrix<polymake::common::OscarNumber>>(pv);
    });
    jlmodule.method("to_sparsevector_oscarnumber",
        [](pm::perl::PropertyValue pv) {
            return to_SmallObject<pm::SparseVector<polymake::common::OscarNumber>>(pv);
    });
    jlmodule.method("to_sparsematrix_oscarnumber",
        [](pm::perl::PropertyValue pv) {
            return to_SmallObject<pm::SparseMatrix<polymake::common::OscarNumber>>(pv);
    });

    jlmodule.method("get_type_names_oscarnumber", &get_type_names_oscarnumber);

    insert_type_in_map("OscarNumber", &POLYMAKETYPE_OscarNumber);
    insert_type_in_map("Array_OscarNumber", &POLYMAKETYPE_Array_OscarNumber);
    insert_type_in_map("Vector_OscarNumber", &POLYMAKETYPE_Vector_OscarNumber);
    insert_type_in_map("Matrix_OscarNumber", &POLYMAKETYPE_Matrix_OscarNumber);
    insert_type_in_map("SparseVector_OscarNumber", &POLYMAKETYPE_SparseVector_OscarNumber);
    insert_type_in_map("SparseMatrix_OscarNumber", &POLYMAKETYPE_SparseMatrix_OscarNumber);

    value_feeder<pm::perl::VarFunCall&> fc_fun(
          [](pm::perl::VarFunCall& fc, jl_value_t* value)->bool {
                return feed_oscarnumber_types(fc, value);
          });
    register_value_feeder(std::move(fc_fun));

    value_feeder<pm::perl::PropertyOut&> po_fun(
          [](pm::perl::PropertyOut& po, jl_value_t* value)->bool {
                return feed_oscarnumber_types(po, value);
          });
    register_value_feeder(std::move(po_fun));

    value_feeder<pm::perl::Value> v_fun(
          [](pm::perl::Value v, jl_value_t* value)->bool {
                return feed_oscarnumber_types(v, value);
          });
    register_value_feeder(std::move(v_fun));


}

}
