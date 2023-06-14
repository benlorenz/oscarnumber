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

#ifndef POLYMAKE_COMMON_OSCARNUMBER_H
#define POLYMAKE_COMMON_OSCARNUMBER_H

#include "polymake/client.h"
#include "polymake/Polynomial.h"
#include "polymake/Integer.h"
#include "polymake/Array.h"

namespace polymake { namespace common {

namespace juliainterface {

class oscar_number_wrap;

}

class OscarNumber;

} }

namespace pm {

template <>
struct spec_object_traits< polymake::common::OscarNumber>
   : spec_object_traits<is_scalar> {
   typedef polymake::common::OscarNumber persistent_type;
   typedef void generic_type;
   typedef is_scalar generic_tag;

   static
   bool is_zero(const persistent_type& p);

   static
   bool is_one(const persistent_type& p);

   static
   const persistent_type& zero();

   static
   const persistent_type& one();
};

}


namespace polymake { namespace common {

   // this is currently only for fields that embed the rational numbers
//template <long index>
class OscarNumber {
   private:
      std::unique_ptr<juliainterface::oscar_number_wrap, void(*)(juliainterface::oscar_number_wrap*)> impl;

      explicit OscarNumber(juliainterface::oscar_number_wrap* x);

   public:

      // constructors

      // 0 in Q
      OscarNumber();

      ~OscarNumber() = default;

      // x in Q
      explicit OscarNumber(const Rational& x);

      OscarNumber(const OscarNumber& x);
      OscarNumber(OscarNumber&& x);

      OscarNumber(void* x, Int index);

      OscarNumber& operator= (const Rational& b);
      OscarNumber& operator= (const OscarNumber& b);

      //
      OscarNumber& operator+= (const Rational& b);
      OscarNumber& operator-= (const Rational& b);
      OscarNumber& operator*= (const Rational& b);
      OscarNumber& operator/= (const Rational& b);
      OscarNumber& operator+= (const OscarNumber& b);
      OscarNumber& operator-= (const OscarNumber& b);
      OscarNumber& operator*= (const OscarNumber& b);
      OscarNumber& operator/= (const OscarNumber& b);

      OscarNumber& negate();

      friend OscarNumber& negate(OscarNumber& nf) {
         return nf.negate();
      }

      friend OscarNumber pow(const OscarNumber& a, Int k);

      Int cmp(const OscarNumber& b) const;
      Int cmp(const Rational& b) const;

      bool is_zero() const;
      bool is_one() const;
      Int is_inf() const;
      Int sign() const;

      friend OscarNumber abs(const OscarNumber& on);

      // infinity
      // sgn: -1 for -inf, 1 for inf and 0 for 0
      static OscarNumber infinity(Int sgn);

      // conversion
      explicit operator double() const;
      explicit operator Rational() const;

      bool uses_rational() const;

      void* unsafe_get() const;

      // TODO check
      inline friend void relocate(OscarNumber* from, OscarNumber* to) {
         pm::relocate(from,to);
      }

      inline friend
      Int isinf(const OscarNumber& a) noexcept
      {
         return a.is_inf();
      }
      // redirecting:

      // construction from compatible types via Rational
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      explicit OscarNumber(const T& x) : OscarNumber(Rational(x)) {}

      //void set_inf(Int sgn);
      //void set_inf(const NumberField& nf, Int sgn);

      bool is_finite() const {
         return this->is_inf() == 0;
      }

      // Assignment
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      OscarNumber& operator= (const T& b)
      {
         return *this = Rational(b);
      }
      inline friend Int sign(const OscarNumber& on) {
         return on.sign();
      }

      // Unary Minus
      inline friend OscarNumber operator- (const OscarNumber& a) {
         return std::move(OscarNumber(a).negate());
      }

      friend bool abs_equal(const OscarNumber& on1, const OscarNumber& on2);

      // Arithmetic +
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      OscarNumber& operator+= (const T& b)
      {
         return *this += Rational(b);
      }

