// Author(s): Wieger Wesselink
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/pbes_expression_builder.h
/// \brief Visitor class for rebuilding a pbes expression.

#ifndef MCRL2_PBES_PBES_EXPRESSION_BUILDER_H
#define MCRL2_PBES_PBES_EXPRESSION_BUILDER_H

#include <stdexcept>
#include "mcrl2/pbes/pbes_expression.h"

namespace mcrl2 {

namespace pbes_system {

/// Visitor class for visiting the nodes of a pbes expression. During traversal
/// of the nodes, the expression is rebuilt from scratch.
/// If a visit_<node> function returns pbes_expression(), the recursion is continued
/// in the children of this node, otherwise not.
// TODO: rebuilding expressions with ATerms is very expensive. So it is probably
// more efficient to  check if the children of a node have actually changed,
// before rebuilding it.
struct pbes_expression_builder
{
  /// Destructor.
  ///
  virtual ~pbes_expression_builder()
  { }

  /// Visit data expression node.
  ///
  virtual pbes_expression visit_data_expression(const pbes_expression& /* e */, const data::data_expression& d)
  {
    return pbes_expression();
  }

  /// Visit true node.
  ///
  virtual pbes_expression visit_true(const pbes_expression& /* e */)
  {
    return pbes_expression();
  }

  /// Visit false node.
  ///
  virtual pbes_expression visit_false(const pbes_expression& /* e */)
  {
    return pbes_expression();
  }

  /// Visit not node.
  ///
  virtual pbes_expression visit_not(const pbes_expression& /* e */, const pbes_expression& /* arg */)
  {
    return pbes_expression();
  }

  /// Visit and node.
  ///
  virtual pbes_expression visit_and(const pbes_expression& /* e */, const pbes_expression& /* left */, const pbes_expression& /* right */)
  {
    return pbes_expression();
  }

  /// Visit or node.
  ///
  virtual pbes_expression visit_or(const pbes_expression& /* e */, const pbes_expression& /* left */, const pbes_expression& /* right */)
  {
    return pbes_expression();
  }    

  /// Visit imp node.
  ///
  virtual pbes_expression visit_imp(const pbes_expression& /* e */, const pbes_expression& /* left */, const pbes_expression& /* right */)
  {
    return pbes_expression();
  }

  /// Visit forall node.
  ///
  virtual pbes_expression visit_forall(const pbes_expression& /* e */, const data::data_variable_list& /* variables */, const pbes_expression& /* expression */)
  {
    return pbes_expression();
  }

  /// Visit exists node.
  ///
  virtual pbes_expression visit_exists(const pbes_expression& /* e */, const data::data_variable_list& /* variables */, const pbes_expression& /* expression */)
  {
    return pbes_expression();
  }

  /// Visit propositional variable node.
  ///
  virtual pbes_expression visit_propositional_variable(const pbes_expression& /* e */, const propositional_variable_instantiation& /* v */)
  {
    return pbes_expression();
  }
  
  /// Visit unknown node. This function is called whenever a node of unknown type is encountered.
  /// By default a std::runtime_error exception will be generated.
  ///
  virtual pbes_expression visit_unknown(const pbes_expression& e)
  {
    throw std::runtime_error(std::string("error in pbes_expression_builder::visit() : unknown pbes expression ") + e.to_string());
    return pbes_expression();
  }

  /// Visits the nodes of the pbes expression, and calls the corresponding visit_<node>
  /// member functions. If the return value of a visit function equals pbes_expression(),
  /// the recursion in this node is continued automatically, otherwise the returned
  /// value is used for rebuilding the expression.
  pbes_expression visit(const pbes_expression& e)
  {
    using namespace pbes_expr_optimized;
    using namespace accessors;

    pbes_expression result;

    if (is_data(e)) {
      result = visit_data_expression(e, val(e));
      if (result == pbes_expression()) {
        result = e;
      }
    } else if (is_true(e)) {
      result = visit_true(e);
      if (result == pbes_expression()) {
        result = e;
      }
    } else if (is_false(e)) {
      result = visit_false(e);
      if (result == pbes_expression()) {
        result = e;
      }
    } else if (is_not(e)) {
      const pbes_expression& arg = not_arg(e);
      result = visit_not(e, arg);
      if (result == pbes_expression()) {
        result = not_(visit(arg));
      }
    } else if (is_and(e)) {
      const pbes_expression& left  = lhs(e);
      const pbes_expression& right = rhs(e);
      result = visit_and(e, left, right);
      if (result == pbes_expression()) {
        result = and_(visit(left), visit(right));
      }
    } else if (is_or(e)) {
      const pbes_expression& left  = lhs(e);
      const pbes_expression& right = rhs(e);
      result = visit_or(e, left, right);
      if (result == pbes_expression()) {
        result = or_(visit(left), visit(right));
      }
    } else if (is_imp(e)) {
      const pbes_expression& left  = lhs(e);
      const pbes_expression& right = rhs(e);
      result = visit_imp(e, left, right);
      if (result == pbes_expression()) {
        result = imp(visit(left), visit(right));
      }
    } else if (is_forall(e)) {
      const data::data_variable_list& qvars = quant_vars(e);
      const pbes_expression& qexpr = quant_expr(e);
      result = visit_forall(e, qvars, qexpr);
      if (result == pbes_expression()) {
        result = forall(qvars, visit(qexpr));
      }
    } else if (is_exists(e)) {
      const data::data_variable_list& qvars = quant_vars(e);
      const pbes_expression& qexpr = quant_expr(e);
      result = visit_exists(e, qvars, qexpr);
      if (result == pbes_expression()) {
        result = exists(qvars, visit(qexpr));
      }
    }
    else if(is_propositional_variable_instantiation(e)) {
      result = visit_propositional_variable(e, propositional_variable_instantiation(e));
      if (result == pbes_expression()) {
        result = e;
      }
    }
    else {
      result = visit_unknown(e);
      if (result == pbes_expression()) {
        result = e;
      }
    }
    return result;
  }
};

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_PBES_EXPRESSION_BUILDER_H
