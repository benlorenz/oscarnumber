/* Copyright (c) 1997-2022
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include <julia/julia.h>

#include "polymake/client.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Polynomial.h"
#include "polymake/common/JuliaFieldElement.h"

namespace polymake { namespace common {

namespace juliainterface {

typedef struct __julia_field_dispatch_helper {
      long index = -1;
      void* init;
      void* init_from_mpz;
      void* copy;
      void* gc_protect;
      void* gc_free;
      void* add;
      void* sub;
      void* mul;
      void* div;
      void* pow;
      void* negate;
      void* cmp;
      void* to_string;
      void* from_string;
      void* is_zero;
      void* is_one;
      void* is_inf;
      void* sign;
      void* abs;
} julia_field_dispatch_helper;

typedef struct __julia_field_dispatch {
      long index = -1;
      std::function<jl_value_t* (long, jl_value_t**, long)> init;
      std::function<jl_value_t* (long, jl_value_t**, const mpz_srcptr, const mpz_srcptr)> init_from_mpz;
      std::function<jl_value_t* (jl_value_t*)> copy;
      std::function<void (jl_value_t*)> gc_protect;
      std::function<void (jl_value_t*)> gc_free;
      std::function<jl_value_t* (jl_value_t*, jl_value_t*)> add;
      std::function<jl_value_t* (jl_value_t*, jl_value_t*)> sub;
      std::function<jl_value_t* (jl_value_t*, jl_value_t*)> mul;
      std::function<jl_value_t* (jl_value_t*, jl_value_t*)> div;
      std::function<jl_value_t* (jl_value_t*, long)> pow;
      std::function<jl_value_t* (jl_value_t*)> negate;
      std::function<long (jl_value_t*, jl_value_t*)> cmp;
      std::function<char* (jl_value_t*)> to_string;
      std::function<jl_value_t* (char*)> from_string;
      std::function<bool (jl_value_t*)> is_zero;
      std::function<bool (jl_value_t*)> is_one;
      std::function<bool (jl_value_t*)> is_inf;
      std::function<long (jl_value_t*)> sign;
      std::function<jl_value_t* (jl_value_t*)> abs;
} julia_field_dispatch;

class julia_field_wrap {
   public:
   virtual ~julia_field_wrap() { }

   virtual julia_field_wrap* copy() const = 0;
   virtual void destruct() = 0;
   virtual julia_field_wrap* upgrade_other(julia_field_wrap* other) const = 0;
   virtual julia_field_wrap* upgrade_to(const julia_field_dispatch& d) = 0;
   virtual jl_value_t* for_julia() const = 0;
   virtual const Rational& as_rational() const = 0;
   virtual bool is_rational() const = 0;
   virtual long index() const = 0;

   virtual julia_field_wrap* negate() = 0;
   virtual julia_field_wrap* add(const julia_field_wrap* other) = 0;
   virtual julia_field_wrap* sub(const julia_field_wrap* other) = 0;
   virtual julia_field_wrap* mul(const julia_field_wrap* other) = 0;
   virtual julia_field_wrap* div(const julia_field_wrap* other) = 0;
   virtual julia_field_wrap* pow(Int p) const = 0;
   virtual Int cmp(const julia_field_wrap* other) const = 0;
   virtual bool is_zero() const = 0;
   virtual bool is_one() const = 0;
   virtual Int is_inf() const = 0;
   virtual Int sign() const = 0;
   virtual julia_field_wrap* abs_value() const = 0;
   virtual std::string to_string() const = 0;

   static julia_field_wrap* create(const Rational&);
   static julia_field_wrap* create(void* v, long index);
   static void destroy(julia_field_wrap*);
   //static julia_field_wrap* from_string(const std::string& s);
};

static std::unordered_map<Int, julia_field_dispatch> julia_field_map;

class julia_field_impl : public julia_field_wrap {
   public:
      // no default construction, this should only contain proper field elements
      // zero / one / ... is handled via embedded rationals
      // TODO: inf via rationals?
      //julia_field_impl() { };
      julia_field_impl() = delete;

      julia_field_impl(Int x, long field_index) :
         dispatch(julia_field_map[field_index]) {
         //cerr << "pre-init from int" << endl;
         jl_value_t* empty;
         JL_GC_PUSH2(&julia_elem, &empty);
         julia_elem = dispatch.init(dispatch.index, &empty, x);
         dispatch.gc_protect(julia_elem);
         JL_GC_POP();
      }

      julia_field_impl(const Rational& x, const julia_field_dispatch& d) :
         dispatch(d) {
         // FIXME: error on ininity?
         //cerr << "pre-init from mpz" << endl;
         jl_value_t* empty;
         JL_GC_PUSH2(&julia_elem, &empty);
         julia_elem = dispatch.init_from_mpz(dispatch.index, &empty, numerator(x).get_rep(),denominator(x).get_rep());
         //cerr << "post-init from mpz" << endl;
         dispatch.gc_protect(julia_elem);
         //cerr << "post-protect from mpz" << endl;
         JL_GC_POP();
      }

      julia_field_impl(const Rational& x, long field_index) :
         julia_field_impl(x, julia_field_map[field_index]) { }

      // this is a move construction taking ownership of the jl_value_t
      julia_field_impl(jl_value_t* v, const julia_field_dispatch& d, std::true_type) :
         dispatch(d), julia_elem(v) {
         //cerr << "moved in constructor" << endl;
         JL_GC_PUSH1(&julia_elem);
         dispatch.gc_protect(julia_elem);
         //cerr << "post-protect in moveconst" << endl;
         JL_GC_POP();
      }

      // this makes a copy
      julia_field_impl(jl_value_t* v, const julia_field_dispatch& d) :
         dispatch(d) {
         //cerr << "copy in constructor" << endl;
         julia_elem = dispatch.copy(v);
         //cerr << "copied in constructor" << endl;
         JL_GC_PUSH1(&julia_elem);
         dispatch.gc_protect(julia_elem);
         //cerr << "post-protect in copyconst" << endl;
         JL_GC_POP();
      }

      julia_field_impl(jl_value_t* v, long field_index, std::true_type) :
         julia_field_impl(v, julia_field_map[field_index], std::true_type()) { }

      julia_field_impl(jl_value_t* v, long field_index) :
         julia_field_impl(v, julia_field_map[field_index]) { }

      julia_field_impl(const julia_field_impl* x, long field_index) :
         julia_field_impl(x->julia_elem, field_index) {
         assert(field_index == x->dispatch.index);
      }

      ~julia_field_impl() {
         JL_GC_PUSH1(&julia_elem);
         //cerr << "free in ~: " << julia_elem << endl;
         dispatch.gc_free(julia_elem);
         JL_GC_POP();
      }

      void destruct() {
         JL_GC_PUSH1(&julia_elem);
         //cerr << "free in destruct: " << julia_elem << endl;
         dispatch.gc_free(julia_elem);
         JL_GC_POP();
      }

      julia_field_wrap* copy() const {
         return new julia_field_impl(this->julia_elem, dispatch.index);
      }

      julia_field_wrap* upgrade_to(const julia_field_dispatch& d) {
         if (dispatch.index != d.index)
            throw std::runtime_error("julia_field_wrap: different julia fields!");
         // nothing to do, both in the same proper field
         return this;
      }

      julia_field_wrap* upgrade_other(julia_field_wrap* other) const {
         return other->upgrade_to(this->dispatch);
      }

      jl_value_t* for_julia() const {
         return julia_elem;
      }

      const Rational& as_rational() const {
         // this should not be called
         throw std::runtime_error("julia_field_wrap: error accessing rational of proper field element");
      }
      /*
      julia_field_wrap* add(const julia_field_wrap* b) {
         jl_value_t* other = b->for_julia();
         //cerr << "pre-add" << endl;
         julia_elem = dispatch.add(julia_elem, other);
         //cerr << "post-add" << endl;
         return this;
      } */
      julia_field_wrap* add(const julia_field_wrap* b) {
         //cerr << "pre-add" << endl;
         jl_value_t* res = dispatch.add(julia_elem, b->for_julia());
         //cerr << "post-add" << endl;
         JL_GC_PUSH1(&res);
         //cerr << "pre-add gc" << endl;
         dispatch.gc_protect(res);
         //cerr << "mid-add gc" << endl;
         dispatch.gc_free(julia_elem);
         //cerr << "post-add gc" << endl;
         julia_elem = res;
         JL_GC_POP();
         return this;
      }

      julia_field_wrap* sub(const julia_field_wrap* b) {
         //cerr << "pre-sub" << endl;
         jl_value_t* res = dispatch.sub(julia_elem, b->for_julia());
         JL_GC_PUSH1(&res);
         dispatch.gc_protect(res);
         dispatch.gc_free(julia_elem);
         julia_elem = res;
         JL_GC_POP();
         return this;
      }
      julia_field_wrap* mul(const julia_field_wrap* b) {
         //cerr << "pre-sub" << endl;
         jl_value_t* res = dispatch.mul(julia_elem, b->for_julia());
         JL_GC_PUSH1(&res);
         dispatch.gc_protect(res);
         dispatch.gc_free(julia_elem);
         julia_elem = res;
         JL_GC_POP();
         return this;
      }
      julia_field_wrap* div(const julia_field_wrap* b) {
         //cerr << "pre-sub" << endl;
         jl_value_t* res = dispatch.div(julia_elem, b->for_julia());
         JL_GC_PUSH1(&res);
         dispatch.gc_protect(res);
         dispatch.gc_free(julia_elem);
         julia_elem = res;
         JL_GC_POP();
         return this;
      }

      julia_field_wrap* negate() {
         //cerr << "pre-sub" << endl;
         jl_value_t* res = dispatch.negate(julia_elem);
         JL_GC_PUSH1(&res);
         dispatch.gc_protect(res);
         dispatch.gc_free(julia_elem);
         julia_elem = res;
         JL_GC_POP();
         return this;
      }

      julia_field_wrap* pow(Int k) const {
         //cerr << "pre-pow" << endl;
         return new julia_field_impl(dispatch.pow(julia_elem, k), dispatch, std::true_type());
      }

      Int cmp(const julia_field_wrap* b) const {
         //cerr << "pre-cmp" << endl;
         return dispatch.cmp(julia_elem, b->for_julia());
      }

      bool is_zero() const {
         //cerr << "pre-isz" << endl;
         return dispatch.is_zero(julia_elem);
      }
      bool is_one() const {
         //cerr << "pre-iso" << endl;
         return dispatch.is_one(julia_elem);
      }
      Int is_inf() const {
         // no inf in nemo types
         // FIXME add int?
         return 0;
         //return dispatch.is_inf(julia_elem);
      }
      Int sign() const {
         //cerr << "pre-sign" << endl;
         return dispatch.sign(julia_elem);
      }
      julia_field_wrap* abs_value() const {
         //cerr << "pre-abs" << endl;
         return new julia_field_impl(dispatch.abs(julia_elem), dispatch, std::true_type());
      }

      constexpr bool is_rational() const {
         return false;
      }
      long index() const {
         return dispatch.index;
      }

      std::string to_string() const {
         std::ostringstream str;
         static jl_function_t *strfun = jl_get_function(jl_base_module, "string");
         jl_value_t* jstr = jl_call1(strfun, julia_elem);
         JL_GC_PUSH1(&jstr);
         const char* cstr = jl_string_ptr(jstr);
         str << "(" << cstr << ")";
         JL_GC_POP();
         return str.str();
      }

   private:
      const julia_field_dispatch& dispatch;
      jl_value_t* julia_elem = nullptr;
      // TODO: infinity not implemented yet
      int infinity = 0;

   friend class julia_field_rational_impl;
};

