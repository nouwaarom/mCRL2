// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/partial_order_reduction.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_PARTIAL_ORDER_REDUCTION_H
#define MCRL2_PBES_PARTIAL_ORDER_REDUCTION_H

#include <chrono>
#include <iomanip>
#include <boost/dynamic_bitset.hpp>
#include "mcrl2/data/enumerator.h"
#include "mcrl2/data/rewriters/one_point_rule_rewriter.h"
#include "mcrl2/data/rewriters/quantifiers_inside_rewriter.h"
#include "mcrl2/data/substitution_utility.h"
#include "mcrl2/data/substitutions/mutable_map_substitution.h"
#include "mcrl2/data/substitutions/maintain_variables_in_rhs.h"
#include "mcrl2/pbes/pbes_equation_index.h"
#include "mcrl2/pbes/replace_capture_avoiding_with_an_identifier_generator.h"
#include "mcrl2/pbes/rewriters/enumerate_quantifiers_rewriter.h"
#include "mcrl2/pbes/srf_pbes.h"
#include "mcrl2/pbes/unify_parameters.h"
#include "mcrl2/smt/solver.h"
#include "mcrl2/utilities/detail/container_utility.h"
#include "mcrl2/utilities/skip.h"

namespace mcrl2 {

namespace pbes_system {

namespace detail {

inline
data::data_expression make_and(const data::data_expression& x1, const data::data_expression& x2, const data::data_expression& x3)
{
  return data::and_(x1, data::and_(x2, x3));
}

inline
data::data_expression equal_to(const data::data_expression_list& x, const data::data_expression_list& y)
{
  data::data_expression result = data::sort_bool::true_();
  auto xi = x.begin();
  auto yi = y.begin();
  for (; xi != x.end(); ++xi, ++yi)
  {
    result = data::lazy::and_(result, data::equal_to(*xi, *yi));
  }
  return result;
}

} // namespace detail

typedef boost::dynamic_bitset<> summand_set;

inline
std::string print_summand_set(const summand_set& s)
{
  bool first = true;
  std::ostringstream buf;
  buf << "{ ";
  for (std::size_t k = s.find_first(); k != summand_set::npos; k = s.find_next(k))
  {
    buf << (first ? "" : ", ") << k;
    first = false;
  }
  buf << " }";
  return buf.str();
}

struct summand_class
{
  data::variable_list e;
  data::data_expression f;
  data::data_expression_list g;
  /// \brief Encodes the dependency relation belonging to this summand_class
  /// \detail nxt[i] contains j iff X_i --this--> X_j
  std::vector<std::set<std::size_t>> nxt;
  summand_set NES;
  summand_set DNA;
  summand_set DNS;
  summand_set DNL;
  bool is_deterministic = false;

  summand_class() = default;

  // n is the number of PBES equations
  summand_class(data::variable_list  e_, data::data_expression f_, data::data_expression_list g_, std::size_t n)
   : e(std::move(e_)), f(std::move(f_)), g(std::move(g_))
  {
    nxt.resize(n);
  }

  void set_num_summands(const std::size_t N)
  {
    NES.resize(N);
    DNA.resize(N);
    DNS.resize(N);
    DNL.resize(N);
  }

  // returns X_i -k->
  bool depends(std::size_t i) const
  {
    return !nxt[i].empty();
  }

  // returns X_i -k-> j
  bool depends(std::size_t i, std::size_t j) const
  {
    using utilities::detail::contains;

    return contains(nxt[i], j);
  }

  void print(std::ostream& out, const std::set<std::size_t>& s, const std::size_t N) const
  {
    using utilities::detail::contains;

    for (std::size_t i = 0; i < N; i++)
    {
      out << (contains(s, i) ? '1' : '0');
    }
  }

  // N is the number of summand classes
  void print(std::ostream& out, const std::size_t N) const
  {
    out << "deterministic = " << std::boolalpha << is_deterministic << std::endl;
    out << "NES = " << print_summand_set(NES) << std::endl;
    out << "DNA = " << print_summand_set(DNA) << std::endl;
    out << "DNS = " << print_summand_set(DNS) << std::endl;
    out << "DNL = " << print_summand_set(DNL) << std::endl;
  }
};

// The part of a summand used for determining equivalence classes
struct summand_equivalence_key
{
  data::variable_list e;
  data::data_expression f;
  data::data_expression_list g;

  explicit summand_equivalence_key(data::variable_list e_, data::data_expression f_, data::data_expression_list g_)
  : e(std::move(e_)), f(std::move(f_)), g(std::move(g_))
  {}

  explicit summand_equivalence_key(const summand_class& summand)
   : e(summand.e), f(summand.f), g(summand.g)
  {}

  explicit summand_equivalence_key(const srf_summand& summand)
   : e(summand.parameters()), f(summand.condition()), g(summand.variable().parameters())
  {}

  bool operator<(const summand_equivalence_key& other) const
  {
    return std::tie(e, f, g) < std::tie(other.e, other.f, other.g);
  }

  bool operator==(const summand_equivalence_key& other) const
  {
    return std::tie(e, f, g) == std::tie(other.e, other.f, other.g);
  }
};

} // namespace pbes_system

} // namespace mcrl2

