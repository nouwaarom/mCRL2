// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/modal_formula/action_formula.h
/// \brief Add your file description here.

#ifndef MCRL2_MODAL_ACTION_FORMULA_H
#define MCRL2_MODAL_ACTION_FORMULA_H

#include <iostream> // for debugging
#include <stdexcept>
#include <string>
#include <cassert>
#include "mcrl2/atermpp/aterm_appl.h"
#include "mcrl2/core/detail/precedence.h"
#include "mcrl2/data/data_specification.h"
#include "mcrl2/lps/action.h"
#include "mcrl2/lps/untyped_action.h"

namespace mcrl2
{

namespace action_formulas
{

//--- start generated classes ---//
/// \brief An action formula
class action_formula: public atermpp::aterm_appl
{
  public:
    /// \brief Default constructor.
    action_formula()
      : atermpp::aterm_appl(core::detail::constructActFrm())
    {}

    /// \brief Constructor.
    /// \param term A term
    action_formula(const atermpp::aterm& term)
      : atermpp::aterm_appl(term)
    {
      assert(core::detail::check_rule_ActFrm(*this));
    }
};

/// \brief list of action_formulas
typedef atermpp::term_list<action_formula> action_formula_list;

/// \brief vector of action_formulas
typedef std::vector<action_formula>    action_formula_vector;


/// \brief Test for a action_formula expression
/// \param t A term
/// \return True if it is a action_formula expression
inline
bool is_action_formula(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActFrm(t);
}


/// \brief The value true for action formulas
class true_: public action_formula
{
  public:
    /// \brief Default constructor.
    true_()
      : action_formula(core::detail::constructActTrue())
    {}

    /// \brief Constructor.
    /// \param term A term
    true_(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActTrue(*this));
    }
};

/// \brief Test for a true expression
/// \param t A term
/// \return True if it is a true expression
inline
bool is_true(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActTrue(t);
}


/// \brief The value false for action formulas
class false_: public action_formula
{
  public:
    /// \brief Default constructor.
    false_()
      : action_formula(core::detail::constructActFalse())
    {}

    /// \brief Constructor.
    /// \param term A term
    false_(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActFalse(*this));
    }
};

/// \brief Test for a false expression
/// \param t A term
/// \return True if it is a false expression
inline
bool is_false(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActFalse(t);
}


/// \brief The not operator for action formulas
class not_: public action_formula
{
  public:
    /// \brief Default constructor.
    not_()
      : action_formula(core::detail::constructActNot())
    {}

    /// \brief Constructor.
    /// \param term A term
    not_(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActNot(*this));
    }

    /// \brief Constructor.
    not_(const action_formula& operand)
      : action_formula(core::detail::gsMakeActNot(operand))
    {}

    const action_formula& operand() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg1(*this));
    }
};

/// \brief Test for a not expression
/// \param t A term
/// \return True if it is a not expression
inline
bool is_not(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActNot(t);
}


/// \brief The and operator for action formulas
class and_: public action_formula
{
  public:
    /// \brief Default constructor.
    and_()
      : action_formula(core::detail::constructActAnd())
    {}

    /// \brief Constructor.
    /// \param term A term
    and_(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActAnd(*this));
    }

    /// \brief Constructor.
    and_(const action_formula& left, const action_formula& right)
      : action_formula(core::detail::gsMakeActAnd(left, right))
    {}

    const action_formula& left() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg1(*this));
    }

    const action_formula& right() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg2(*this));
    }
};

/// \brief Test for a and expression
/// \param t A term
/// \return True if it is a and expression
inline
bool is_and(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActAnd(t);
}


/// \brief The or operator for action formulas
class or_: public action_formula
{
  public:
    /// \brief Default constructor.
    or_()
      : action_formula(core::detail::constructActOr())
    {}

    /// \brief Constructor.
    /// \param term A term
    or_(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActOr(*this));
    }

    /// \brief Constructor.
    or_(const action_formula& left, const action_formula& right)
      : action_formula(core::detail::gsMakeActOr(left, right))
    {}

    const action_formula& left() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg1(*this));
    }

    const action_formula& right() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg2(*this));
    }
};