class julia_field_rational_impl : Rational, public julia_field_wrap {
public:
   julia_field_rational_impl(const Rational& r) : Rational(r) { }

   julia_field_rational_impl() = delete;

   ~julia_field_rational_impl() { }

   julia_field_wrap* copy() const {
      return new julia_field_rational_impl(Rational(*this));
   }

   void destruct() { }

   julia_field_wrap* upgrade_other(julia_field_wrap* other) const {
      // this should not be called
      throw std::runtime_error("julia_field_wrap: error upgrading to rational element");
   }

   julia_field_wrap* upgrade_to(const julia_field_dispatch& d) {
      return new julia_field_impl((Rational) *this, d);
   }

   jl_value_t* for_julia() const {
      // TODO check upcast?
      // we should probably never end up here
      throw std::runtime_error("julia_field_rational: not implemented");
      //return (jl_value_t*) nullptr;
   }

   const Rational& as_rational() const {
      return (const Rational&) *this;
   }

   julia_field_wrap* negate() {
      Rational::negate();
      return this;
   }
   julia_field_wrap* add(const julia_field_wrap* other) {
      Rational::operator+=(other->as_rational());
      return this;
   }
   julia_field_wrap* sub(const julia_field_wrap* other) {
      Rational::operator-=(other->as_rational());
      return this;
   }
   julia_field_wrap* mul(const julia_field_wrap* other) {
      Rational::operator*=(other->as_rational());
      return this;
   }
   julia_field_wrap* div(const julia_field_wrap* other) {
      Rational::operator/=(other->as_rational());
      return this;
   }
   julia_field_wrap* pow(Int p) const {
      return new julia_field_rational_impl(Rational::pow(*this, p));
   }
   Int cmp(const julia_field_wrap* other) const {
      return this->compare(other->as_rational());
   }
   bool is_zero() const {
      return Rational::is_zero();
   }
   bool is_one() const {
      return pm::is_one((Rational) *this);
   }
   Int is_inf() const {
      return pm::isinf(*this);
   }
   Int sign() const {
      return pm::sign(*this);
   }
   julia_field_wrap* abs_value() const {
      return new julia_field_rational_impl(abs((Rational)*this));
   }

