// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file rewriter_test.cpp
/// \brief Add your file description here.

#include <iostream>
#include <string>
#include <set>
#include <boost/test/minimal.hpp>
#include "mcrl2/atermpp/atermpp.h"
#include "mcrl2/atermpp/make_list.h"
#include "mcrl2/atermpp/map.h"
#include "mcrl2/core/text_utility.h"
#include "mcrl2/new_data/nat.h"
#include "mcrl2/new_data/int.h"
#include "mcrl2/new_data/list.h"
#include "mcrl2/new_data/structured_sort.h"
#include "mcrl2/new_data/find.h"
#include "mcrl2/new_data/parser.h"
#include "mcrl2/new_data/replace.h"
#include "mcrl2/new_data/rewriter.h"
#include "mcrl2/new_data/function_sort.h"
#include "mcrl2/new_data/detail/data_functional.h"
#include "mcrl2/new_data/detail/implement_data_types.h"
#include "mcrl2/new_data/detail/data_specification_compatibility.h"

using namespace atermpp;
using namespace mcrl2;
using namespace mcrl2::core;
using namespace mcrl2::new_data;
using namespace mcrl2::new_data::detail;

template <typename Rewriter>
void data_rewrite_test(Rewriter& R, data_expression const& input, data_expression const& expected_output) {
  data_expression output = R(input);

  BOOST_CHECK(output == expected_output);

  if (output != expected_output) {
    std::clog << "--- test failed --- " << core::pp(input) << " ->* " << core::pp(expected_output) << std::endl
              << "input    " << core::pp(input) << std::endl
              << "expected " << core::pp(expected_output) << std::endl
              << "R(input) " << core::pp(output) << std::endl;
  }
}

void bool_rewrite_test() {
  using namespace mcrl2::new_data::sort_bool_;

  data_specification specification = parse_data_specification(
    ""
  );

  new_data::rewriter R(specification);

  data_rewrite_test(R, true_(), true_());
  data_rewrite_test(R, false_(), false_());

  data_rewrite_test(R, and_(true_(), false_()), false_());
  data_rewrite_test(R, and_(false_(), true_()), false_());

  data_rewrite_test(R, or_(true_(), false_()), true_());
  data_rewrite_test(R, or_(false_(), true_()), true_());

  data_rewrite_test(R, implies(true_(), false_()), false_());
  data_rewrite_test(R, implies(false_(), true_()), true_());
}

void pos_rewrite_test() {
  using namespace mcrl2::new_data::sort_pos;

  data_specification specification = parse_data_specification(
    "sort A = Pos;"
  );

  new_data::rewriter R(specification);

  data_expression p1(parse_data_expression("1"));
  data_expression p2(parse_data_expression("2"));
  data_expression p3(parse_data_expression("3"));
  data_expression p4(parse_data_expression("4"));

  data_rewrite_test(R, sort_pos::plus(p1, p2), p3);
  data_rewrite_test(R, sort_pos::plus(p2, p1), p3);

  data_rewrite_test(R, sort_pos::times(p1, p1), p1);
  data_rewrite_test(R, sort_pos::times(p1, p2), p2);

  data_rewrite_test(R, (sort_pos::min)(p1, p1), p1);
  data_rewrite_test(R, (sort_pos::min)(p1, p2), p1);

  data_rewrite_test(R, (sort_pos::max)(p1, p1), p1);
  data_rewrite_test(R, (sort_pos::max)(p1, p2), p2);

  data_rewrite_test(R, sort_pos::succ(p1), p2);

  data_rewrite_test(R, sort_pos::abs(p4), p4);
}

void nat_rewrite_test() {
  using namespace mcrl2::new_data::sort_nat;

  data_specification specification = parse_data_specification(
    "sort A = Nat;"
  );

  new_data::rewriter R(specification);

  data_expression p0(parse_data_expression("0"));
  data_expression p1(pos2nat(parse_data_expression("1")));
  data_expression p2(pos2nat(parse_data_expression("2")));
  data_expression p3(pos2nat(parse_data_expression("3")));
  data_expression p4(pos2nat(parse_data_expression("4")));

  data_rewrite_test(R, plus(p0, p2), p2);
  data_rewrite_test(R, plus(p2, p0), p2);
  data_rewrite_test(R, plus(p1, p2), p3);
  data_rewrite_test(R, plus(p2, p1), p3);

  data_rewrite_test(R, times(p1, p1), p1);
  data_rewrite_test(R, times(p0, p2), p0);
  data_rewrite_test(R, times(p2, p0), p0);
  data_rewrite_test(R, times(p1, p2), p2);

  data_rewrite_test(R, (min)(p1, p1), p1);
  data_rewrite_test(R, (min)(p0, p2), p0);
  data_rewrite_test(R, (min)(p2, p0), p0);
  data_rewrite_test(R, (min)(p1, p2), p1);

  data_rewrite_test(R, (max)(p1, p1), p1);
  data_rewrite_test(R, (max)(p0, p2), p2);
  data_rewrite_test(R, (max)(p2, p0), p2);
  data_rewrite_test(R, (max)(p1, p2), p2);

  data_rewrite_test(R, succ(p0), p1);
  data_rewrite_test(R, succ(p1), p2);

  data_rewrite_test(R, pred(p1), p0);
  data_rewrite_test(R, pred(p2), p1);

  data_rewrite_test(R, abs(p1), p1);

  data_rewrite_test(R, div(p1, p1), p1);
  data_rewrite_test(R, div(p0, p2), p0);
  data_rewrite_test(R, div(p2, p1), p2);
  data_rewrite_test(R, div(p4, p2), p2);

  data_rewrite_test(R, mod(p1, p1), p0);
  data_rewrite_test(R, mod(p0, p2), p0);
  data_rewrite_test(R, mod(p2, p1), p0);
  data_rewrite_test(R, mod(p4, p3), p1);

  data_rewrite_test(R, exp(p2, p2), p4);
}