/// \brief Test for a or expression
/// \param t A term
/// \return True if it is a or expression
inline
bool is_or(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActOr(t);
}


/// \brief The implication operator for action formulas
class imp: public action_formula
{
  public:
    /// \brief Default constructor.
    imp()
      : action_formula(core::detail::constructActImp())
    {}

    /// \brief Constructor.
    /// \param term A term
    imp(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActImp(*this));
    }

    /// \brief Constructor.
    imp(const action_formula& left, const action_formula& right)
      : action_formula(core::detail::gsMakeActImp(left, right))
    {}

    const action_formula& left() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg1(*this));
    }

    const action_formula& right() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg2(*this));
    }
};

/// \brief Test for a imp expression
/// \param t A term
/// \return True if it is a imp expression
inline
bool is_imp(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActImp(t);
}


/// \brief The universal quantification operator for action formulas
class forall: public action_formula
{
  public:
    /// \brief Default constructor.
    forall()
      : action_formula(core::detail::constructActForall())
    {}

    /// \brief Constructor.
    /// \param term A term
    forall(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActForall(*this));
    }

    /// \brief Constructor.
    forall(const data::variable_list& variables, const action_formula& body)
      : action_formula(core::detail::gsMakeActForall(variables, body))
    {}

    const data::variable_list& variables() const
    {
      return atermpp::aterm_cast<const data::variable_list>(atermpp::list_arg1(*this));
    }

    const action_formula& body() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg2(*this));
    }
};

/// \brief Test for a forall expression
/// \param t A term
/// \return True if it is a forall expression
inline
bool is_forall(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActForall(t);
}


/// \brief The existential quantification operator for action formulas
class exists: public action_formula
{
  public:
    /// \brief Default constructor.
    exists()
      : action_formula(core::detail::constructActExists())
    {}

    /// \brief Constructor.
    /// \param term A term
    exists(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActExists(*this));
    }

    /// \brief Constructor.
    exists(const data::variable_list& variables, const action_formula& body)
      : action_formula(core::detail::gsMakeActExists(variables, body))
    {}

    const data::variable_list& variables() const
    {
      return atermpp::aterm_cast<const data::variable_list>(atermpp::list_arg1(*this));
    }

    const action_formula& body() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg2(*this));
    }
};

/// \brief Test for a exists expression
/// \param t A term
/// \return True if it is a exists expression
inline
bool is_exists(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActExists(t);
}


/// \brief The at operator for action formulas
class at: public action_formula
{
  public:
    /// \brief Default constructor.
    at()
      : action_formula(core::detail::constructActAt())
    {}

    /// \brief Constructor.
    /// \param term A term
    at(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActAt(*this));
    }

    /// \brief Constructor.
    at(const action_formula& operand, const data::data_expression& time_stamp)
      : action_formula(core::detail::gsMakeActAt(operand, time_stamp))
    {}

    const action_formula& operand() const
    {
      return atermpp::aterm_cast<const action_formula>(atermpp::arg1(*this));
    }

    const data::data_expression& time_stamp() const
    {
      return atermpp::aterm_cast<const data::data_expression>(atermpp::arg2(*this));
    }
};

/// \brief Test for a at expression
/// \param t A term
/// \return True if it is a at expression
inline
bool is_at(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActAt(t);
}


/// \brief The multi action for action formulas
class multi_action: public action_formula
{
  public:
    /// \brief Default constructor.
    multi_action()
      : action_formula(core::detail::constructActMultAct())
    {}

    /// \brief Constructor.
    /// \param term A term
    multi_action(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActMultAct(*this));
    }

    /// \brief Constructor.
    multi_action(const lps::action_list& actions)
      : action_formula(core::detail::gsMakeActMultAct(actions))
    {}

    const lps::action_list& actions() const
    {
      return atermpp::aterm_cast<const lps::action_list>(atermpp::list_arg1(*this));
    }
};

/// \brief Test for a multi_action expression
/// \param t A term
/// \return True if it is a multi_action expression
inline
bool is_multi_action(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActMultAct(t);
}