   constexpr bool is_rational() const {
      return true;
   }
   constexpr long index() const {
      return 0;
   }

   std::string to_string() const {
      std::ostringstream str;
      str << (Rational) *this;
      return str.str();
   }

   //static julia_field_wrap* from_string(const std::string& s);
};

julia_field_wrap* julia_field_wrap::create(const Rational& r) {
   return new julia_field_rational_impl(r);
}

julia_field_wrap* julia_field_wrap::create(void* e, long index) {
   return new julia_field_impl(reinterpret_cast<jl_value_t*>(e), index);
}

void julia_field_wrap::destroy(julia_field_wrap* j) {
   delete j;
}

using jfwptr = std::unique_ptr<julia_field_wrap, void (*)(julia_field_wrap*)>;

void maybe_upgrade(jfwptr& a, jfwptr& b) {
   if (b->is_rational() && !a->is_rational())
      b = std::move(jfwptr(a->upgrade_other(b.get()), &julia_field_wrap::destroy));
   else if (a->is_rational() && !b->is_rational())
      a = std::move(jfwptr(b->upgrade_other(a.get()), &julia_field_wrap::destroy));
   else if (a->index() != b->index() && a->index() * b->index() != 0)
      throw std::runtime_error("julia_field_wrap: different julia fields!");
}

} // end juliainterface

