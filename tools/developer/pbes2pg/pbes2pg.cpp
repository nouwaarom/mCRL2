// Author(s): Elbert van de Put
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file pbespgsolve.cpp

//#define MCRL2_ENUMERATE_QUANTIFIERS_BUILDER_DEBUG
//#define MCRL2_PBES_EXPRESSION_BUILDER_DEBUG

#include "mcrl2/bes/pbes_input_tool.h"
#include "mcrl2/bes/boolean_equation_system.h"
#include "mcrl2/bes/bes2pbes.h"
#include "mcrl2/bes/pg_parse.h"
#include "mcrl2/bes/io.h"
#include "mcrl2/data/rewriter_tool.h"
#include "mcrl2/pbes/detail/bes_equation_limit.h"
#include "mcrl2/pg/pbespgsolve.h"
#include "mcrl2/pg/pg_output_tool.h"
#include "mcrl2/utilities/execution_timer.h"
#include "mcrl2/utilities/input_output_tool.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <memory>
#include <cstdio>

using namespace mcrl2;
using namespace mcrl2::pbes_system;
using namespace mcrl2::core;
using namespace mcrl2::utilities;
using namespace mcrl2::log;
using bes::tools::pbes_input_tool;
using pg::tools::pg_output_tool;
using data::tools::rewriter_tool;
using utilities::tools::input_output_tool;

// class pg_solver_tool: public pbes_rewriter_tool<rewriter_tool<input_tool> >
// TODO: extend the tool with rewriter options
class pg_converter_tool : public rewriter_tool<pbes_input_tool<pg_output_tool<input_output_tool>>>
{
  protected:
    typedef rewriter_tool<pbes_input_tool<pg_output_tool<input_output_tool>>> super;

    pbespgsolve_options m_options;

    void add_options(interface_description& desc)
    {
      super::add_options(desc);
    }

    void parse_options(const command_line_parser& parser)
    {
      super::parse_options(parser);

      input_output_tool::parse_options(parser);
    }

  public:

    pg_converter_tool()
      : super(
        "pbes2pg",
        "Elbert van de Put",
        "Pbes to Parity Game converter",
        "Reads a file containing a (P)BES. "
        "A PBES input is first instantiated to a BES; from which a parity game "
        "can be obtained."
        "When INFILE is not present, standard input is used."
      )
    {
    }

    bool run() {
      pbes p;
      mcrl2::bes::load_pbes(p, input_filename(), pbes_input_format());

      mCRL2log(log::verbose) << "Generating parity game..."  << std::endl;
      // Generate the game from a PBES:
      verti goal_v;
      ParityGame pg;

      pg.assign_pbes(p, &goal_v, StaticGraph::EDGE_BIDIRECTIONAL, data::pp(m_options.rewrite_strategy)); // N.B. mCRL2 could raise an exception here
      mCRL2log(log::verbose) << "Game: " << pg.graph().V() << " vertices, " << pg.graph().E() << " edges." << std::endl;

      std::ofstream filestream(output_filename(),std::ios_base::out);
      pg.write_pgsolver(filestream);

      return true;
    }
};

int main(int argc, char* argv[])
{
  return pg_converter_tool().execute(argc, argv);
}