void int_rewrite_test() {
  using namespace mcrl2::new_data::sort_int_;

  data_specification specification = parse_data_specification(
    "sort A = Int;"
  );

  new_data::rewriter R(specification);

  data_expression p0(nat2int(parse_data_expression("0")));
  data_expression p1(pos2int(parse_data_expression("1")));
  data_expression p2(pos2int(parse_data_expression("2")));
  data_expression p3(pos2int(parse_data_expression("3")));
  data_expression p4(pos2int(parse_data_expression("4")));

  data_rewrite_test(R, plus(p0, p2), p2);
  data_rewrite_test(R, plus(p2, p0), p2);
  data_rewrite_test(R, plus(p1, p2), p3);
  data_rewrite_test(R, plus(p2, p1), p3);
  data_rewrite_test(R, plus(negate(p4), p4), p0);
  data_rewrite_test(R, minus(p4, p4), p0);

  data_rewrite_test(R, times(p1, p1), p1);
  data_rewrite_test(R, times(p0, p2), p0);
  data_rewrite_test(R, times(p2, p0), p0);
  data_rewrite_test(R, times(p1, p2), p2);

  data_rewrite_test(R, (min)(p1, p1), p1);
  data_rewrite_test(R, (min)(p0, p2), p0);
  data_rewrite_test(R, (min)(p2, p0), p0);
  data_rewrite_test(R, (min)(p1, p2), p1);

  data_rewrite_test(R, (max)(p1, p1), p1);
  data_rewrite_test(R, (max)(p0, p2), p2);
  data_rewrite_test(R, (max)(p2, p0), p2);
  data_rewrite_test(R, (max)(p1, p2), p2);

  data_rewrite_test(R, succ(p0), p1);
  data_rewrite_test(R, succ(p1), p2);

  data_rewrite_test(R, pred(p1), p0);
  data_rewrite_test(R, pred(p2), p1);

  data_rewrite_test(R, abs(p1), p1);

  data_rewrite_test(R, div(p1, p1), p1);
  data_rewrite_test(R, div(p0, p2), p0);
  data_rewrite_test(R, div(p2, p1), p2);
  data_rewrite_test(R, div(p4, p2), p2);

  data_rewrite_test(R, mod(p1, p1), p0);
  data_rewrite_test(R, mod(p0, p2), p0);
  data_rewrite_test(R, mod(p2, p1), p0);
  data_rewrite_test(R, mod(p4, p3), p1);

  data_rewrite_test(R, exp(p2, p2), p4);
}

void list_rewrite_test() {
  using namespace mcrl2::new_data::sort_bool_;
  using namespace mcrl2::new_data::sort_list;

  data_specification specification = parse_data_specification(
    "sort A = List(Bool);"
  );

  new_data::rewriter R(specification);

  sort_expression list_bool(list(bool_()));
  data_expression empty(nil(list_bool));
  data_expression head_true(cons_(list_bool, true_(), empty));

  data_rewrite_test(R, in(list_bool, true_(), head_true), true_());
  data_rewrite_test(R, in(list_bool, false_(), head_true), false_());
  data_rewrite_test(R, count(true_(), head_true), parse_data_expression("1"));
  data_rewrite_test(R, in(list_bool, false_(), snoc(list_bool, true_(), head_true)), false_());
  data_rewrite_test(R, concat(list_bool, head_true, head_true), cons_(list_bool, true_(), head_true));
  data_rewrite_test(R, element_at(bool_(), head_true, parse_data_expression("1")), true_());
  data_rewrite_test(R, head(list_bool, head_true), true_());
  data_rewrite_test(R, rhead(list_bool, head_true), true_());
  data_rewrite_test(R, rtail(list_bool, head_true), empty);
  data_rewrite_test(R, tail(list_bool, head_true), empty);
}

int test_main(int argc, char** argv)
{
  MCRL2_ATERMPP_INIT(argc, argv)

//  std::string expr1("struct ");
//  std::string expr1("exists b: Bool, c: Bool. if(b, c, b)");
//  std::string expr1("forall b: Bool, c: Bool. if(b, c, b)");

  list_rewrite_test();
  int_rewrite_test();
  bool_rewrite_test();
  pos_rewrite_test();
  nat_rewrite_test();

  return 0;
}