namespace std {

/// \brief specialization of the standard std::hash function.
template<>
struct hash<mcrl2::pbes_system::summand_equivalence_key>
{
  std::size_t operator()(const mcrl2::pbes_system::summand_equivalence_key& x) const
  {
    std::size_t seed = std::hash<atermpp::aterm>()(x.f);
    if (!x.e.empty())
    {
      seed = std::hash<atermpp::aterm>()(x.e) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    if (!x.g.empty())
    {
      seed = std::hash<atermpp::aterm>()(x.g) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

} // namespace std

namespace mcrl2 {

namespace pbes_system {

enum tribool
{
  no, maybe, yes
};
static inline bool operator&&(tribool a, tribool b)
{
  return a == yes || b == yes || (a == maybe && b == maybe);
}
// Short-circuit version of the operator && for tribools
// The second function will be told whether a 'yes' answer is required to satisfy
// the expression
static inline bool operator&&(std::function<tribool()> a, std::function<tribool(bool)> b)
{
  tribool a_ = a();
  if(a_ == yes)
  {
    return true;
  }
  return a_ && b(a_ == no);
}

class partial_order_reduction_algorithm
{
  protected:
    using enumerator_element = data::enumerator_list_element_with_substitution<pbes_expression>;

    struct invis_pair
    {
      summand_set Twork;
      summand_set Ts;
      invis_pair(summand_set Twork_, summand_set Ts_)
       : Twork(std::move(Twork_)), Ts(std::move(Ts_))
      {}

      bool operator<(const invis_pair& other) const
      {
        return std::tie(Twork, Ts) < std::tie(other.Twork, other.Ts);
      }
    };

    struct parameter_info
    {
      std::set<std::size_t> Ts; // test set
      std::set<std::size_t> Ws; // write set
      std::set<std::size_t> Rs; // read set
      std::set<std::size_t> Vs; // variable set
    };

    data::rewriter m_rewr;
    enumerate_quantifiers_rewriter m_pbes_rewr;
    data::enumerator_identifier_generator m_id_generator;
    data::enumerator_algorithm<enumerate_quantifiers_rewriter, data::rewriter> m_enumerator;
    srf_pbes m_pbes;
    pbes_equation_index m_equation_index;
    data::mutable_indexed_substitution<> m_sigma;
    std::size_t m_largest_equation_size = 0;

    // the parameters of the PBES equations
    std::vector<data::variable> m_parameters;

    // maps parameters to their corresponding index
    std::map<data::variable, std::size_t> m_parameter_positions;

    // maps summands to the index of the corresponding summand class
    std::unordered_map<summand_equivalence_key, std::size_t> m_summand_index;

    // X_j \in nxt_k(X_i) <=> j \in m_summand_classes[k].nxt[i]
    // (X_i |- k -> X_j) <=> m_summand_classes[k].NES[i][j]
    std::vector<summand_class> m_summand_classes;

    summand_set m_invis; // invisible summand classes
    summand_set m_vis;   // visible summand classes

    // One NES for every predicate variable X_i that can be used for summand
    // class k when !depends(i,k).
    std::vector<summand_set> m_dependency_nes;

    std::chrono::high_resolution_clock::duration m_static_analysis_duration;
    std::chrono::high_resolution_clock::duration m_exploration_duration;

    smt::smt_solver* m_solver;
    std::chrono::milliseconds m_smt_timeout;

    // if true use alternative A3 for maybe clauses in accordance conditions
    bool m_use_weak_conditions;
    bool m_no_determinisim;
    bool m_no_triangle;

    class summand_relations_data
    {
    private:
      partial_order_reduction_algorithm& parent;
      bool use_weak_conditions;
      data::set_identifier_generator id_gen;

      data::variable_list qvars1_k;
      data::data_expression condition1_k;
      data::data_expression_list updates1_k;

      data::variable_list qvars1_k1;
      data::data_expression condition1_k1;
      data::data_expression_list updates1_k1;

      data::variable_list qvars2_k;
      data::data_expression condition2_k;
      data::data_expression_list updates2_k;

      data::variable_list qvars2_k1;
      data::data_expression condition2_k1;
      data::data_expression_list updates2_k1;

      data::mutable_indexed_substitution<> sigma_k;
      data::mutable_indexed_substitution<> sigma_k1;

      data::variable_list combined_quantified_vars;

      // Depending on whether the weak (A3) or strong condition (A4) is used, wrap the consequent in
      // an existential quantifier
      data::data_expression make_exists_if_strong(const data::variable_list& vars, const data::data_expression& body)
      {
        return use_weak_conditions ? body : make_exists(vars, body);
      }

      data::data_expression left_accords_antecedent()
      {
        return data::sort_bool::and_(condition1_k1, data::replace_variables_capture_avoiding(condition1_k, sigma_k1, id_gen));
      }

      data::data_expression left_accords_consequent()
      {
        data::data_expression parameters_equal = detail::equal_to(data::replace_variables_capture_avoiding(updates2_k, sigma_k1, id_gen),
                                                                  data::replace_variables_capture_avoiding(updates2_k1, sigma_k, id_gen));
        data::data_expression body = detail::make_and(
          condition2_k,
          data::replace_variables_capture_avoiding(condition2_k1, sigma_k, id_gen),
          parameters_equal
        );
        return make_exists_if_strong(qvars2_k + qvars2_k1, body);
      }

      data::data_expression coenabled_antecedent()
      {
        return data::sort_bool::and_(condition1_k, condition1_k1);
      }

      data::data_expression square_accords_consequent()
      {
        data::data_expression parameters_equal = detail::equal_to(data::replace_variables_capture_avoiding(updates2_k, sigma_k1, id_gen),
                                                                  data::replace_variables_capture_avoiding(updates2_k1, sigma_k, id_gen));
        data::data_expression body = detail::make_and(
          data::replace_variables_capture_avoiding(condition2_k, sigma_k1, id_gen),
          data::replace_variables_capture_avoiding(condition2_k1, sigma_k, id_gen),
          parameters_equal
        );
        return make_exists_if_strong(qvars2_k + qvars2_k1, body);
      }

      data::data_expression triangle_accords_consequent_weak()
      {
        data::data_expression parameters_equal = detail::equal_to(updates1_k1, data::replace_variables_capture_avoiding(updates1_k1, sigma_k, id_gen));
        return data::sort_bool::and_(
          data::replace_variables_capture_avoiding(condition1_k1, sigma_k, id_gen),
          parameters_equal
        );
      }

      data::data_expression triangle_accords_consequent()
      {
        data::data_expression parameters_equal = detail::equal_to(updates2_k1, data::replace_variables_capture_avoiding(updates2_k1, sigma_k, id_gen));
        data::data_expression body = data::sort_bool::and_(
          data::replace_variables_capture_avoiding(condition2_k1, sigma_k, id_gen),
          parameters_equal
        );
        return make_exists_if_strong(qvars2_k1, body);
      }

      tribool accords_data(bool affect_set, bool needs_yes,
                           std::function<data::data_expression()> make_antecedent,
                           std::function<data::data_expression()> make_consequent)
      {
        // Check whether the maybe clause is satisfied by affect sets and it is sufficient to return maybe
        if (affect_set && !needs_yes)
        {
          return maybe;
        }

        data::data_expression antecedent = make_antecedent();
        data::data_expression yes_condition = make_forall(combined_quantified_vars, data::sort_bool::not_(antecedent));
        if (parent.is_true(yes_condition))
        {
          return yes;
        }
        if (needs_yes)
        {
          // we were not able to return yes, now it doesn't matter what we return
          return no;
        }

        data::data_expression consequent = make_consequent();
        data::data_expression condition = make_forall(combined_quantified_vars, data::sort_bool::implies(antecedent, consequent));

        return parent.is_true(condition) ? maybe : no;
      }

    public:
      summand_relations_data(partial_order_reduction_algorithm& p, const std::size_t k, const std::size_t k1)
      : parent(p)
      , use_weak_conditions(p.m_use_weak_conditions)
      {
        const summand_class& summand_k = parent.m_summand_classes[k];
        const summand_class& summand_k1 = parent.m_summand_classes[k1];

        const data::variable_list& parameters = parent.m_pbes.equations()[0].variable().parameters();
        for(const data::variable& v: parameters)
        {
          id_gen.add_identifier(v.name());
        }

        // For both summands, create a copy with fresh variables
        // These will be used when constructing accordance and NES conditions
        summand_equivalence_key new1_k = rename_duplicate_variables(id_gen, summand_equivalence_key(summand_k));
        summand_equivalence_key new1_k1 = rename_duplicate_variables(id_gen, summand_equivalence_key(summand_k1));
        qvars1_k  = new1_k.e;  condition1_k  = new1_k.f;  updates1_k  = new1_k.g;
        qvars1_k1 = new1_k1.e; condition1_k1 = new1_k1.f; updates1_k1 = new1_k1.g;
        data::add_assignments(sigma_k, parameters, updates1_k);
        data::add_assignments(sigma_k1, parameters, updates1_k1);

        if (!use_weak_conditions)
        {
          // When using the stronger condition A4, create another fresh copy
          summand_equivalence_key new2_k = rename_duplicate_variables(id_gen, summand_equivalence_key(summand_k));
          summand_equivalence_key new2_k1 = rename_duplicate_variables(id_gen, summand_equivalence_key(summand_k1));
          qvars2_k  = new2_k.e;  condition2_k  = new2_k.f;  updates2_k  = new2_k.g;
          qvars2_k1 = new2_k1.e; condition2_k1 = new2_k1.f; updates2_k1 = new2_k1.g;
        }
        else
        {
          // In the weak case (A3), the two copies are identical
          qvars2_k  = qvars1_k;  condition2_k  = condition1_k;  updates2_k  = updates1_k;
          qvars2_k1 = qvars1_k1; condition2_k1 = condition1_k1; updates2_k1 = updates1_k1;
        }

        combined_quantified_vars = parameters + qvars1_k + qvars1_k1;
      }

      bool can_enable()
      {
        data::data_expression cannot_enable = make_forall(
          combined_quantified_vars,
          data::sort_bool::not_(
            detail::make_and(
              data::sort_bool::not_(condition1_k),
              condition1_k1,
              data::replace_variables_capture_avoiding(condition1_k, sigma_k1, id_gen)
            )
          )
        );

        // The condition is constructed in a negated way, so the approximation of the decision
        // procedure works the right way. Note that the result of this function is negated as well.
        return !parent.is_true(cannot_enable);
      }

      tribool left_accords_data(bool affect_set, bool needs_yes)
      {
        return accords_data(affect_set, needs_yes,
                            std::bind(&summand_relations_data::left_accords_antecedent, *this),
                            std::bind(&summand_relations_data::left_accords_consequent, *this));
      }

      tribool square_accords_data(bool affect_set, bool needs_yes)
      {
        return accords_data(affect_set, needs_yes,
                            std::bind(&summand_relations_data::coenabled_antecedent, *this),
                            std::bind(&summand_relations_data::square_accords_consequent, *this));
      }

      tribool triangle_accords_data(bool affect_set, bool needs_yes)
      {
        return accords_data(affect_set, needs_yes,
                            std::bind(&summand_relations_data::coenabled_antecedent, *this),
                            std::bind(&summand_relations_data::triangle_accords_consequent, *this));
      }
    };

    std::size_t summand_index(const srf_summand& summand) const
    {
      auto i = m_summand_index.find(summand_equivalence_key(summand));
      assert(i != m_summand_index.end());
      return i->second;
    }

    std::size_t parameter_position(const data::variable& v) const
    {
      auto i = m_parameter_positions.find(v);
      return i->second;
    }

    const summand_set& DNA(std::size_t k) const
    {
      return m_summand_classes[k].DNA;
    }

    summand_set& DNA(std::size_t k)
    {
      return m_summand_classes[k].DNA;
    }

    const summand_set& DNS(std::size_t k) const
    {
      return m_summand_classes[k].DNS;
    }

    summand_set& DNS(std::size_t k)
    {
      return m_summand_classes[k].DNS;
    }

    const summand_set& DNL(std::size_t k) const
    {
      return m_summand_classes[k].DNL;
    }

    summand_set& DNL(std::size_t k)
    {
      return m_summand_classes[k].DNL;
    }

    const summand_set& NES(std::size_t k) const
    {
      return m_summand_classes[k].NES;
    }

    summand_set& NES(std::size_t k)
    {
      return m_summand_classes[k].NES;
    }

    summand_set en(const propositional_variable_instantiation& X_e)
    {
      std::size_t N = m_summand_classes.size();

      summand_set result(N);
      std::size_t i = m_equation_index.index(X_e.name());
      const data::variable_list& d = m_pbes.equations()[i].variable().parameters();
      const data::data_expression_list& e = X_e.parameters();
      add_assignments(m_sigma, d, e);
      for (std::size_t k = 0; k < N; k++)
      {
        if (!depends(i, k))
        {
          continue;
        }
        const summand_class& summand_k = m_summand_classes[k];
        const data::variable_list& e_k = summand_k.e;
        const pbes_expression& f_k = summand_k.f;
        m_enumerator.enumerate(enumerator_element(e_k, f_k),
                               m_sigma,
                               [&](const enumerator_element&) {
                                 result.set(k);
                                 return false;
                               },
                               pbes_system::is_false
        );
        remove_assignments(m_sigma, e_k);
      }
      remove_assignments(m_sigma, d);
      return result;
    }

    summand_set invis(const summand_set& K)
    {
      return m_invis & K;
    }

    // Choose a NES according to the heuristic function h.
    const summand_set& choose_minimal_NES(std::size_t k,
                                   const propositional_variable_instantiation& X_e,
                                   const summand_set& /* Twork */,
                                   const summand_set& /* Ts */,
                                   const summand_set& /* en_X_e */
                                  ) const
    {
      using utilities::detail::set_difference;
      using utilities::detail::set_intersection;
      using utilities::detail::set_union;

      if(!depends(m_equation_index.index(X_e.name()), k))
      {
        return m_dependency_nes[m_equation_index.index(X_e.name())];
      }
      else
      {
        return m_summand_classes[k].NES;
      }

      //TODO implement one NES per guard, choose the smallest one
      // summand_set Twork_Ts = set_union(Twork, Ts);
      // const summand_class& summand_k = m_summand_classes[k];
      //
      // summand_set T1 = set_union(Twork_Ts, en_X_e);
      // summand_set T2 = set_intersection(Twork_Ts, en_X_e);
      //
      // auto h = [&](std::size_t i)
      // {
      //   const summand_set NES_k = summand_k.NES[i];
      //   return set_difference(NES_k, T1).size() + m_largest_equation_size * set_difference(NES_k, T2).size();
      // };
      //
      // std::size_t i_min = 0;
      // std::size_t h_min = h(0);
      // for (std::size_t i = 1; i < n; i++)
      // {
      //   std::size_t h_i = h(i);
      //   if (h_i < h_min)
      //   {
      //     i_min = i;
      //     h_min = h_i;
      //   }
      // }
      // return i_min;
    }

    const summand_set& DNX(std::size_t k,
                                   const summand_set& Twork,
                                   const summand_set& Ts,
                                   const summand_set& en_X_e
                                  ) const
    {
      using utilities::detail::set_difference;
      using utilities::detail::set_intersection;
      using utilities::detail::set_union;

      const summand_class& summand_k = m_summand_classes[k];
      if (!summand_k.is_deterministic)
      {
        return DNL(k);
      }

      summand_set Twork_Ts = Twork | Ts;

      summand_set T1 = Twork_Ts | en_X_e;
      summand_set T2 = Twork_Ts & en_X_e;

      auto h = [&](const summand_set& A)
      {
        return (A - T1).count() + m_largest_equation_size * (A - T2).count();
      };

      return h(DNS(k)) <= h(DNL(k)) ? DNS(k) : DNL(k);
    }

    summand_set stubborn_set(const propositional_variable_instantiation& X_e)
    {
      using utilities::detail::contains;
      using utilities::detail::has_empty_intersection;
      using utilities::detail::set_difference;
      using utilities::detail::set_includes;
      using utilities::detail::set_intersection;
      using utilities::detail::set_union;

      std::size_t N = m_summand_classes.size();

      summand_set en_X_e = en(X_e);

      summand_set empty_set;
      auto size = [&](const invis_pair& p)
      {
        empty_set = en_X_e;
        empty_set &= p.Twork;
        empty_set &= p.Ts;
        return empty_set.count();
      };

      auto choose_minimum_element = [&](std::set<invis_pair>& C)
      {
        auto i = std::min_element(C.begin(), C.end(),
                                  [&](const invis_pair& x, const invis_pair& y)
                                  {
                                    return size(x) < size(y);
                                  }
        );
        invis_pair result = *i;
        C.erase(i);
        return result;
      };

      if ((en_X_e & m_invis).none())
      {
        return en_X_e;
      }

      std::set<invis_pair> C;
      auto invis_en_X_e = invis(en_X_e);
      for (std::size_t k = invis_en_X_e.find_first(); k != summand_set::npos; k = invis_en_X_e.find_next(k))
      {
        invis_pair pair_k((summand_set(N)), summand_set(N));
        pair_k.Twork.set(k);
        C.insert(pair_k);
      }
      assert(!C.empty());

      while (true)
      {
        auto p = choose_minimum_element(C);
        auto& Twork = p.Twork;
        auto& Ts = p.Ts;
        if (Twork.none())
        {
          summand_set T = Ts & en_X_e;
          for (std::size_t k = T.find_first(); k != summand_set::npos; k = T.find_next(k))
          {
            if (DNA(k).is_subset_of(Ts))
            {
              return Ts;
            }
          }
          std::size_t k = T.find_first(); // TODO: choose k according to D2t
          Twork = Twork | (DNA(k) - Ts);
        }
        else
        {
          std::size_t k = Twork.find_first();
          Twork.reset(k);
          Ts.set(k);
          if (en_X_e.test(k))
          {
            auto& DNS_or_DNL = DNX(k, Twork, Ts, en_X_e);
            Twork = Twork | (DNS_or_DNL - Ts);
            if (m_vis.test(k))
            {
              Twork = Twork | (m_vis - Ts);
            }
          }
          else
          {
            auto& NES = choose_minimal_NES(k, X_e, Twork, Ts, en_X_e);
            Twork = Twork | (NES - Ts);
          }
        }
        C.insert(invis_pair(Twork, Ts));
      }
    }

    std::set<propositional_variable_instantiation> succ(const propositional_variable_instantiation& X_e, const summand_set& K)
    {
      const auto& d = m_parameters;
      const auto& e = X_e.parameters();

      std::set<propositional_variable_instantiation> result;
      std::size_t i = m_equation_index.index(X_e.name());
      for (std::size_t k = K.find_first(); k != summand_set::npos; k = K.find_next(k))
      {
        const summand_class& summand_k = m_summand_classes[k];
        const data::variable_list& e_k = summand_k.e;
        const pbes_expression& f_k = summand_k.f;
        const data::data_expression_list& g_k = summand_k.g;
        const auto& J = summand_k.nxt[i];

        // Add assignments for parameters during every iteration,
        // because they might have been removed on the previous one if
        // a parameter coincides with a quantified variable.
        data::add_assignments(m_sigma, d, e);
        m_enumerator.enumerate(enumerator_element(e_k, f_k),
                               m_sigma,
                               [&](const enumerator_element& p) {
                                 p.add_assignments(e_k, m_sigma, m_rewr);
                                 data::data_expression_list g(g_k.begin(), g_k.end(), [&](const data::data_expression& x) { return m_rewr(x, m_sigma); });
                                 for (std::size_t j: J)
                                 {
                                   const core::identifier_string& X_j = m_pbes.equations()[j].variable().name();
                                   result.insert(propositional_variable_instantiation(X_j, g));
                                 }
                                 return false;
                               },
                               pbes_system::is_false
        );
        data::remove_assignments(m_sigma, e_k);
      }
      return result;
    }

    void compute_nxt()
    {
      std::size_t n = m_pbes.equations().size();
      for (std::size_t i = 0; i < n; i++)
      {
        const srf_equation& eqn = m_pbes.equations()[i];
        for (const srf_summand& summand: eqn.summands())
        {
          std::size_t j = m_equation_index.index(summand.variable().name());
          std::size_t k = summand_index(summand);
          m_summand_classes[k].nxt[i].insert(j);
        }
      }
    }

    // returns the indices of the parameters that occur freely in x
    std::set<std::size_t> FV(const pbes_expression& x) const
    {
      std::set<std::size_t> result;
      for (const data::variable& v: find_free_variables(x))
      {
        result.insert(parameter_position(v));
      }
      return result;
    }

    bool is_true(data::data_expression expr)
    {
      if(m_solver != nullptr)
      {
        bool negate = false;
        if(data::is_forall(expr))
        {
          negate = true;
          const data::forall& f = atermpp::down_cast<data::forall>(expr);
          expr = make_exists(f.variables(), data::sort_bool::not_(f.body()));
        }
        // data::data_expression result = data::one_point_rule_rewrite(m_rewr(expr));
        switch(m_solver->solve(data::variable_list(), expr, m_smt_timeout))
        {
          case smt::answer::SAT: return negate ^ true;
          case smt::answer::UNSAT: return negate ^ false;
          case smt::answer::UNKNOWN: return negate ^ false;
        }
      }
      else
      {
        data::data_expression result = m_rewr(data::one_point_rule_rewrite((m_rewr(expr))));
        if (result != data::sort_bool::true_() && result != data::sort_bool::false_())
        {
          mCRL2log(log::verbose) << "Cannot rewrite " << result << " any further" << std::endl;
        }
        return result == data::sort_bool::true_();
      }
    }

    /// \brief Return true iff k1 can never happen after k happens, as deduced from
    /// predicate dependencies.
    bool dependency_permanently_disables(const std::size_t k, const std::size_t k1) const
    {
      std::size_t N = m_summand_classes.size();
      const summand_class& summand_k = m_summand_classes[k];
      std::set<std::size_t> reachable_after_k;

      // Check to which equations k can lead
      for (std::size_t i = 0; i < m_pbes.equations().size(); i++)
      {
        reachable_after_k.insert(summand_k.nxt[i].begin(), summand_k.nxt[i].end());
      }

      // Explore the rest of the dependency relation
      std::list<std::size_t> todo(reachable_after_k.begin(), reachable_after_k.end());
      while (!todo.empty())
      {
        std::size_t i = todo.front();
        todo.pop_front();
        for (std::size_t k2 = 0; k2 < N; k2++)
        {
          for (const std::size_t j: m_summand_classes[k2].nxt[i])
          {
            if (reachable_after_k.count(j) == 0)
            {
              todo.push_back(j);
              reachable_after_k.insert(j);
            }
          }
        }
      }

      return !std::any_of(reachable_after_k.begin(), reachable_after_k.end(),
        [&](const std::size_t i) { return depends(i, k1); });
    }

    void compute_dependency_NES()
    {
      using utilities::detail::set_union;
      using utilities::detail::has_empty_intersection;

      std::size_t n = m_pbes.equations().size();
      std::size_t N = m_summand_classes.size();

      for (std::size_t i = 0; i < n; i++)
      {
        m_dependency_nes[i].resize(N);
        for (std::size_t k = 0; k < N; k++)
        {
          const std::set<std::size_t>& J = m_summand_classes[k].nxt[i];
          if (J.size() > 1 || (J.size() == 1 && *J.begin() != i))
          {
            m_dependency_nes[i].set(k);
          }
        }
      }
    }

    // returns X_i |--k--> X_j
    bool depends(std::size_t i, std::size_t k, std::size_t j) const
    {
      return m_summand_classes[k].depends(i, j);
    };

    // TODO: precompute this function
    // returns X_i |--k-->
    bool depends(std::size_t i, std::size_t k) const
    {
      std::size_t n = m_pbes.equations().size();
      for (std::size_t j = 0; j < n; j++)
      {
        if (depends(i, k, j))
        {
          return true;
        }
      }
      return false;
    };

    static summand_equivalence_key rename_duplicate_variables(data::set_identifier_generator& id_gen, const summand_equivalence_key& summ)
    {
      std::vector<data::variable> new_variables;
      data::maintain_variables_in_rhs< data::mutable_map_substitution<> > sigma;
      for (const data::variable& var: summ.e)
      {
        core::identifier_string new_name = id_gen(var.name());
        if (new_name != var.name())
        {
          sigma[var] = data::variable(new_name, var.sort());
        }
        new_variables.emplace_back(new_name, var.sort());
      }

      auto replace_vars = [&](const data::data_expression& e)
      {
        return data::replace_variables_capture_avoiding_with_an_identifier_generator(e, sigma, id_gen);
      };

      return summand_equivalence_key(
        data::variable_list(new_variables.begin(), new_variables.end()),
        replace_vars(summ.f),
        data::data_expression_list(summ.g.begin(), summ.g.end(), replace_vars)
      );
    }

    tribool left_accords_equations(std::size_t k, std::size_t k1) const
    {
      std::size_t n = m_pbes.equations().size();
      tribool result = yes;

      for (std::size_t i = 0; i < n; i++)
      {
        for (std::size_t i1 = 0; i1 < n; i1++)
        {
          bool X_k1_X1 = depends(i, k1, i1);
          for (std::size_t i_prime = 0; i_prime < n; i_prime++)
          {
            bool X1_k_Xprime = depends(i1, k, i_prime);
            if (X_k1_X1 && X1_k_Xprime)
            {
              result = maybe;
              bool found = false;
              for (std::size_t i2 = 0; i2 < n; i2++)
              {
                bool X_k_X2 = depends(i, k, i2);
                bool X2_k1_Xprime = depends(i2, k1, i_prime);
                if (X_k_X2 && X2_k1_Xprime)
                {
                  found = true;
                }
              }
              if (!found)
              {
                return no;
              }
            }
          }
        }
      }
      return result;
    }

    tribool square_accords_equations(std::size_t k, std::size_t k1) const
    {
      std::size_t n = m_pbes.equations().size();
      tribool result = yes;

      for (std::size_t i = 0; i < n; i++)
      {
        for (std::size_t i1 = 0; i1 < n; i1++)
        {
          bool X_k1_X1 = depends(i, k1, i1);
          for (std::size_t i2 = 0; i2 < n; i2++)
          {
            bool X_k_X2 = depends(i, k, i2);
            if (X_k1_X1 && X_k_X2)
            {
              result = maybe;
              bool found = false;
              for (std::size_t i_prime = 0; i_prime < n; i_prime++)
              {
                bool X1_k_Xprime = depends(i1, k, i_prime);
                bool X2_k1_Xprime = depends(i2, k1, i_prime);
                if (X1_k_Xprime && X2_k1_Xprime)
                {
                  found = true;
                  break;
                }
              }
              if (!found)
              {
                return no;
              }
            }
          }
        }
      }
      return result;
    }

    tribool triangle_accords_equations(std::size_t k, std::size_t k1) const
    {
      std::size_t n = m_pbes.equations().size();
      tribool result = yes;

      for (std::size_t i = 0; i < n; i++)
      {
        for (std::size_t i1 = 0; i1 < n; i1++)
        {
          bool X_k1_X1 = depends(i, k1, i1);
          for (std::size_t i2 = 0; i2 < n; i2++)
          {
            bool X_k_X2 = depends(i, k, i2);
            bool X2_k1_X1 = depends(i2, k1, i1);
            if (X_k1_X1 && X_k_X2)
            {
              result = maybe;
              if(!X2_k1_X1)
              {
                return no;
              }
            }
          }
        }
      }
      return result;
    }

    void compute_DNA_DNL_NES(const std::vector<parameter_info>& info)
    {
      using utilities::detail::has_empty_intersection;
      using utilities::detail::set_includes;
      using utilities::detail::set_intersection;
      using utilities::detail::set_union;

      std::size_t N = m_summand_classes.size();

      auto Rs = [&](const std::size_t k) { return info[k].Rs; };
      auto Ts = [&](const std::size_t k) { return info[k].Ts; };
      auto Vs = [&](const std::size_t k) { return info[k].Vs; };
      auto Ws = [&](const std::size_t k) { return info[k].Ws; };

      for (std::size_t k = 0; k < N; k++)
      {
        for (std::size_t k1 = 0; k1 < N; k1++)
        {
          if (k == k1)
          {
            continue;
          }
          bool DNL_DNS_affect_sets = has_empty_intersection(set_intersection(Vs(k), Vs(k1)), set_union(Ws(k), Ws(k1)));
          bool DNT_affect_sets = has_empty_intersection(Ws(k), Rs(k1)) && has_empty_intersection(Ws(k), Ts(k1)) && set_includes(Ws(k1), Ws(k));

          summand_relations_data summand_data(*this, k, k1);
          // Use lambda lifting for short-circuiting the && operator on tribools
          bool left_accords     = [&]{ return left_accords_equations(k, k1); } &&
                                  [&](bool needs_yes) { return summand_data.left_accords_data(DNL_DNS_affect_sets, needs_yes); };
          // The DNS relation is symmetric
          bool square_accords   = (k1 < k && !DNS(k1).test(k)) ||
                                  (k1 > k &&
                                      ([&]{ return square_accords_equations(k, k1); } &&
                                       [&](bool needs_yes) { return summand_data.square_accords_data(DNL_DNS_affect_sets, needs_yes); }));
          bool triangle_accords = !m_no_triangle && ([&]{ return triangle_accords_equations(k, k1); } &&
                                                     [&](bool needs_yes) { return summand_data.triangle_accords_data(DNT_affect_sets, needs_yes); });
          bool accords          = square_accords || triangle_accords;
          bool can_enable       = !dependency_permanently_disables(k1, k) && !has_empty_intersection(Ts(k), Ws(k1)) && summand_data.can_enable();

          if (!left_accords)
          {
            DNL(k).set(k1);
          }
          if (!square_accords)
          {
            DNS(k).set(k1);
          }
          if (!accords)
          {
            DNA(k).set(k1);
          }
          if (can_enable)
          {
            NES(k).set(k1);
          }
        }
      }
    }

    void compute_NES_DNA_DNL()
    {
      using utilities::detail::set_union;

      std::size_t N = m_summand_classes.size();
      std::vector<parameter_info> info(N);
      const std::vector<data::variable>& d = m_parameters;

      auto compute_parameter_info = [&](summand_class& summand, parameter_info& info)
      {
        // compute Ts
        std::set<data::variable> FV = find_free_variables(summand.f);
        for (const data::variable& v: summand.e)
        {
          FV.erase(v);
        }
        for (const data::variable& v: FV)
        {
          info.Ts.insert(parameter_position(v));
        }

        // compute Ws and Rs
        auto gi = summand.g.begin();
        auto di = d.begin();
        for ( ; di != d.end(); ++di, ++gi)
        {
          if (*di != *gi)
          {
            std::size_t i = di - d.begin();
            info.Ws.insert(i);

            for (const data::variable& v: find_free_variables(*gi))
            {
              info.Rs.insert(parameter_position(v));
            }
          }
        }

        // compute Vs
        info.Vs = set_union(info.Ts, set_union(info.Ws, info.Rs));
      };

      for (std::size_t k = 0; k < N; k++)
      {
        compute_parameter_info(m_summand_classes[k], info[k]);
      }

      compute_dependency_NES();
      compute_DNA_DNL_NES(info);
    }

    bool compute_deterministic_equations(std::size_t k)
    {
      const summand_class& summand_k = m_summand_classes[k];

      std::size_t n = m_pbes.equations().size();
      for (std::size_t i = 0; i < n; i++)
      {
        if (summand_k.nxt[i].size() >= 2)
        {
          return false;
        }
      }
      return true;
    }

    bool compute_deterministic_data(std::size_t k)
    {
      const summand_class& summand_k = m_summand_classes[k];

      const data::variable_list& parameters = m_pbes.equations()[0].variable().parameters();
      data::set_identifier_generator id_gen;
      for(const data::variable& v: parameters)
      {
        id_gen.add_identifier(v.name());
      }

      summand_equivalence_key new1_k = rename_duplicate_variables(id_gen, summand_equivalence_key(summand_k));
      summand_equivalence_key new2_k = rename_duplicate_variables(id_gen, summand_equivalence_key(summand_k));
      data::variable_list qvars1_k = new1_k.e; data::data_expression condition1_k = new1_k.f; data::data_expression_list updates1_k = new1_k.g;
      data::variable_list qvars2_k = new2_k.e; data::data_expression condition2_k = new2_k.f; data::data_expression_list updates2_k = new2_k.g;

      data::data_expression antecedent = data::sort_bool::and_(condition1_k, condition2_k);
      data::data_expression consequent = data::sort_bool::true_();
      auto it1_k = updates1_k.begin();
      auto it2_k = updates2_k.begin();
      while (it1_k != updates1_k.end())
      {
        consequent = data::lazy::and_(consequent, data::equal_to(*it1_k, *it2_k));
        ++it1_k; ++it2_k;
      }
      data::data_expression condition = make_forall(parameters + qvars1_k + qvars2_k, data::sort_bool::implies(antecedent, consequent));

      // mCRL2log(log::verbose) << "Determinism condition for " << k << ": " << m_rewr(condition) << " original " << condition << std::endl;

      return is_true(condition);
    }

    void compute_deterministic()
    {
      if (m_no_determinisim)
      {
        return;
      }
      std::size_t N = m_summand_classes.size();
      for (std::size_t k = 0; k < N; k++)
      {
        m_summand_classes[k].is_deterministic = compute_deterministic_equations(k) && compute_deterministic_data(k);
      }
    }

    void compute_summand_classes()
    {
      std::size_t n = m_pbes.equations().size();

      for (const srf_equation& eqn: m_pbes.equations())
      {
        for (const srf_summand& summand: eqn.summands())
        {
          summand_equivalence_key key(summand);
          auto i = m_summand_index.find(key);
          if (i == m_summand_index.end())
          {
            std::size_t k = m_summand_index.size();
            m_summand_index[key] = k;
            m_summand_classes.emplace_back(summand.parameters(), summand.condition(), summand.variable().parameters(), n);
          }
        }
      }
      for(summand_class& s: m_summand_classes)
      {
        s.set_num_summands(m_summand_classes.size());
      }
      compute_nxt();
      compute_NES_DNA_DNL();
      compute_deterministic();
    }

    void compute_vis_invis()
    {
      using utilities::detail::contains;

      std::size_t n = m_pbes.equations().size();
      std::size_t N = m_summand_classes.size();

      m_vis.resize(N);
      for (std::size_t i = 0; i < n; i++)
      {
        const srf_equation& eqn = m_pbes.equations()[i];
        const core::identifier_string& X_i = eqn.variable().name();
        bool op_i = eqn.is_conjunctive();
        std::size_t rank_i = m_equation_index.rank(X_i);

        for (const srf_summand& summand: eqn.summands())
        {
          const core::identifier_string& X_j = summand.variable().name();
          std::size_t j = m_equation_index.index(X_j);
          std::size_t rank_j = m_equation_index.rank(X_j);
          bool op_j = m_pbes.equations()[j].is_conjunctive();
          bool is_invisible = op_i == op_j && rank_i == rank_j;
          if (!is_invisible)
          {
            std::size_t k = summand_index(summand);
            m_vis.set(k);
          }
        }
      }

      // Invis is the opposite of vis
      m_invis = m_vis;
      m_invis.flip();
    }

    std::string print_variables(const data::variable_list& v) const
    {
      std::ostringstream out;
      for (auto i = v.begin(); i != v.end(); ++i)
      {
        if (i != v.begin())
        {
          out << ", ";
        }
        out << *i << ": " << i->sort();
      }
      return out.str();
    }

    void print_summand(const srf_summand& summand, bool is_conjunctive) const
    {
      std::size_t k = summand_index(summand);
      mCRL2log(log::verbose) << "   (" << k << ") ";
      if (!summand.parameters().empty())
      {
        mCRL2log(log::verbose) << (is_conjunctive ? "forall " : "exists ") << print_variables(summand.parameters()) << ". ";
      }
      mCRL2log(log::verbose) << summand.condition()
                << (is_conjunctive ? " => " : " && ")
                << summand.variable()
                << std::endl;
    }

    void print_pbes() const
    {
      mCRL2log(log::verbose) << m_pbes.to_pbes() << std::endl;
      mCRL2log(log::verbose) << "srf_pbes" << std::endl;
      for (const srf_equation& eqn: m_pbes.equations())
      {
        mCRL2log(log::verbose) << eqn.symbol() << " " << eqn.variable() << " = " << (eqn.is_conjunctive() ? "conjunction" : "disjunction") << " of summands\n";
        for (const srf_summand& summand: eqn.summands())
        {
          print_summand(summand, eqn.is_conjunctive());
        }
        mCRL2log(log::verbose) << std::endl;
      }
    }

    void print_summand_classes() const
    {
      using utilities::detail::contains;

      if(mCRL2logEnabled(log::verbose))
      {
        std::size_t N = m_summand_classes.size();
        for (std::size_t k = 0; k < N; k++)
        {
          const summand_class& summand = m_summand_classes[k];
          mCRL2log(log::verbose) << "\n--- summand class " << k << " ---" << std::endl;
          mCRL2log(log::verbose) << "visible = " << std::boolalpha << m_vis.test(k) << "\n";
          summand.print(log::mcrl2_logger().get(log::verbose), N);
        }
        for (std::size_t i = 0; i < m_pbes.equations().size(); i++)
        {
          mCRL2log(log::verbose) << "dependency NES[" << std::setw(3) << i << "]  " << print_summand_set(m_dependency_nes[i]) << std::endl;
        }
      }
    }

  public:
    explicit partial_order_reduction_algorithm(const pbes& p,
          data::rewrite_strategy strategy,
          bool use_smt_solver,
          std::size_t smt_timeout,
          bool weak_conditions,
          bool no_determinisim,
          bool no_triangle
        )
     : m_rewr(p.data(),
              //TODO temporarily disabled used_data_equation_selector so the rewriter can rewrite accordance conditions
              // data::used_data_equation_selector(p.data(), pbes_system::find_function_symbols(p), p.global_variables()),
              strategy),
       m_pbes_rewr(m_rewr, p.data()),
       m_enumerator(m_pbes_rewr, p.data(), m_rewr, m_id_generator, false),
       m_pbes(pbes2srf(p)),
       m_equation_index(m_pbes),
       m_dependency_nes(m_pbes.equations().size()),
       m_solver(use_smt_solver ? new smt::smt_solver(p.data()) : nullptr),
       m_smt_timeout(smt_timeout),
       m_use_weak_conditions(weak_conditions),
       m_no_determinisim(no_determinisim),
       m_no_triangle(no_triangle)
    {
      unify_parameters(m_pbes);

      // initialize m_parameters and m_parameter_positions
      const data::variable_list& parameters = m_pbes.equations().front().variable().parameters();
      m_parameters = std::vector<data::variable>{parameters.begin(), parameters.end()};
      for (std::size_t m = 0; m < m_parameters.size(); m++)
      {
        m_parameter_positions[m_parameters[m]] = m;
      }

      const std::chrono::time_point<std::chrono::high_resolution_clock> t_start =
        std::chrono::high_resolution_clock::now();
      compute_summand_classes();
      compute_vis_invis();
      m_static_analysis_duration = std::chrono::high_resolution_clock::now() - t_start;

      // initialize m_largest_equation_size;
      for (std::size_t i = 0; i < m_pbes.equations().size(); i++)
      {
        m_largest_equation_size = std::max(m_largest_equation_size, m_pbes.equations()[i].summands().size());
      }

      mCRL2log(log::verbose) << p << std::endl;
      print_pbes();
    }

    ~partial_order_reduction_algorithm()
    {
      if(m_solver != nullptr)
      {
        delete m_solver;
      }
    }

    const propositional_variable_instantiation& initial_state() const
    {
      return m_pbes.initial_state();
    }

    const std::vector<data::variable>& parameters() const
    {
      return m_parameters;
    }

    const fixpoint_symbol& symbol(const core::identifier_string& X) const
    {
      std::size_t i = m_equation_index.index(X);
      return m_pbes.equations()[i].symbol();
    }

    void print() const
    {
      print_summand_classes();
    }

    template <
      typename EmitNode = utilities::skip,
      typename EmitEdge = utilities::skip
    >
    void explore(
      const propositional_variable_instantiation& X_init,
      EmitNode emit_node = EmitNode(),
      EmitEdge emit_edge = EmitEdge(),
      bool use_condition_L = true
    )
    {
      using utilities::detail::contains;
      using utilities::detail::has_empty_intersection;
      using utilities::detail::set_difference;
      using utilities::detail::set_includes;
      using utilities::detail::set_intersection;
      using utilities::detail::set_union;

      const std::chrono::time_point<std::chrono::high_resolution_clock> t_start =
        std::chrono::high_resolution_clock::now();

      enum todo_state
      {
        NEW,            ///< Will be partially expanded
        DONE_PARTIALLY, ///< Has been partially expanded
        STARTS_CYCLE,   ///< Needs to be fully expanded, because it starts a cycle
        DONE            ///< Has been fully expanded
      };
      typedef std::pair<propositional_variable_instantiation, todo_state> todo_pair;

      // The set seen also stores for each node an index and a boolean that expresses whether
      // the node is currently in the DFS stack.
      std::unordered_map<propositional_variable_instantiation, std::pair<std::size_t, bool>> seen;
      std::deque<todo_pair> todo{todo_pair(X_init, NEW)};
      // Each state is given unique index, based on the order of discovery.
      // This means that the indices in the DFS stack are sorted from low to high.
      std::size_t index = 0;

      {
        std::size_t rank = m_equation_index.rank(X_init.name());
        std::size_t i = m_equation_index.index(X_init.name());
        bool is_conjunctive = m_pbes.equations()[i].is_conjunctive();
        emit_node(X_init, is_conjunctive, rank);
        seen.insert(std::make_pair(X_init, std::make_pair(index, true)));
        index++;
      }

      std::size_t iteration = 0;
      while (!todo.empty())
      {
        todo_pair& p = todo.back();
        const propositional_variable_instantiation X_e = p.first;
        todo_state& s = p.second;
        mCRL2log(log::debug) << "choose X_e = " << X_e << std::endl;

        if (s == DONE || s == DONE_PARTIALLY)
        {
          todo.pop_back();
          seen[X_e].second = false;
          continue;
        }

        std::set<propositional_variable_instantiation> next;
        summand_set en_X_e = en(X_e);

        if (s == NEW)
        {
          summand_set stubborn_set_X_e = stubborn_set(X_e);
          mCRL2log(log::debug) << "stubborn_set(X_e) = " << print_summand_set(stubborn_set_X_e) << std::endl;
          next = succ(X_e, stubborn_set_X_e & en_X_e);

          bool vis_expanded = stubborn_set_X_e.is_subset_of(m_vis);
          s = vis_expanded ? DONE : DONE_PARTIALLY;

          // Check if a cycle is closed
          // At the same time, check whether some node on the stack is fully expanded
          // If both are true, some node will be fully expanded
          bool cycle_found = false;
          std::size_t min_index = std::numeric_limits<std::size_t>::max();
          propositional_variable_instantiation min_node;
          for (const propositional_variable_instantiation& Y_f: next)
          {
            auto node = seen.find(Y_f);
            if (node == seen.end())
            {
              continue;
            }
            std::size_t node_index = node->second.first;
            std::size_t node_instack = node->second.second;
            if (node_instack && node_index < min_index)
            {
              cycle_found = true;
              min_index = std::min(min_index, node_index);
              min_node = Y_f;
            }
          }
          bool fully_expanded_node_found = false;
          auto it = todo.rbegin();
          for (; cycle_found && !fully_expanded_node_found; ++it)
          {
            assert(it != todo.rend());
            fully_expanded_node_found |= (it->second == STARTS_CYCLE || it->second == DONE);
            if(it->first == min_node)
            {
              break;
            }
          }
          if (cycle_found && !fully_expanded_node_found)
          {
            it->second = STARTS_CYCLE;
          }
        }
        else
        {
          assert(s == STARTS_CYCLE);
          next = succ(X_e, en_X_e);
          s = DONE;
        }

        mCRL2log(log::debug) << "next = " << core::detail::print_set(next) << std::endl;
        for (const propositional_variable_instantiation& Y_f: next)
        {
          if (seen.find(Y_f) == seen.end())
          {
            std::size_t rank = m_equation_index.rank(Y_f.name());
            std::size_t i = m_equation_index.index(Y_f.name());
            bool is_conjunctive = m_pbes.equations()[i].is_conjunctive();
            emit_node(Y_f, is_conjunctive, rank);
            seen.insert(std::make_pair(Y_f, std::make_pair(index, true)));
            index++;
            todo.emplace_back(Y_f, NEW);
          }
        }
        for (const propositional_variable_instantiation& Y_f: next)
        {
          emit_edge(X_e, Y_f);
        }

        iteration++;
        if(iteration == 100)
        {
          mCRL2log(log::status) << "Found " << seen.size() << " nodes. Todo set contains " << todo.size() << " nodes.\n";
          iteration = 0;
        }
      }
      mCRL2log(log::verbose) << "Finished exploration, found " << seen.size() << " nodes." << std::endl;

      m_exploration_duration = std::chrono::high_resolution_clock::now() - t_start;
      mCRL2log(log::info) << "timing pbespor (wall clock time in seconds):"
        "\n  static analysis: " << std::chrono::duration<double>(m_static_analysis_duration).count() <<
        "\n  exploration:     " << std::chrono::duration<double>(m_exploration_duration).count() << std::endl;
    }
};

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_PARTIAL_ORDER_REDUCTION_H