/// \brief The multi action for action formulas (untyped)
class untyped_multi_action: public action_formula
{
  public:
    /// \brief Default constructor.
    untyped_multi_action()
      : action_formula(core::detail::constructActUntypedMultAct())
    {}

    /// \brief Constructor.
    /// \param term A term
    untyped_multi_action(const atermpp::aterm& term)
      : action_formula(term)
    {
      assert(core::detail::check_term_ActUntypedMultAct(*this));
    }

    /// \brief Constructor.
    untyped_multi_action(const lps::untyped_action_list& arguments)
      : action_formula(core::detail::gsMakeActUntypedMultAct(arguments))
    {}

    const lps::untyped_action_list& arguments() const
    {
      return atermpp::aterm_cast<const lps::untyped_action_list>(atermpp::list_arg1(*this));
    }
};

/// \brief Test for a untyped_multi_action expression
/// \param t A term
/// \return True if it is a untyped_multi_action expression
inline
bool is_untyped_multi_action(const atermpp::aterm_appl& t)
{
  return core::detail::gsIsActUntypedMultAct(t);
}

//--- end generated classes ---//

inline
int precedence(const action_formula& x)
{
  if (is_forall(x) || is_exists(x))
  {
    return 0;
  }
  else if (is_imp(x))
  {
    return 2;
  }
  else if (is_or(x))
  {
    return 3;
  }
  else if (is_and(x))
  {
    return 4;
  }
  else if (is_at(x))
  {
    return 5;
  }
  else if (is_not(x))
  {
    return 6;
  }
  return core::detail::precedences::max_precedence;
}

// TODO: is there a cleaner way to make the precedence function work for derived classes like and_ ?
inline int precedence(const forall& x) { return precedence(static_cast<const action_formula&>(x)); }
inline int precedence(const exists& x) { return precedence(static_cast<const action_formula&>(x)); }
inline int precedence(const imp& x) { return precedence(static_cast<const action_formula&>(x)); }
inline int precedence(const and_& x) { return precedence(static_cast<const action_formula&>(x)); }
inline int precedence(const or_& x) { return precedence(static_cast<const action_formula&>(x)); }
inline int precedence(const at& x) { return precedence(static_cast<const action_formula&>(x)); }
inline int precedence(const not_& x) { return precedence(static_cast<const action_formula&>(x)); }

inline const action_formula& unary_operand(const not_& x) { return x.operand(); }
inline const action_formula& unary_operand(const at& x)   { return x.operand(); }
inline const action_formula& binary_left(const and_& x)   { return x.left(); }
inline const action_formula& binary_right(const and_& x)  { return x.right(); }
inline const action_formula& binary_left(const or_& x)    { return x.left(); }
inline const action_formula& binary_right(const or_& x)   { return x.right(); }
inline const action_formula& binary_left(const imp& x)    { return x.left(); }
inline const action_formula& binary_right(const imp& x)   { return x.right(); }

// template function overloads
std::string pp(const action_formula& x);
std::set<data::variable> find_variables(const action_formulas::action_formula& x);

} // namespace action_formulas

} // namespace mcrl2

namespace std {
//--- start generated swap functions ---//
template <>
inline void swap(mcrl2::action_formulas::action_formula& t1, mcrl2::action_formulas::action_formula& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::true_& t1, mcrl2::action_formulas::true_& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::false_& t1, mcrl2::action_formulas::false_& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::not_& t1, mcrl2::action_formulas::not_& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::and_& t1, mcrl2::action_formulas::and_& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::or_& t1, mcrl2::action_formulas::or_& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::imp& t1, mcrl2::action_formulas::imp& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::forall& t1, mcrl2::action_formulas::forall& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::exists& t1, mcrl2::action_formulas::exists& t2)
{
  t1.swap(t2);
}

template <>
inline void swap(mcrl2::action_formulas::at& t1, mcrl2::action_formulas::at& t2)
{
  t1.swap(t2);
}
//--- end generated swap functions ---//
} // namespace std

#endif // MCRL2_MODAL_ACTION_FORMULA_H
