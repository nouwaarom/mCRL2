// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lps/substitute.h
/// \brief add your file description here.

#ifndef MCRL2_LPS_SUBSTITUTE_H
#define MCRL2_LPS_SUBSTITUTE_H

#include "mcrl2/data/substitute.h"
#include "mcrl2/lps/add_binding.h"
#include "mcrl2/lps/builder.h"
#include "mcrl2/lps/detail/lps_substituter.h"

namespace mcrl2 {

namespace lps {

  /// \brief Applies a substitution to an LPS data type.
  template <typename Object, typename Substitution>
  void substitute(Object& o, const Substitution& sigma, bool replace_parameters)
  {
    lps::detail::lps_substituter<Substitution> r(sigma, replace_parameters);
    r(o);
  }

//--- start generated lps replace code ---//
template <typename T, typename Substitution>
  void substitute_variables(T& x,
                            Substitution sigma,
                            typename boost::disable_if<typename boost::is_base_of<atermpp::aterm_base, T>::type>::type* = 0
                           )
  {
    core::make_update_apply_builder<lps::data_expression_builder>(sigma)(x);
  }

  template <typename T, typename Substitution>
  T substitute_variables(const T& x,
                         Substitution sigma,
                         typename boost::enable_if<typename boost::is_base_of<atermpp::aterm_base, T>::type>::type* = 0
                        )
  {   
    return core::make_update_apply_builder<lps::data_expression_builder>(sigma)(x);
  }

  template <typename T, typename Substitution>
  void substitute_free_variables(T& x,
                                 Substitution sigma,
                                 typename boost::disable_if<typename boost::is_base_of<atermpp::aterm_base, T>::type>::type* = 0
                                )
  {
    data::detail::make_substitute_free_variables_builder<lps::data_expression_builder, lps::add_data_variable_binding>(sigma)(x);
  }

  template <typename T, typename Substitution>
  T substitute_free_variables(const T& x,
                              Substitution sigma,
                              typename boost::enable_if<typename boost::is_base_of<atermpp::aterm_base, T>::type>::type* = 0
                             )
  {
    return data::detail::make_substitute_free_variables_builder<lps::data_expression_builder, lps::add_data_variable_binding>(sigma)(x);
  }

  template <typename T, typename Substitution, typename VariableContainer>
  void substitute_free_variables(T& x,
                                 Substitution sigma,
                                 const VariableContainer& bound_variables,
                                 typename boost::disable_if<typename boost::is_base_of<atermpp::aterm_base, T>::type>::type* = 0
                                )
  {
    data::detail::make_substitute_free_variables_builder<lps::data_expression_builder, lps::add_data_variable_binding>(sigma)(x, bound_variables);
  }

  template <typename T, typename Substitution, typename VariableContainer>
  T substitute_free_variables(const T& x,
                              Substitution sigma,
                              const VariableContainer& bound_variables,
                              typename boost::enable_if<typename boost::is_base_of<atermpp::aterm_base, T>::type>::type* = 0
                             )
  {
    return data::detail::make_substitute_free_variables_builder<lps::data_expression_builder, lps::add_data_variable_binding>(sigma)(x, bound_variables);
  }
//--- end generated lps replace code ---//

} // namespace lps

} // namespace mcrl2

#endif // MCRL2_LPS_SUBSTITUTE_H
