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
#include "polymake/common/OscarNumber.h"

namespace polymake { namespace common {

namespace juliainterface {

typedef struct __oscar_number_dispatch_helper {
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
} oscar_number_dispatch_helper;

typedef struct __oscar_number_dispatch {
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
} oscar_number_dispatch;

class oscar_number_wrap {
   public:
   virtual ~oscar_number_wrap() { }

   virtual oscar_number_wrap* copy() const = 0;
   virtual void destruct() = 0;
   virtual oscar_number_wrap* upgrade_other(oscar_number_wrap* other) const = 0;
   virtual oscar_number_wrap* upgrade_to(const oscar_number_dispatch& d) = 0;
   virtual jl_value_t* for_julia() const = 0;
   virtual const Rational& as_rational() const = 0;
   virtual bool uses_rational() const = 0;
   virtual long index() const = 0;

   virtual oscar_number_wrap* negate() = 0;
   virtual oscar_number_wrap* add(const oscar_number_wrap* other) = 0;
   virtual oscar_number_wrap* sub(const oscar_number_wrap* other) = 0;
   virtual oscar_number_wrap* mul(const oscar_number_wrap* other) = 0;
   virtual oscar_number_wrap* div(const oscar_number_wrap* other) = 0;
   virtual oscar_number_wrap* pow(Int p) const = 0;
   virtual Int cmp(const oscar_number_wrap* other) const = 0;
   virtual bool is_zero() const = 0;
   virtual bool is_one() const = 0;
   virtual Int is_inf() const = 0;
   virtual Int sign() const = 0;
   virtual oscar_number_wrap* abs_value() const = 0;
   virtual std::string to_string() const = 0;

   static oscar_number_wrap* create(const Rational&);
   static oscar_number_wrap* create(void* v, long index);
   static void destroy(oscar_number_wrap*);
   //static oscar_number_wrap* from_string(const std::string& s);
};

static std::unordered_map<Int, oscar_number_dispatch> oscar_number_map;

class oscar_number_impl : public oscar_number_wrap {
   public:
      // no default construction, this should only contain proper field elements
      // zero / one / ... is handled via embedded rationals
      //oscar_number_impl() { };
      oscar_number_impl() = delete;

      oscar_number_impl(Int x, long field_index) :
         dispatch(oscar_number_map[field_index]) {
         //cerr << "pre-init from int" << endl;
         jl_value_t* empty;
         JL_GC_PUSH2(&julia_elem, &empty);
         julia_elem = dispatch.init(dispatch.index, &empty, x);
         dispatch.gc_protect(julia_elem);
         JL_GC_POP();
      }

      oscar_number_impl(const Rational& x, const oscar_number_dispatch& d) :
         dispatch(d) {
         //cerr << "pre-init from mpz" << endl;
         jl_value_t* empty;
         JL_GC_PUSH2(&julia_elem, &empty);
         if (__builtin_expect(isfinite(x),1)) {
            julia_elem = dispatch.init_from_mpz(dispatch.index, &empty, numerator(x).get_rep(),denominator(x).get_rep());
         } else {
            julia_elem = dispatch.init(dispatch.index, &empty, 1);
            infinity = isinf(x);
         }
         //cerr << "post-init from mpz" << endl;
         dispatch.gc_protect(julia_elem);
         //cerr << "post-protect from mpz" << endl;
         JL_GC_POP();

      }

      oscar_number_impl(const Rational& x, long field_index) :
         oscar_number_impl(x, oscar_number_map[field_index]) { }

      // this is a move construction taking ownership of the jl_value_t
      oscar_number_impl(jl_value_t* v, const oscar_number_dispatch& d, std::true_type) :
         dispatch(d), julia_elem(v) {
         //cerr << "moved in constructor" << endl;
         JL_GC_PUSH1(&julia_elem);
         dispatch.gc_protect(julia_elem);
         //cerr << "post-protect in moveconst" << endl;
         JL_GC_POP();
      }

      // this makes a copy
      oscar_number_impl(jl_value_t* v, const oscar_number_dispatch& d) :
         dispatch(d) {
         //cerr << "copy in constructor" << endl;
         julia_elem = dispatch.copy(v);
         //cerr << "copied in constructor" << endl;
         JL_GC_PUSH1(&julia_elem);
         dispatch.gc_protect(julia_elem);
         //cerr << "post-protect in copyconst" << endl;
         JL_GC_POP();
      }

