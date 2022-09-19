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

#ifndef POLYMAKE_COMMON_JULIAFIELDELEMENT_H
#define POLYMAKE_COMMON_JULIAFIELDELEMENT_H

#include "polymake/client.h"
#include "polymake/Polynomial.h"
#include "polymake/Integer.h"
#include "polymake/Array.h"

namespace polymake { namespace common {

namespace juliainterface {

class julia_field_wrap;

}

class JuliaFieldElement;

} }

namespace pm {

template <>
struct spec_object_traits< polymake::common::JuliaFieldElement>
   : spec_object_traits<is_scalar> {
   typedef polymake::common::JuliaFieldElement persistent_type;
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
class JuliaFieldElement {
   private:
      mutable std::unique_ptr<juliainterface::julia_field_wrap, void(*)(juliainterface::julia_field_wrap*)> impl;

      explicit JuliaFieldElement(juliainterface::julia_field_wrap* x);

   public:

      // constructors

      // 0 in Q
      JuliaFieldElement();

      ~JuliaFieldElement() = default;

      // x in Q
      explicit JuliaFieldElement(const Rational& x);

      JuliaFieldElement(const JuliaFieldElement& x);
      JuliaFieldElement(JuliaFieldElement&& x);

      JuliaFieldElement(void* x, Int index);

      JuliaFieldElement& operator= (const Rational& b);
      JuliaFieldElement& operator= (const JuliaFieldElement& b);

      //
      JuliaFieldElement& operator+= (const Rational& b);
      JuliaFieldElement& operator-= (const Rational& b);
      JuliaFieldElement& operator*= (const Rational& b);
      JuliaFieldElement& operator/= (const Rational& b);
      JuliaFieldElement& operator+= (const JuliaFieldElement& b);
      JuliaFieldElement& operator-= (const JuliaFieldElement& b);
      JuliaFieldElement& operator*= (const JuliaFieldElement& b);
      JuliaFieldElement& operator/= (const JuliaFieldElement& b);

      JuliaFieldElement& negate();

      friend JuliaFieldElement& negate(JuliaFieldElement& nf) {
         return nf.negate();
      }

      friend JuliaFieldElement pow(const JuliaFieldElement& a, Int k);

      Int cmp(const JuliaFieldElement& b) const;
      Int cmp(const Rational& b) const;

      bool is_zero() const;
      bool is_one() const;
      Int is_inf() const;
      Int sign() const;

      friend JuliaFieldElement abs(const JuliaFieldElement& jf);

      // infinity
      // sgn: -1 for -inf, 1 for inf and 0 for 0
      static JuliaFieldElement infinity(Int sgn);

      // conversion
      explicit operator double() const;
      explicit operator Rational() const;


      // TODO check
      inline friend void relocate(JuliaFieldElement* from, JuliaFieldElement* to) {
         pm::relocate(from,to);
      }

      inline friend
      Int isinf(const JuliaFieldElement& a) noexcept
      {
         return a.is_inf();
      }
      // redirecting:

      // construction from compatible types via Rational
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      explicit JuliaFieldElement(const T& x) : JuliaFieldElement(Rational(x)) {}

      //void set_inf(Int sgn);
      //void set_inf(const NumberField& nf, Int sgn);

      bool is_finite() const {
         return this->is_inf() == 0;
      }

      // Assignment
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      JuliaFieldElement& operator= (const T& b)
      {
         return *this = Rational(b);
      }
      inline friend Int sign(const JuliaFieldElement& jf) {
         return jf.sign();
      }

      // Unary Minus
      inline friend JuliaFieldElement operator- (const JuliaFieldElement& a) {
         return std::move(JuliaFieldElement(a).negate());
      }

      friend bool abs_equal(const JuliaFieldElement& jf1, const JuliaFieldElement& jf2);

      // Arithmetic +
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      JuliaFieldElement& operator+= (const T& b)
      {
         return *this += Rational(b);
      }

      inline friend JuliaFieldElement operator+ (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return std::move(JuliaFieldElement(a) += b);
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend JuliaFieldElement operator+ (const JuliaFieldElement& a, const T& b)
      {
         return std::move(JuliaFieldElement(a) += Rational(b));
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend JuliaFieldElement operator+ (const T& a, const JuliaFieldElement& b)
      {
         return b+a;
      }

      // Arithmetic -
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      JuliaFieldElement& operator-= (const T& b)
      {
         return *this -= Rational(b);
      }

      inline friend JuliaFieldElement operator- (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return std::move(JuliaFieldElement(a) -= b);
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend JuliaFieldElement operator- (const JuliaFieldElement& a, const T& b)
      {
         return std::move(JuliaFieldElement(a) -= Rational(b));
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend JuliaFieldElement operator- (const T& a, const JuliaFieldElement& b)
      {
         return (-b)+a;
      }

      // Arithmetic *
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      JuliaFieldElement& operator*= (const T& b)
      {
         return *this *= Rational(b);
      }

      inline friend JuliaFieldElement operator* (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return std::move(JuliaFieldElement(a) *= b);
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend JuliaFieldElement operator* (const JuliaFieldElement& a, const T& b)
      {
         return std::move(JuliaFieldElement(a) *= Rational(b));
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend JuliaFieldElement operator* (const T& a, const JuliaFieldElement& b)
      {
         return b*a;
      }

      // Arithmetic *
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      JuliaFieldElement& operator/= (const T& b)
      {
         return *this /= Rational(b);
      }

      inline friend JuliaFieldElement operator/ (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return std::move(JuliaFieldElement(a) /= b);
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend JuliaFieldElement operator/ (const JuliaFieldElement& a, const T& b)
      {
         return std::move(JuliaFieldElement(a) /= Rational(b));
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      inline friend JuliaFieldElement operator/ (const T& a, const JuliaFieldElement& b)
      {
         return std::move(JuliaFieldElement(a) /= b);
      }


      // comparison
      inline friend bool operator== (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return a.cmp(b) == 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator== (const JuliaFieldElement& a, const T& b)
      {
         return a.cmp(Rational(b)) == 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator== (const T& a, const JuliaFieldElement& b)
      {
         return b.cmp(a) == 0;
      }

      // comparison
      inline friend bool operator!= (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return a.cmp(b) != 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator!= (const JuliaFieldElement& a, const T& b)
      {
         return a.cmp(Rational(b)) != 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator!= (const T& a, const JuliaFieldElement& b)
      {
         return b.cmp(a) != 0;
      }

      inline friend bool operator< (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return a.cmp(b) < 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator< (const JuliaFieldElement& a, const T& b)
      {
         return a.cmp(Rational(b)) < 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator< (const T& a, const JuliaFieldElement& b)
      {
         return b.cmp(a) > 0;
      }

      inline friend bool operator<= (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return a.cmp(b) <= 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator<= (const JuliaFieldElement& a, const T& b)
      {
         return a.cmp(Rational(b)) <= 0;
      }
      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator<= (const T& a, const JuliaFieldElement& b)
      {
         return b.cmp(a) >= 0;
      }

      inline friend bool operator> (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return a.cmp(b) > 0;
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator> (const JuliaFieldElement& a, const T& b)
      {
         return a.cmp(Rational(b)) > 0;
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator> (const T& a, const JuliaFieldElement& b)
      {
         return b.cmp(a) < 0;
      }

      inline friend bool operator>= (const JuliaFieldElement& a, const JuliaFieldElement& b)
      {
         return a.cmp(b) >= 0;
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator>= (const JuliaFieldElement& a, const T& b)
      {
         return a.cmp(Rational(b)) >= 0;
      }

      template <typename T, typename=std::enable_if_t<pm::can_initialize<pure_type_t<T>, Rational>::value>>
      friend bool operator>= (const T& a, const JuliaFieldElement& b)
      {
         return b.cmp(a) <= 0;
      }

      std::string to_string() const;

      std::string to_serialized() const;

      static void register_julia_field(void* dispatch_helper, long index);

   }; // end JuliaFieldElement

inline bool abs_equal(const polymake::common::JuliaFieldElement& jf1,const polymake::common::JuliaFieldElement& jf2) {
   return abs(jf1).cmp(jf2) == 0;
}

} }


namespace pm {

// convince polymake to properly deserialize this as a composite object even though it is
// used as a scalar
template <>
struct has_serialized<polymake::common::JuliaFieldElement> : std::true_type {};
/*
template<>
   struct spec_object_traits< Serialized < polymake::common::JuliaFieldElementSerializer> >
   : spec_object_traits<is_composite> {
      typedef polymake::common::JuliaFieldElementSerializer masquerade_for;

      typedef cons<std::string> elements;

      template <typename Me, typename Visitor>
         static void visit_elements(Me& me, Visitor& v)
         {
            v << std::get<0>(me);
         }
   };
*/
template <typename Output>
   Output& operator<< (GenericOutput<Output>& out, const polymake::common::JuliaFieldElement& me) {
      out.top() << me.to_string();
      return out.top();
};
/*
template <typename Input>
Input& operator>> (GenericInput<Input>& is, polymake::common::JuliaFieldElement& jfe)
{
   polymake::common::JuliaFieldElementSerializer p;
   is.top() >> serialize(p);
   jfe = polymake::common::JuliaFieldElement::from_string(std::get<0>(p));
   return is.top();
}
*/
template <>
struct algebraic_traits<polymake::common::JuliaFieldElement> {
   typedef polymake::common::JuliaFieldElement field_type;
};

inline
bool isfinite(const polymake::common::JuliaFieldElement& a) noexcept
{
   return a.is_finite();
}


}

namespace std {
   template <>
   class numeric_limits<polymake::common::JuliaFieldElement>
      : public numeric_limits<pm::Rational> {
   public:
      static polymake::common::JuliaFieldElement min() { return polymake::common::JuliaFieldElement::infinity(-1); }
      static polymake::common::JuliaFieldElement max() { return polymake::common::JuliaFieldElement::infinity(1); }
      static polymake::common::JuliaFieldElement infinity() { return polymake::common::JuliaFieldElement::infinity(1); }
   };
}

#endif
