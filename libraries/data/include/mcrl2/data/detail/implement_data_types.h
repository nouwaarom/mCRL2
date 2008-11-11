// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/detail/implement_data_types.h
/// \brief Utility to convert a data specification into a specification in the
/// old internal data format after data implementation, to accommodate legacy
/// code that relies on this.

#ifndef MCRL2_DATA_DETAIL_IMPLEMENT_DATA_TYPES_H
#define MCRL2_DATA_DETAIL_IMPLEMENT_DATA_TYPES_H

#include "mcrl2/atermpp/aterm_access.h"
#include "mcrl2/data/detail/data_implementation.h"
#include "mcrl2/data/detail/data_specification_compatibility.h"

namespace mcrl2 {

  namespace data {

    namespace detail {

      inline atermpp::aterm_appl implement_data_specification(const data_specification& spec)
      {
        return implement_data_data_spec(data_specification_to_aterm_data_spec(spec));
      }

    } // namespace detail
    
  } // namespace data

} // namespace mcrl2

#endif //MCRL2_DATA_DETAIL_IMPLEMENT_DATA_TYPES_H