      oscar_number_impl(jl_value_t* v, long field_index, std::true_type) :
         oscar_number_impl(v, oscar_number_map[field_index], std::true_type()) { }

      oscar_number_impl(jl_value_t* v, long field_index) :
         oscar_number_impl(v, oscar_number_map[field_index]) { }

      oscar_number_impl(const oscar_number_impl* x, long field_index) :
         oscar_number_impl(x->julia_elem, field_index) {
         infinity = x->infinity;
         assert(field_index == x->dispatch.index);
      }

      ~oscar_number_impl() {
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

      oscar_number_wrap* copy() const {
         return new oscar_number_impl(this, dispatch.index);
      }

      oscar_number_wrap* upgrade_to(const oscar_number_dispatch& d) {
         if (dispatch.index != d.index)
            throw std::runtime_error("oscar_number_wrap: different julia fields!");
         // nothing to do, both in the same proper field
         return this;
      }

      oscar_number_wrap* upgrade_other(oscar_number_wrap* other) const {
         return other->upgrade_to(this->dispatch);
      }

      jl_value_t* for_julia() const {
         return julia_elem;
      }

      const Rational& as_rational() const {
         // this should not be called
         throw std::runtime_error("oscar_number_wrap: error accessing rational of proper field element");
      }

      //oscar_number_wrap* add(const oscar_number_wrap* b) {
      //   jl_value_t* other = b->for_julia();
      //   //cerr << "pre-add" << endl;
      //   julia_elem = dispatch.add(julia_elem, other);
      //   //cerr << "post-add" << endl;
      //   return this;
      //} 
      oscar_number_wrap* add(const oscar_number_wrap* b) {
         if (__builtin_expect(this->is_inf() == 0, 1)) {
            if (__builtin_expect(b->is_inf() == 0, 1)) {
               jl_value_t* res = dispatch.add(julia_elem, b->for_julia());
               JL_GC_PUSH1(&res);
               dispatch.gc_protect(res);
               dispatch.gc_free(julia_elem);
               julia_elem = res;
               JL_GC_POP();
            } else
               infinity = b->is_inf();
         } else if (this->is_inf() + b->is_inf() == 0)
            throw pm::GMP::NaN();
         return this;
      }

      oscar_number_wrap* sub(const oscar_number_wrap* b) {
         if (__builtin_expect(this->is_inf() == 0, 1)) {
            if (__builtin_expect(b->is_inf() == 0, 1)) {
               jl_value_t* res = dispatch.sub(julia_elem, b->for_julia());
               JL_GC_PUSH1(&res);
               dispatch.gc_protect(res);
               dispatch.gc_free(julia_elem);
               julia_elem = res;
               JL_GC_POP();
            } else
               infinity = -b->is_inf();
         } else if (this->is_inf() - b->is_inf() == 0)
            throw pm::GMP::NaN();
         return this;
      }
      oscar_number_wrap* mul(const oscar_number_wrap* b) {
         if (__builtin_expect(this->is_inf() == 0, 1)) {
            if (__builtin_expect(b->is_inf() == 0, 1)) {
               jl_value_t* res = dispatch.mul(julia_elem, b->for_julia());
               JL_GC_PUSH1(&res);
               dispatch.gc_protect(res);
               dispatch.gc_free(julia_elem);
               julia_elem = res;
               JL_GC_POP();
            } else {
               if (this->is_zero())
                  throw pm::GMP::NaN();
               infinity = this->sign() * b->is_inf();
            }
         } else {
            if (b->is_zero())
               throw pm::GMP::NaN();
            infinity = this->is_inf() * b->sign();
         }
         return this;
      }
      oscar_number_wrap* div(const oscar_number_wrap* b) {
         if (__builtin_expect(b->is_zero(), 0))
            throw pm::GMP::ZeroDivide();
         if (__builtin_expect(this->is_inf() == 0, 1)) {
            if (__builtin_expect(b->is_inf() == 0, 1)) {
               jl_value_t* res = dispatch.div(julia_elem, b->for_julia());
               JL_GC_PUSH1(&res);
               dispatch.gc_protect(res);
               dispatch.gc_free(julia_elem);
               julia_elem = res;
               JL_GC_POP();
            } else {
               jl_value_t* empty;
               JL_GC_PUSH1(&empty);
               jl_value_t* zero = dispatch.init(dispatch.index, &empty, 0);
               dispatch.gc_protect(zero);
               dispatch.gc_free(julia_elem);
               julia_elem = zero;
               JL_GC_POP();
            }
         } else {
            if (b->is_inf())
               throw pm::GMP::NaN();
            infinity *= b->sign();
         }
         return this;
      }

      oscar_number_wrap* negate() {
         //cerr << "pre-sub" << endl;
         if (this->is_zero())
            return this;
         if (__builtin_expect(this->is_inf() == 0, 1)) {
            jl_value_t* res = dispatch.negate(julia_elem);
            JL_GC_PUSH1(&res);
            dispatch.gc_protect(res);
            dispatch.gc_free(julia_elem);
            julia_elem = res;
            JL_GC_POP();
         } else {
            infinity = -infinity;
         }
         return this;
      }

      oscar_number_wrap* pow(Int k) const {
         //cerr << "pre-pow" << endl;
         if (__builtin_expect(this->is_inf() == 0, 1))
            return new oscar_number_impl(dispatch.pow(julia_elem, k), dispatch, std::true_type());
         else if (k > 0)
            return oscar_number_wrap::create(
                     Rational::infinity(k%2 == 0 ? 1 : this->is_inf())
                   );
         else if (k == 0)
            throw pm::GMP::NaN();
         else
            return oscar_number_wrap::create(Rational(0));
      }

      Int cmp(const oscar_number_wrap* b) const {
         //cerr << "pre-cmp" << endl;
         if (__builtin_expect(this->is_inf() == 0, 1))
            if (__builtin_expect(b->is_inf() == 0, 1))
               return dispatch.cmp(julia_elem, b->for_julia());
         Int res = this->is_inf() - b->is_inf();
         return res < 0 ? -1 : (res > 0 ? 1 : 0);
      }

      bool is_zero() const {
         if (__builtin_expect(this->is_inf() == 0, 1))
            return dispatch.is_zero(julia_elem);
         return false;
      }
      bool is_one() const {
         if (__builtin_expect(this->is_inf() == 0, 1))
            return dispatch.is_one(julia_elem);
         return false;
      }
      Int is_inf() const {
         return infinity;
      }
      Int sign() const {
         if (__builtin_expect(this->is_inf() == 0, 1))
            return dispatch.sign(julia_elem);
         else
            return infinity;
      }
      oscar_number_wrap* abs_value() const {
         //cerr << "pre-abs" << endl;
         if (__builtin_expect(this->is_inf() == 0, 1))
            return new oscar_number_impl(dispatch.abs(julia_elem), dispatch, std::true_type());
         return oscar_number_wrap::create(Rational::infinity(1));
      }

      bool uses_rational() const {
         return false;
      }
      long index() const {
         return dispatch.index;
      }

      std::string to_string() const {
         std::ostringstream str("");
         if (__builtin_expect(this->is_inf() == 0, 1)) {
            static jl_function_t *strfun = jl_get_function(jl_base_module, "string");
            jl_value_t* jstr = jl_call1(strfun, julia_elem);
            JL_GC_PUSH1(&jstr);
            const char* cstr = jl_string_ptr(jstr);
            str << "(" << cstr << ")";
            JL_GC_POP();
         } else {
            str << (infinity > 0 ? "inf" : "-inf");
         }
         return str.str();
      }

   private:
      const oscar_number_dispatch& dispatch;
      jl_value_t* julia_elem = nullptr;
      Int infinity = 0;

   friend class oscar_number_rational_impl;
};

class oscar_number_rational_impl : Rational, public oscar_number_wrap {
public:
   oscar_number_rational_impl(const Rational& r) : Rational(r) { }