      inline friend OscarNumber operator+ (const OscarNumber& a, const OscarNumber& b)
      {
         return std::move(OscarNumber(a) += b);
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend OscarNumber operator+ (const OscarNumber& a, const T& b)
      {
         return std::move(OscarNumber(a) += Rational(b));
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend OscarNumber operator+ (const T& a, const OscarNumber& b)
      {
         return b+a;
      }

      // Arithmetic -
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      OscarNumber& operator-= (const T& b)
      {
         return *this -= Rational(b);
      }

      inline friend OscarNumber operator- (const OscarNumber& a, const OscarNumber& b)
      {
         return std::move(OscarNumber(a) -= b);
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend OscarNumber operator- (const OscarNumber& a, const T& b)
      {
         return std::move(OscarNumber(a) -= Rational(b));
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend OscarNumber operator- (const T& a, const OscarNumber& b)
      {
         return (-b)+a;
      }

      // Arithmetic *
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      OscarNumber& operator*= (const T& b)
      {
         return *this *= Rational(b);
      }

      inline friend OscarNumber operator* (const OscarNumber& a, const OscarNumber& b)
      {
         return std::move(OscarNumber(a) *= b);
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend OscarNumber operator* (const OscarNumber& a, const T& b)
      {
         return std::move(OscarNumber(a) *= Rational(b));
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend OscarNumber operator* (const T& a, const OscarNumber& b)
      {
         return b*a;
      }

      // Arithmetic *
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      OscarNumber& operator/= (const T& b)
      {
         return *this /= Rational(b);
      }

      inline friend OscarNumber operator/ (const OscarNumber& a, const OscarNumber& b)
      {
         return std::move(OscarNumber(a) /= b);
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend OscarNumber operator/ (const OscarNumber& a, const T& b)
      {
         return std::move(OscarNumber(a) /= Rational(b));
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend OscarNumber operator/ (const T& a, const OscarNumber& b)
      {
         return std::move(OscarNumber(a) /= b);
      }


      // comparison
      inline friend bool operator== (const OscarNumber& a, const OscarNumber& b)
      {
         return a.cmp(b) == 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator== (const OscarNumber& a, const T& b)
      {
         return a.cmp(Rational(b)) == 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator== (const T& a, const OscarNumber& b)
      {
         return b.cmp(a) == 0;
      }

      // comparison
      inline friend bool operator!= (const OscarNumber& a, const OscarNumber& b)
      {
         return a.cmp(b) != 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator!= (const OscarNumber& a, const T& b)
      {
         return a.cmp(Rational(b)) != 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator!= (const T& a, const OscarNumber& b)
      {
         return b.cmp(a) != 0;
      }

      inline friend bool operator< (const OscarNumber& a, const OscarNumber& b)
      {
         return a.cmp(b) < 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator< (const OscarNumber& a, const T& b)
      {
         return a.cmp(Rational(b)) < 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator< (const T& a, const OscarNumber& b)
      {
         return b.cmp(a) > 0;
      }

      inline friend bool operator<= (const OscarNumber& a, const OscarNumber& b)
      {
         return a.cmp(b) <= 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator<= (const OscarNumber& a, const T& b)
      {
         return a.cmp(Rational(b)) <= 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator<= (const T& a, const OscarNumber& b)
      {
         return b.cmp(a) >= 0;
      }

      inline friend bool operator> (const OscarNumber& a, const OscarNumber& b)
      {
         return a.cmp(b) > 0;
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator> (const OscarNumber& a, const T& b)
      {
         return a.cmp(Rational(b)) > 0;
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator> (const T& a, const OscarNumber& b)
      {
         return b.cmp(a) < 0;
      }

      inline friend bool operator>= (const OscarNumber& a, const OscarNumber& b)
      {
         return a.cmp(b) >= 0;
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator>= (const OscarNumber& a, const T& b)
      {
         return a.cmp(Rational(b)) >= 0;
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator>= (const T& a, const OscarNumber& b)
      {
         return b.cmp(a) <= 0;
      }

      std::string to_string() const;

      std::string to_serialized() const;

      static void register_oscar_number(void* dispatch_helper, long index);

   }; // end OscarNumber

inline bool abs_equal(const polymake::common::OscarNumber& on1,const polymake::common::OscarNumber& on2) {
   return abs(on1).cmp(on2) == 0;
}

} }


namespace pm {

// convince polymake to properly deserialize this as a composite object even though it is
// used as a scalar
template <>
struct has_serialized<polymake::common::OscarNumber> : std::true_type {};
/*
template<>
   struct spec_object_traits< Serialized < polymake::common::OscarNumberSerializer> >
   : spec_object_traits<is_composite> {
      typedef polymake::common::OscarNumberSerializer masquerade_for;

      typedef cons<std::string> elements;

      template <typename Me, typename Visitor>
         static void visit_elements(Me& me, Visitor& v)
         {
            v << std::get<0>(me);
         }
   };
*/
template <typename Output>
   Output& operator<< (GenericOutput<Output>& out, const polymake::common::OscarNumber& me) {
      out.top() << me.to_string();
      return out.top();
};
/*
template <typename Input>
Input& operator>> (GenericInput<Input>& is, polymake::common::OscarNumber& on)
{
   polymake::common::OscarNumberSerializer p;
   is.top() >> serialize(p);
   on = polymake::common::OscarNumber::from_string(std::get<0>(p));
   return is.top();
}
*/
template <>
struct algebraic_traits<polymake::common::OscarNumber> {
   typedef polymake::common::OscarNumber field_type;
};

inline
bool isfinite(const polymake::common::OscarNumber& a) noexcept
{
   return a.is_finite();
}


}

namespace std {
   template <>
   class numeric_limits<polymake::common::OscarNumber>
      : public numeric_limits<pm::Rational> {
   public:
      static polymake::common::OscarNumber min() { return polymake::common::OscarNumber::infinity(-1); }
      static polymake::common::OscarNumber max() { return polymake::common::OscarNumber::infinity(1); }
      static polymake::common::OscarNumber infinity() { return polymake::common::OscarNumber::infinity(1); }
   };
}

#endif
