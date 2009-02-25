// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/new_data/alias.h
/// \brief The class alias.

#ifndef MCRL2_NEW_DATA_ALIAS_H
#define MCRL2_NEW_DATA_ALIAS_H

#include "boost/range/iterator_range.hpp"

#include "mcrl2/atermpp/aterm_appl.h"
#include "mcrl2/atermpp/aterm_access.h"
#include "mcrl2/new_data/sort_expression.h"
#include "mcrl2/new_data/basic_sort.h"

namespace mcrl2 {

  namespace new_data {

    /// \brief alias.
    ///
    /// An alias introduces another name for a sort.
    class alias: public sort_expression
    {
      public:

        /// \brief Constructor
        ///
        alias()
          : sort_expression(core::detail::constructSortRef())
        {}

        /// \brief Constructor
        ///
        alias(const sort_expression s)
          : sort_expression(s)
        {
          assert(s.is_alias());
        }

        /// \brief Constructor
        ///
        /// \param[in] b The name of the alias that is created.
        /// \param[in] s The sort for which an alias is created.
        /// \post n and s describe the same sort.
        alias(const basic_sort& b, const sort_expression s)
          : sort_expression(mcrl2::core::detail::gsMakeSortRef(atermpp::arg1(static_cast< ATermAppl >(b)), s))
        {}

        /// \brief Returns the name of this sort.
        ///
        inline
        basic_sort name() const
        {
          return basic_sort(std::string(atermpp::aterm_string(atermpp::arg1(*this))));
        }

        /// \brief Returns the sort to which the name refers.
        ///
        inline
        sort_expression reference() const
        {
          return atermpp::arg2(*this);
        }

    }; // class alias

    /// \brief list of aliases
    typedef atermpp::vector< alias >                              alias_list;
    /// \brief iterator range over list of aliases
    typedef boost::iterator_range< alias_list::iterator >         alias_range;
    /// \brief iterator range over constant list of aliases
    typedef boost::iterator_range< alias_list::const_iterator >   alias_const_range;

  } // namespace new_data

} // namespace mcrl2

#endif // MCRL2_NEW_DATA_SORT_EXPRESSION_H