   oscar_number_rational_impl() = delete;

   ~oscar_number_rational_impl() { }

   oscar_number_wrap* copy() const {
      return new oscar_number_rational_impl(Rational(*this));
   }

   void destruct() { }

   oscar_number_wrap* upgrade_other(oscar_number_wrap* other) const {
      // this should not be called
      throw std::runtime_error("oscar_number_wrap: error upgrading to rational element");
   }

   oscar_number_wrap* upgrade_to(const oscar_number_dispatch& d) {
      return new oscar_number_impl((Rational) *this, d);
   }

   jl_value_t* for_julia() const {
      // we should probably never end up here
      throw std::runtime_error("oscar_number_rational: not implemented");
      //return (jl_value_t*) nullptr;
   }

   const Rational& as_rational() const {
      return (const Rational&) *this;
   }

   oscar_number_wrap* negate() {
      Rational::negate();
      return this;
   }
   oscar_number_wrap* add(const oscar_number_wrap* other) {
      Rational::operator+=(other->as_rational());
      return this;
   }
   oscar_number_wrap* sub(const oscar_number_wrap* other) {
      Rational::operator-=(other->as_rational());
      return this;
   }
   oscar_number_wrap* mul(const oscar_number_wrap* other) {
      Rational::operator*=(other->as_rational());
      return this;
   }
   oscar_number_wrap* div(const oscar_number_wrap* other) {
      Rational::operator/=(other->as_rational());
      return this;
   }
   oscar_number_wrap* pow(Int p) const {
      return new oscar_number_rational_impl(Rational::pow(*this, p));
   }
   Int cmp(const oscar_number_wrap* other) const {
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
   oscar_number_wrap* abs_value() const {
      return new oscar_number_rational_impl(abs((Rational)*this));
   }

   bool uses_rational() const {
      return true;
   }
   long index() const {
      return 0;
   }

   std::string to_string() const {
      std::ostringstream str;
      str << (Rational) *this;
      return str.str();
   }

   //static oscar_number_wrap* from_string(const std::string& s);
};

oscar_number_wrap* oscar_number_wrap::create(const Rational& r) {
   return new oscar_number_rational_impl(r);
}

oscar_number_wrap* oscar_number_wrap::create(void* e, long index) {
   return new oscar_number_impl(reinterpret_cast<jl_value_t*>(e), index);
}

void oscar_number_wrap::destroy(oscar_number_wrap* j) {
   delete j;
}

using onptr = std::unique_ptr<oscar_number_wrap, void (*)(oscar_number_wrap*)>;

void maybe_upgrade(onptr& a, onptr& b) {
   if (b->uses_rational() && !a->uses_rational())
      b = std::move(onptr(a->upgrade_other(b.get()), &oscar_number_wrap::destroy));
   else if (a->uses_rational() && !b->uses_rational())
      a = std::move(onptr(b->upgrade_other(a.get()), &oscar_number_wrap::destroy));
   else if (a->index() != b->index() && a->index() * b->index() != 0)
      throw std::runtime_error("oscar_number_wrap: different julia fields!");
}

} // end juliainterface

using juliainterface::onptr;

// implementations for on class
OscarNumber::OscarNumber() :
   OscarNumber(Rational(0)) {}

//OscarNumber::~OscarNumber() = default;

OscarNumber::OscarNumber(const Rational& r) :
   impl(juliainterface::oscar_number_wrap::create(r), &juliainterface::oscar_number_wrap::destroy) {}

OscarNumber::OscarNumber(const OscarNumber& on) :
   impl(on.impl->copy(), &juliainterface::oscar_number_wrap::destroy) {
   //cerr << "copied" << endl;
}

OscarNumber::OscarNumber(OscarNumber&& on) :
   impl(std::move(on.impl)) {
   //cerr << "moved" << endl;
}

OscarNumber::OscarNumber(void* jv, Int index) :
   impl(juliainterface::oscar_number_wrap::create(jv, index), &juliainterface::oscar_number_wrap::destroy) {}

OscarNumber& OscarNumber::operator= (const Rational& b) {
   impl = std::move(onptr(juliainterface::oscar_number_wrap::create(b), &juliainterface::oscar_number_wrap::destroy));
   return *this;
}

OscarNumber::OscarNumber(juliainterface::oscar_number_wrap* on) :
   impl(on, &juliainterface::oscar_number_wrap::destroy) {}

OscarNumber& OscarNumber::operator= (const OscarNumber& b) {
   impl = std::move(onptr(b.impl->copy(), &juliainterface::oscar_number_wrap::destroy));
   return *this;
}

OscarNumber& OscarNumber::operator+= (const Rational& b) {
   return *this += OscarNumber(b);
}
OscarNumber& OscarNumber::operator-= (const Rational& b) {
   return *this -= OscarNumber(b);
}
OscarNumber& OscarNumber::operator*= (const Rational& b) {
   return *this *= OscarNumber(b);
}
OscarNumber& OscarNumber::operator/= (const Rational& b) {
   return *this /= OscarNumber(b);
}
//OscarNumber& OscarNumber::operator+= (const Rational& r){
//   auto jfr = juliainterface::oscar_number_wrap::create(r);
//   if (impl->uses_rational()) {
//      impl->add(jfr);
//   } else {
//      auto on = impl->upgrade_other(jfr);
//      impl->add(on);
//      delete on;
//   }
//   delete jfr;
//   return *this;
//}
//OscarNumber& OscarNumber::operator-= (const Rational& r){
//   impl->sub(juliainterface::oscar_number_wrap::create(r));
//   return *this;
//}
//OscarNumber& OscarNumber::operator*= (const Rational& r){
//   impl->mul(juliainterface::oscar_number_wrap::create(r));
//   return *this;
//}
//OscarNumber& OscarNumber::operator/= (const Rational& r){
//   impl->div(juliainterface::oscar_number_wrap::create(r));
//   return *this;
//}

OscarNumber& OscarNumber::operator+= (const OscarNumber& b){
   juliainterface::maybe_upgrade(impl, b.impl);
   impl->add(b.impl.get());
   return *this;
}
OscarNumber& OscarNumber::operator-= (const OscarNumber& b){
   juliainterface::maybe_upgrade(impl, b.impl);
   impl->sub(b.impl.get());
   return *this;
}
OscarNumber& OscarNumber::operator*= (const OscarNumber& b){
   juliainterface::maybe_upgrade(impl, b.impl);
   impl->mul(b.impl.get());
   return *this;
}
OscarNumber& OscarNumber::operator/= (const OscarNumber& b){
   juliainterface::maybe_upgrade(impl, b.impl);
   impl->div(b.impl.get());
   return *this;

}

OscarNumber& OscarNumber::negate() {
   impl->negate();
   return *this;
}

OscarNumber pow(const OscarNumber& a, Int k) {
   return OscarNumber(a.impl->pow(k));
}

Int OscarNumber::cmp(const OscarNumber& b) const {
   juliainterface::maybe_upgrade(impl, b.impl);
   return impl->cmp(b.impl.get());
}

Int OscarNumber::cmp(const Rational& r) const {
   return this->cmp(OscarNumber(r));
}

bool OscarNumber::is_zero() const {
   return impl->is_zero();
}
bool OscarNumber::is_one() const {
   return impl->is_one();
}

Int OscarNumber::is_inf() const {
   return impl->is_inf();
}

Int OscarNumber::sign() const {
   return impl->sign();
}

OscarNumber abs(const OscarNumber& on) {
   return OscarNumber(on.impl->abs_value());
}

OscarNumber::operator Rational() const {
   return Rational(impl->as_rational());
}

OscarNumber::operator double() const {
   // TODO dont go via rational
   return double(impl->as_rational());
}

bool OscarNumber::uses_rational() const {
   return impl->uses_rational();
}

void* OscarNumber::unsafe_get() const {
   return reinterpret_cast<void*>(impl->for_julia());
}

std::string OscarNumber::to_string() const {
   return impl->to_string();
}

void OscarNumber::register_oscar_number(void* disp, long index) {
   using namespace juliainterface;
   if (oscar_number_map.find(index) != oscar_number_map.end())
      throw std::runtime_error("polymake::OscarNumber: cannot re-register field index");

   oscar_number_dispatch dispatch;
   dispatch.index = index;
   oscar_number_dispatch_helper* helper = reinterpret_cast<oscar_number_dispatch_helper*>(disp);
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

   oscar_number_map.emplace(index, std::move(dispatch));
}

} }

namespace pm {

bool
spec_object_traits< polymake::common::OscarNumber>::is_zero(const persistent_type& p)
{
   return p.is_zero();
}
bool
spec_object_traits< polymake::common::OscarNumber>::is_one(const persistent_type& p)
{
   return p.is_one();
}
const polymake::common::OscarNumber&
spec_object_traits< polymake::common::OscarNumber>::zero()
{
   static const persistent_type x(0);
   return x;
}
const polymake::common::OscarNumber&
spec_object_traits< polymake::common::OscarNumber>::one()
{
   static const persistent_type x(1);
   return x;
}

}