using juliainterface::jfwptr;

// implementations for jfe class
JuliaFieldElement::JuliaFieldElement() :
   JuliaFieldElement(Rational(0)) {}

//JuliaFieldElement::~JuliaFieldElement() = default;

JuliaFieldElement::JuliaFieldElement(const Rational& r) :
   impl(juliainterface::julia_field_wrap::create(r), &juliainterface::julia_field_wrap::destroy) {}

JuliaFieldElement::JuliaFieldElement(const JuliaFieldElement& jfe) :
   impl(jfe.impl->copy(), &juliainterface::julia_field_wrap::destroy) {
   //cerr << "copied" << endl;
}

JuliaFieldElement::JuliaFieldElement(JuliaFieldElement&& jfe) :
   impl(std::move(jfe.impl)) {
   //cerr << "moved" << endl;
}

JuliaFieldElement::JuliaFieldElement(void* jv, Int index) :
   impl(juliainterface::julia_field_wrap::create(jv, index), &juliainterface::julia_field_wrap::destroy) {}

JuliaFieldElement& JuliaFieldElement::operator= (const Rational& b) {
   impl = std::move(jfwptr(juliainterface::julia_field_wrap::create(b), &juliainterface::julia_field_wrap::destroy));
   return *this;
}

JuliaFieldElement::JuliaFieldElement(juliainterface::julia_field_wrap* jfw) :
   impl(jfw, &juliainterface::julia_field_wrap::destroy) {}

JuliaFieldElement& JuliaFieldElement::operator= (const JuliaFieldElement& b) {
   impl = std::move(jfwptr(b.impl->copy(), &juliainterface::julia_field_wrap::destroy));
   return *this;
}

JuliaFieldElement& JuliaFieldElement::operator+= (const Rational& b) {
   return *this += JuliaFieldElement(b);
}
JuliaFieldElement& JuliaFieldElement::operator-= (const Rational& b) {
   return *this -= JuliaFieldElement(b);
}
JuliaFieldElement& JuliaFieldElement::operator*= (const Rational& b) {
   return *this *= JuliaFieldElement(b);
}
JuliaFieldElement& JuliaFieldElement::operator/= (const Rational& b) {
   return *this /= JuliaFieldElement(b);
}
//JuliaFieldElement& JuliaFieldElement::operator+= (const Rational& r){
//   auto jfr = juliainterface::julia_field_wrap::create(r);
//   if (impl->is_rational()) {
//      impl->add(jfr);
//   } else {
//      auto jf = impl->upgrade_other(jfr);
//      impl->add(jf);
//      delete jf;
//   }
//   delete jfr;
//   return *this;
//}
//JuliaFieldElement& JuliaFieldElement::operator-= (const Rational& r){
//   impl->sub(juliainterface::julia_field_wrap::create(r));
//   return *this;
//}
//JuliaFieldElement& JuliaFieldElement::operator*= (const Rational& r){
//   impl->mul(juliainterface::julia_field_wrap::create(r));
//   return *this;
//}
//JuliaFieldElement& JuliaFieldElement::operator/= (const Rational& r){
//   impl->div(juliainterface::julia_field_wrap::create(r));
//   return *this;
//}

JuliaFieldElement& JuliaFieldElement::operator+= (const JuliaFieldElement& b){
   juliainterface::maybe_upgrade(impl, b.impl);
   impl->add(b.impl.get());
   return *this;
}
JuliaFieldElement& JuliaFieldElement::operator-= (const JuliaFieldElement& b){
   juliainterface::maybe_upgrade(impl, b.impl);
   impl->sub(b.impl.get());
   return *this;
}
JuliaFieldElement& JuliaFieldElement::operator*= (const JuliaFieldElement& b){
   juliainterface::maybe_upgrade(impl, b.impl);
   impl->mul(b.impl.get());
   return *this;
}
JuliaFieldElement& JuliaFieldElement::operator/= (const JuliaFieldElement& b){
   juliainterface::maybe_upgrade(impl, b.impl);
   impl->div(b.impl.get());
   return *this;

}

JuliaFieldElement& JuliaFieldElement::negate() {
   impl->negate();
   return *this;
}

JuliaFieldElement pow(const JuliaFieldElement& a, Int k) {
   return JuliaFieldElement(a.impl->pow(k));
}

Int JuliaFieldElement::cmp(const JuliaFieldElement& b) const {
   juliainterface::maybe_upgrade(impl, b.impl);
   return impl->cmp(b.impl.get());
}

Int JuliaFieldElement::cmp(const Rational& r) const {
   return this->cmp(JuliaFieldElement(r));
}

bool JuliaFieldElement::is_zero() const {
   return impl->is_zero();
}
bool JuliaFieldElement::is_one() const {
   return impl->is_one();
}

Int JuliaFieldElement::is_inf() const {
   return impl->is_inf();
}

Int JuliaFieldElement::sign() const {
   return impl->sign();
}

JuliaFieldElement abs(const JuliaFieldElement& jf) {
   return JuliaFieldElement(jf.impl->abs_value());
}

JuliaFieldElement::operator Rational() const {
   return Rational(impl->as_rational());
}

JuliaFieldElement::operator double() const {
   // TODO dont go via rational
   return double(impl->as_rational());
}

std::string JuliaFieldElement::to_string() const {
   return impl->to_string();
}


void JuliaFieldElement::register_julia_field(void* disp, long index) {
   using namespace juliainterface;
   if (julia_field_map.find(index) != julia_field_map.end())
      throw std::runtime_error("polymake::JFE: error re-registering julia field index");

   julia_field_dispatch dispatch;
   dispatch.index = index;
   julia_field_dispatch_helper* helper = reinterpret_cast<julia_field_dispatch_helper*>(disp);
   dispatch.init          = std::function<jl_value_t*     (long, jl_value_t**, long)>(
                         reinterpret_cast<jl_value_t* (*) (long, jl_value_t**, long)>(helper->init));
   dispatch.init_from_mpz = std::function<jl_value_t*     (long, jl_value_t**, const mpz_srcptr, const mpz_srcptr)>(
                         reinterpret_cast<jl_value_t* (*) (long, jl_value_t**, const mpz_srcptr, const mpz_srcptr)>(helper->init_from_mpz));
   dispatch.copy          = std::function<jl_value_t*     (jl_value_t*)>(
                         reinterpret_cast<jl_value_t* (*) (jl_value_t*)>(helper->copy));

   dispatch.gc_protect    = std::function<void     (jl_value_t*)>(
                         reinterpret_cast<void (*) (jl_value_t*)>(helper->gc_protect));
   dispatch.gc_free       = std::function<void     (jl_value_t*)>(
                         reinterpret_cast<void (*) (jl_value_t*)>(helper->gc_free));

   dispatch.add = std::function<jl_value_t*     (jl_value_t*, jl_value_t*)>(
               reinterpret_cast<jl_value_t* (*) (jl_value_t*, jl_value_t*)>(helper->add));
   dispatch.sub = std::function<jl_value_t*     (jl_value_t*, jl_value_t*)>(
               reinterpret_cast<jl_value_t* (*) (jl_value_t*, jl_value_t*)>(helper->sub));
   dispatch.mul = std::function<jl_value_t*     (jl_value_t*, jl_value_t*)>(
               reinterpret_cast<jl_value_t* (*) (jl_value_t*, jl_value_t*)>(helper->mul));
   dispatch.div = std::function<jl_value_t*     (jl_value_t*, jl_value_t*)>(
               reinterpret_cast<jl_value_t* (*) (jl_value_t*, jl_value_t*)>(helper->div));

   dispatch.pow     = std::function<jl_value_t*     (jl_value_t*, long)>(
                   reinterpret_cast<jl_value_t* (*) (jl_value_t*, long)>(helper->pow));
   dispatch.negate  = std::function<jl_value_t*     (jl_value_t*)>(
                   reinterpret_cast<jl_value_t* (*) (jl_value_t*)>(helper->negate));
   dispatch.abs     = std::function<jl_value_t*     (jl_value_t*)>(
                   reinterpret_cast<jl_value_t* (*) (jl_value_t*)>(helper->abs));

   dispatch.cmp = std::function<long (jl_value_t*, jl_value_t*)>(reinterpret_cast<long(*)(jl_value_t*, jl_value_t*)>(helper->cmp));

   //dispatch.to_string   = std::function<char* (jl_value_t*)>(reinterpret_cast<char* (*)(jl_value_t*)>(helper->to_string));
   //dispatch.from_string = std::function<jl_value_t* (char*)>(reinterpret_cast<jl_value_t*  (*) (char*)>(helper->from_string));

   dispatch.is_zero = std::function<bool     (jl_value_t*)>(
                   reinterpret_cast<bool (*) (jl_value_t*)>(helper->is_zero));
   dispatch.is_one  = std::function<bool     (jl_value_t*)>(
                   reinterpret_cast<bool (*) (jl_value_t*)>(helper->is_one));
   //dispatch.is_inf  = std::function<bool     (jl_value_t*)>(
   //                reinterpret_cast<bool (*) (jl_value_t*)>(helper->is_inf));
   dispatch.sign    = std::function<long     (jl_value_t*)>(
                   reinterpret_cast<long (*) (jl_value_t*)>(helper->sign));

   julia_field_map.emplace(index, std::move(dispatch));
}

} }

namespace pm {

bool
spec_object_traits< polymake::common::JuliaFieldElement>::is_zero(const persistent_type& p)
{
   return p.is_zero();
}
bool
spec_object_traits< polymake::common::JuliaFieldElement>::is_one(const persistent_type& p)
{
   return p.is_one();
}
const polymake::common::JuliaFieldElement&
spec_object_traits< polymake::common::JuliaFieldElement>::zero()
{
   static const persistent_type x(0);
   return x;
}
const polymake::common::JuliaFieldElement&
spec_object_traits< polymake::common::JuliaFieldElement>::one()
{
   static const persistent_type x(1);
   return x;
}

}
