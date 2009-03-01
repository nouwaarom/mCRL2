// Author(s): Wieger Wesselink
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/constelm.h
/// \brief The constelm algorithm.

#ifndef MCRL2_PBES_CONSTELM_H
#define MCRL2_PBES_CONSTELM_H

// #define MCRL2_PBES_CONSTELM_DEBUG

#include <sstream>
#include <iostream>
#include <utility>
#include <deque>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include "mcrl2/core/messaging.h"
#include "mcrl2/new_data/replace.h"
#include "mcrl2/pbes/pbes.h"
#include "mcrl2/pbes/find.h"
#include "mcrl2/pbes/pbes_expression_visitor.h"
#include "mcrl2/pbes/remove_parameters.h"

namespace mcrl2 {

namespace pbes_system {

/// \cond INTERNAL_DOCS
namespace detail {
  /// \brief Compares two terms
  /// \param v A term
  /// \param w A term
  /// \return True if v is less than w
  inline
  bool less_term(atermpp::aterm_appl v, atermpp::aterm_appl w)
  {
    return ATermAppl(v) < ATermAppl(w);
  }

  template <typename Term>
  struct true_false_pair
  {
    typedef typename core::term_traits<Term>::term_type term_type;
    typedef typename core::term_traits<Term> tr;

    term_type TC;
    term_type FC;

    true_false_pair()
      : TC(tr::true_()), FC(tr::true_())
    {}

    true_false_pair(term_type t, term_type f)
      : TC(t), FC(f)
    {}
  };

  template <typename Term>
  struct apply_exists
  {
    typedef typename core::term_traits<Term>::variable_sequence_type variable_sequence_type;
    typedef typename core::term_traits<Term> tr;

    variable_sequence_type variables_;

    apply_exists(variable_sequence_type variables)
      : variables_(variables)
    {}

    /// \brief Function call operator
    /// \param p A true-false pair
    void operator()(true_false_pair<Term>& p) const
    {
      p.TC = tr::exists(variables_, p.TC);
      p.FC = tr::forall(variables_, p.FC);
    }
  };

  template <typename Term>
  struct apply_forall
  {
    typedef typename core::term_traits<Term>::variable_sequence_type variable_sequence_type;
    typedef typename core::term_traits<Term> tr;

    variable_sequence_type variables_;

    apply_forall(variable_sequence_type variables)
      : variables_(variables)
    {}

    /// \brief Function call operator
    /// \param p A true-false pair
    void operator()(true_false_pair<Term>& p) const
    {
      p.TC = tr::forall(variables_, p.TC);
      p.FC = tr::exists(variables_, p.FC);
    }
  };

  template <typename Term>
  struct constelm_edge_condition
  {
    typedef typename core::term_traits<Term>::term_type term_type;
    typedef typename core::term_traits<Term>::propositional_variable_type propositional_variable_type;
    typedef typename core::term_traits<Term> tr;
    typedef std::multimap<propositional_variable_type, std::vector<true_false_pair<Term> > > condition_map;

    term_type TC;
    term_type FC;
    condition_map condition;  // condT + condF

    /// \brief Returns the true-false pair corresponding to the edge condition
    /// \return The true-false pair corresponding to the edge condition
    true_false_pair<Term> TCFC() const
    {
      return true_false_pair<Term>(TC, FC);
    }

    /// \brief Returns the condition
    /// \param c A sequence of true-false pairs
    /// \return The condition
    term_type compute_condition(const std::vector<true_false_pair<Term> >& c) const
    {
      term_type result = tr::true_();
      for (typename std::vector<true_false_pair<Term> >::const_iterator i = c.begin(); i != c.end(); ++i)
      {
        result = core::optimized_and(result, core::optimized_not(i->TC));
        result = core::optimized_and(result, core::optimized_not(i->FC));
      }
      return result;
    }
  };

  template <typename Term>
  struct edge_condition_visitor: public pbes_expression_visitor<Term, constelm_edge_condition<Term> >
  {
    typedef typename core::term_traits<Term>::term_type term_type;
    typedef typename core::term_traits<Term>::variable_type variable_type;
    typedef typename core::term_traits<Term>::variable_sequence_type variable_sequence_type;
    typedef typename core::term_traits<Term>::data_term_type data_term_type;
    typedef typename core::term_traits<Term>::propositional_variable_type propositional_variable_type;
    typedef constelm_edge_condition<Term> edge_condition;
    typedef typename edge_condition::condition_map condition_map;
    typedef typename core::term_traits<Term> tr;

    // N.B. As a side effect ec1 and ec2 are changed!!!
    void merge_conditions(edge_condition& ec1,
                          edge_condition& ec2,
                          edge_condition& ec
                         )
    {
      for (typename condition_map::iterator i = ec1.condition.begin(); i != ec1.condition.end(); ++i)
      {
        i->second.push_back(ec.TCFC());
        ec.condition.insert(*i);
      }
      for (typename condition_map::iterator i = ec2.condition.begin(); i != ec2.condition.end(); ++i)
      {
        i->second.push_back(ec.TCFC());
        ec.condition.insert(*i);
      }
    }

    /// \brief Visit data_expression node
    /// \param e A term
    /// \param d A data term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_data_expression(const term_type& e, const data_term_type& d, edge_condition& ec)
    {
      ec.TC = d;
      ec.FC = core::optimized_not(d);
      return this->stop_recursion;
    }

    /// \brief Visit true node
    /// \param e A term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_true(const term_type& e, edge_condition& ec)
    {
      ec.TC = tr::true_();
      ec.FC = tr::false_();
      return this->stop_recursion;
    }

    /// \brief Visit false node
    /// \param e A term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_false(const term_type& e, edge_condition& ec)
    {
      ec.TC = tr::false_();
      ec.FC = tr::true_();
      return this->stop_recursion;
    }

    /// \brief Visit not node
    /// \param e A term
    /// \param arg A term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_not(const term_type& e, const term_type& arg, edge_condition& ec)
    {
      edge_condition ec_arg;
      visit(arg, ec_arg);
      ec.TC = ec_arg.FC;
      ec.FC = ec_arg.TC;
      ec.condition = ec_arg.condition;
      return this->stop_recursion;
    }

    /// \brief Visit and node
    /// \param e A term
    /// \param left A term
    /// \param right A term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_and(const term_type& e, const term_type& left, const term_type&  right, edge_condition& ec)
    {
      edge_condition ec_left;
      visit(left, ec_left);
      edge_condition ec_right;
      visit(right, ec_right);
      ec.TC = core::optimized_and(ec_left.TC, ec_right.TC);
      ec.FC = core::optimized_or(ec_left.FC, ec_right.FC);
      merge_conditions(ec_left, ec_right, ec);
      return this->stop_recursion;
    }

    /// \brief Visit or node
    /// \param e A term
    /// \param left A term
    /// \param right A term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_or(const term_type& e, const term_type&  left, const term_type&  right, edge_condition& ec)
    {
      edge_condition ec_left;
      visit(left, ec_left);
      edge_condition ec_right;
      visit(right, ec_right);
      ec.TC = core::optimized_or(ec_left.TC, ec_right.TC);
      ec.FC = core::optimized_and(ec_left.FC, ec_right.FC);
      merge_conditions(ec_left, ec_right, ec);
      return this->stop_recursion;
    }

    /// \brief Visit imp node
    /// \param e A term
    /// \param left A term
    /// \param right A term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_imp(const term_type& e, const term_type&  left, const term_type&  right, edge_condition& ec)
    {
      edge_condition ec_left;
      visit(left, ec_left);
      edge_condition ec_right;
      visit(right, ec_right);
      ec.TC = core::optimized_or(ec_left.FC, ec_right.TC);
      ec.FC = core::optimized_and(ec_left.TC, ec_right.FC);
      merge_conditions(ec_left, ec_right, ec);
      return this->stop_recursion;
    }

    /// \brief Visit forall node
    /// \param e A term
    /// \param variables A sequence of variables
    /// \param expr A term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_forall(const term_type& e, const variable_sequence_type& variables, const term_type& expr, edge_condition& ec)
    {
      visit(expr, ec);
      for (typename condition_map::iterator i = ec.condition.begin(); i != ec.condition.end(); ++i)
      {
        i->second.push_back(ec.TCFC());
        std::for_each(i->second.begin(), i->second.end(), apply_forall<Term>(variables));
      }
      return this->stop_recursion;
    }

    /// \brief Visit exists node
    /// \param e A term
    /// \param variables A sequence of variables
    /// \param expr A term
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_exists(const term_type& e, const variable_sequence_type&  variables, const term_type& expr, edge_condition& ec)
    {
      visit(expr, ec);
      for (typename condition_map::iterator i = ec.condition.begin(); i != ec.condition.end(); ++i)
      {
        i->second.push_back(ec.TCFC());
        std::for_each(i->second.begin(), i->second.end(), apply_exists<Term>(variables));
      }
      return this->stop_recursion;
    }

    /// \brief Visit propositional_variable node
    /// \param e A term
    /// \param v A propositional variable
    /// \param ec An edge condition
    /// \return The result of visiting the node
    bool visit_propositional_variable(const term_type& e, const propositional_variable_type& v, edge_condition& ec)
    {
      ec.TC = tr::false_();
      ec.FC = tr::false_();
      std::vector<true_false_pair<Term> > c;
      c.push_back(true_false_pair<Term>(tr::false_(), tr::false_()));
      ec.condition.insert(std::make_pair(v, c));
      return this->stop_recursion;
    }
  };

  template <typename Container, typename Predicate>
  /// \brief Removes elements from a container
  /// \param container A container
  /// \param pred All elements that satisfy the predicate pred are removed
  /// Note: this implementation is very inefficient!
  void remove_elements(Container& container, Predicate pred)
  {
    std::vector<typename Container::value_type> result;
    for (typename Container::iterator i = container.begin(); i != container.end(); ++i)
    {
      if (!pred(*i))
      {
        result.push_back(*i);
      }
    }
    container = Container(result.begin(), result.end());
  }

  template <typename Variable>
  struct equation_is_contained_in
  {
    const std::set<Variable>& m_variables;

    equation_is_contained_in(const std::set<Variable>& variables)
      : m_variables(variables)
    {}

    template <typename Equation>
    bool operator()(const Equation& e)
    {
      return m_variables.find(e.variable()) != m_variables.end();
    }
  };

  template <typename MapContainer>
  void print_constraint_map(const MapContainer& constraints)
  {
    for (typename MapContainer::const_iterator i = constraints.begin(); i != constraints.end(); ++i)
    {
      std::string lhs = mcrl2::core::pp(i->first);
      std::string rhs = mcrl2::core::pp(i->second);
      std::cout << "{" << lhs << " := " << rhs << "} ";
    }
  }

} // namespace detail
/// \endcond

  /// \brief Algorithm class for the constelm algorithm
  template <typename Term, typename DataRewriter, typename PbesRewriter>
  class pbes_constelm_algorithm
  {
    public:
      /// \brief The term type
      typedef typename core::term_traits<Term>::term_type term_type;

      /// \brief The variable type
      typedef typename core::term_traits<Term>::variable_type variable_type;

      /// \brief The variable sequence type
      typedef typename core::term_traits<Term>::variable_sequence_type variable_sequence_type;

      /// \brief The data term type
      typedef typename core::term_traits<Term>::data_term_type data_term_type;

      /// \brief The data term sequence type
      typedef typename core::term_traits<Term>::data_term_sequence_type data_term_sequence_type;

      /// \brief The string type
      typedef typename core::term_traits<Term>::string_type string_type;

      /// \brief The propositional variable declaration type
      typedef typename core::term_traits<Term>::propositional_variable_decl_type propositional_variable_decl_type;

      /// \brief The propositional variable instantiation type
      typedef typename core::term_traits<Term>::propositional_variable_type propositional_variable_type;

      /// \brief The visitor type for edge conditions
      typedef typename detail::edge_condition_visitor<Term> edge_condition_visitor;

      /// \brief The edge condition type
      typedef typename edge_condition_visitor::edge_condition edge_condition;

      /// \brief The edge condition map type
      typedef typename edge_condition_visitor::condition_map condition_map;

      /// \brief The term traits
      typedef typename core::term_traits<Term> tr;

    protected:
      /// \brief A map with constraints on the vertices of the graph
      typedef std::map<variable_type, data_term_type> constraint_map;

      /// \brief Compares data expressions for equality.
      DataRewriter m_data_rewriter;

      /// \brief Compares data expressions for equality.
      PbesRewriter m_pbes_rewriter;

      /// \brief Represents an edge of the dependency graph. The assignments are stored
      /// implicitly using the 'right' parameter. The condition determines under
      /// what circumstances the influence of the edge is propagated to its target
      /// vertex.
      struct edge
      {
        /// \brief The propositional variable at the source of the edge
        propositional_variable_decl_type source;

        /// \brief The propositional variable instantiation that determines the target of the edge
        propositional_variable_type target;

        /// \brief The condition of the edge
        term_type condition;

        /// \brief Constructor
        edge()
        {}

        /// \brief Constructor
        /// \param l A propositional variable declaration
        /// \param r A propositional variable
        /// \param c A term
        edge(propositional_variable_decl_type src, propositional_variable_type tgt, term_type c = pbes_expr::true_())
         : source(src), target(tgt), condition(c)
        {}

        /// \brief Returns a string representation of the edge.
        /// \return A string representation of the edge.
        std::string to_string() const
        {
          std::ostringstream out;
          out << "(" << mcrl2::core::pp(source.name()) << ", " << mcrl2::core::pp(target.name()) << ")  label = " << mcrl2::core::pp(target) << "  condition = " << mcrl2::core::pp(condition);
          return out.str();
        }
      };

      /// \brief Represents a vertex of the dependency graph.
      struct vertex
      {
        /// \brief The propositional variable that corresponds to the vertex
        propositional_variable_decl_type variable;

        /// \brief Maps data variables to data expressions. If the right hand side is a data
        /// variable, it means that it represents NaC ("not a constant").
        constraint_map constraints;

        /// \brief Constructor
        vertex()
        {}

        /// \brief Constructor
        /// \param v A propositional variable declaration
        vertex(propositional_variable_decl_type x)
          : variable(x)
        {}

        /// \brief Returns true if the data variable v has been assigned a constant expression.
        /// \param v A variable
        /// \return True if the data variable v has been assigned a constant expression.
        bool is_constant(variable_type v) const
        {
          typename constraint_map::const_iterator i = constraints.find(v);
          return i != constraints.end() && !core::term_traits<data_term_type>::is_variable(i->second);
        }

        /// \brief Returns the constant parameters of this vertex.
        /// \return The constant parameters of this vertex.
        std::vector<variable_type> constant_parameters() const
        {
          std::vector<variable_type> result;
          variable_sequence_type parameters(variable.parameters());
          for (typename variable_sequence_type::iterator i = parameters.begin(); i != parameters.end(); ++i)
          {
            if (is_constant(*i))
            {
              result.push_back(*i);
            }
          }
          return result;
        }

        /// \brief Returns the indices of the constant parameters of this vertex.
        /// \return The indices of the constant parameters of this vertex.
        std::vector<int> constant_parameter_indices() const
        {
          std::vector<int> result;
          int index = 0;
          variable_sequence_type parameters(variable.parameters());
          for (typename variable_sequence_type::iterator i = parameters.begin(); i != parameters.end(); ++i, index++)
          {
            if (is_constant(*i))
            {
              result.push_back(index);
            }
          }
          return result;
        }

        /// \brief Returns a string representation of the vertex.
        /// \return A string representation of the vertex.
        std::string to_string() const
        {
          std::ostringstream out;
          out << mcrl2::core::pp(variable) << "  assertions = ";
          for (typename constraint_map::const_iterator i = constraints.begin(); i != constraints.end(); ++i)
          {
            std::string lhs = mcrl2::core::pp(i->first);
            std::string rhs = mcrl2::core::pp(i->second);
            out << "{" << lhs << " := " << rhs << "} ";
          }
          return out.str();
        }

        /// \brief Assign new values to the parameters of this vertex, and update the constraints accordingly.
        /// The new values have a number of constraints.
        bool update(data_term_sequence_type e, const constraint_map& e_constraints, DataRewriter datar)
        {
          bool changed = false;

          typename data_term_sequence_type::iterator i;
          typename variable_sequence_type::iterator j;
          variable_sequence_type params = variable.parameters();

          if (constraints.empty())
          {
            for (i = e.begin(), j = params.begin(); i != e.end(); ++i, ++j)
            {
              data_term_type e1 = datar(new_data::variable_map_replace(*i, e_constraints));
              if (core::term_traits<data_term_type>::is_constant(e1))
              {
                constraints[*j] = e1;
              }
              else
              {
              	constraints[*j] = *j;
              }
            }
            changed = true;
          }
          else
          {
            for (i = e.begin(), j = params.begin(); i != e.end(); ++i, ++j)
            {
              typename constraint_map::iterator k = constraints.find(*j);
              assert(k != constraints.end());
              data_term_type& ci = k->second;
              if (ci == *j)
              {
                continue;
              }
              data_term_type ei = datar(new_data::variable_map_replace(*i, e_constraints));
              if (ci != ei)
              {
                ci = *j;
                changed = true;
              }
            }
          }
          return changed;
        }
      };

      /// \brief The storage type for vertices
      typedef std::map<string_type, vertex> vertex_map;

      /// \brief The storage type for edges
      typedef std::map<string_type, std::vector<edge> > edge_map;

      /// \brief The vertices of the dependency graph. They are stored in a map, to
      /// support searching for a vertex.
      vertex_map m_vertices;

      /// \brief The edges of the dependency graph. They are stored in a map, to
      /// easily access all out-edges corresponding to a particular vertex.
      edge_map m_edges;

      /// \brief The redundant parameters.
      std::map<string_type, std::vector<int> > m_redundant_parameters;

      /// \brief The redundant propositional variables.
      std::set<propositional_variable_decl_type> m_redundant_equations;

      /// \brief Prints the vertices of the dependency graph.
      void print_vertices() const
      {
        for (typename vertex_map::const_iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
        {
          std::cerr << i->second.to_string() << std::endl;
        }
      }

      /// \brief Prints the edges of the dependency graph.
      void print_edges() const
      {
        for (typename edge_map::const_iterator i = m_edges.begin(); i != m_edges.end(); ++i)
        {
          for (typename std::vector<edge>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
          {
            std::cerr << j->to_string() << std::endl;
          }
        }
      }

    public:

      /// \brief Constructor.
      /// \param datar A data rewriter
      /// \param pbesr A PBES rewriter
      pbes_constelm_algorithm(DataRewriter datar, PbesRewriter pbesr)
        : m_data_rewriter(datar), m_pbes_rewriter(pbesr)
      {}

      /// \brief Returns the parameters that have been removed by the constelm algorithm
      /// \return The removed parameters
      std::map<propositional_variable_decl_type, std::vector<variable_type> > redundant_parameters() const
      {
        std::map<propositional_variable_decl_type, std::vector<variable_type> > result;
        for (typename std::map<string_type, std::vector<int> >::const_iterator i = m_redundant_parameters.begin(); i != m_redundant_parameters.end(); ++i)
        {
          const vertex& v = m_vertices.find(i->first)->second;
          std::vector<variable_type>& variables = result[v.variable];
          for (std::vector<int>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
          {
            // std::advance doesn't work for aterm lists :-(
            variable_sequence_type parameters(v.variable.parameters());
            typename variable_sequence_type::iterator k = parameters.begin();
            for (int i = 0; i < *j; i++)
            {
              ++k;
            }
            variables.push_back(*k);
          }
        }
        return result;
      }

      /// \brief Returns the propositional variables that have optionally been removed by the constelm algorithm
      /// \return The removed variables
      const std::set<propositional_variable_decl_type>& redundant_equations() const
      {
        return m_redundant_equations;
      }

      /// \brief Runs the constelm algorithm
      /// \param p A pbes
      /// \param name_generator A generator for fresh identifiers
      /// \param remove_redundant_equations If true, redundant equations are removed from the PBES
      /// The call \p name_generator() should return an identifier that doesn't appear
      /// in the pbes \p p
      /// \param compute_conditions If true, propagation conditions are computed. Note
      /// that the currently implementation has exponential behavior.
      template <typename Container>
      void run(pbes<Container>& p, bool compute_conditions = false, bool remove_redundant_equations = false)
      {
        m_vertices.clear();
        m_edges.clear();
        m_redundant_parameters.clear();
        m_redundant_equations.clear();

        // compute the vertices and edges of the dependency graph
        for (typename Container::const_iterator i = p.equations().begin(); i != p.equations().end(); ++i)
        {
          string_type name = i->variable().name();
          m_vertices[name] = vertex(i->variable());

          if (compute_conditions)
          {
            // use an edge_condition_visitor to compute the edges
            edge_condition ec;
            edge_condition_visitor visitor;
            visitor.visit(i->formula(), ec);
            if (!ec.condition.empty())
            {
              std::vector<edge>& edges = m_edges[name];
              for (typename condition_map::iterator j = ec.condition.begin(); j != ec.condition.end(); ++j)
              {
                propositional_variable_type X = j->first;
                term_type condition = ec.compute_condition(j->second);
                edges.push_back(edge(i->variable(), X, condition));
              }
            }
          }
          else
          {
            // use find function to compute the edges
            std::set<propositional_variable_type> inst = find_all_propositional_variable_instantiations(i->formula());
            if (!inst.empty())
            {
              std::vector<edge>& edges = m_edges[name];
              for (typename std::set<propositional_variable_type>::iterator k = inst.begin(); k != inst.end(); ++k)
              {
                edges.push_back(edge(i->variable(), *k));
              }
            }
          }
        }

        // initialize the todo list of vertices that need to be processed
        std::deque<propositional_variable_decl_type> todo;
        std::set<propositional_variable_decl_type> visited;
        std::set<propositional_variable_type> inst = find_all_propositional_variable_instantiations(p.initial_state());
        for (typename std::set<propositional_variable_type>::iterator i = inst.begin(); i != inst.end(); ++i)
        {
          data_term_sequence_type e = i->parameters();
          vertex& u = m_vertices[i->name()];
          u.update(e, constraint_map(), m_data_rewriter);
          todo.push_back(u.variable);
          visited.insert(u.variable);
        }

        if (mcrl2::core::gsDebug)
        {
          std::cerr << "\n--- initial vertices ---" << std::endl;
          print_vertices();
          std::cerr << "\n--- edges ---" << std::endl;
          print_edges();
        }

        // propagate constraints over the edges until the todo list is empty
        while (!todo.empty())
        {
#ifdef MCRL2_PBES_CONSTELM_DEBUG
std::cerr << "\n<todo list>" << core::pp(propositional_variable_list(todo.begin(), todo.end())) << std::endl;
#endif
          propositional_variable_decl_type var = todo.front();

          // remove all occurrences of var from todo
          todo.erase(std::remove(todo.begin(), todo.end(), var), todo.end());

          const vertex& u = m_vertices[var.name()];
          std::vector<edge>& u_edges = m_edges[var.name()];
          variable_sequence_type Xparams = u.variable.parameters();

          for (typename std::vector<edge>::const_iterator ei = u_edges.begin(); ei != u_edges.end(); ++ei)
          {
            const edge& e = *ei;
            vertex& v = m_vertices[e.target.name()];
#ifdef MCRL2_PBES_CONSTELM_DEBUG
std::cerr << "\n<updating edge>" << e.to_string() << std::endl;
std::cerr << "  <source vertex       >" << u.to_string() << std::endl;
std::cerr << "  <target vertex before>" << v.to_string() << std::endl;
#endif

            term_type value = m_pbes_rewriter(new_data::variable_map_replace(e.condition, u.constraints));
#ifdef MCRL2_PBES_CONSTELM_DEBUG
std::cerr << "\nEvaluated condition " << core::pp(new_data::variable_map_replace(e.condition, u.constraints)) << " to " << core::pp(value) << std::endl;
#endif
            if (!tr::is_false(value) && !tr::is_true(value))
            {
#ifdef MCRL2_PBES_CONSTELM_DEBUG
std::cerr << "\nCould not evaluate condition " << core::pp(new_data::variable_map_replace(e.condition, u.constraints)) << " to true or false";
#endif
            }
            if (!tr::is_false(value))
            {
              bool changed = v.update(e.target.parameters(), u.constraints, m_data_rewriter);
              if (changed)
              {
                todo.push_back(v.variable);
                visited.insert(v.variable);
              }
            }
#ifdef MCRL2_PBES_CONSTELM_DEBUG
std::cerr << "  <target vertex after >" << v.to_string() << std::endl;
#endif
          }
        }

        if (mcrl2::core::gsDebug)
        {
          std::cerr << "\n--- final vertices ---" << std::endl;
          print_vertices();
        }

        // compute the redundant parameters and the redundant equations
        for (typename Container::iterator i = p.equations().begin(); i != p.equations().end(); ++i)
        {
          string_type name = i->variable().name();
          vertex& v = m_vertices[name];
          if (v.constraints.empty())
          {
            if (visited.find(i->variable()) == visited.end())
            {
              m_redundant_equations.insert(i->variable());
            }
          }
          else
          {
            std::vector<int> r = v.constant_parameter_indices();
            if (!r.empty())
            {
              m_redundant_parameters[name] = r;
            }
          }
        }

        // Apply the constraints to the equations.
        for (typename Container::iterator i = p.equations().begin(); i != p.equations().end(); ++i)
        {
          string_type name = i->variable().name();
          vertex& v = m_vertices[name];

          if (!v.constraints.empty())
          {
            *i = pbes_equation(
              i->symbol(),
              i->variable(),
              new_data::variable_map_replace(i->formula(), v.constraints)
            );
          }
        }

        // remove the redundant parameters and variables/equations
        remove_parameters(p, m_redundant_parameters);
        if (remove_redundant_equations)
        {
          remove_elements(p.equations(), detail::equation_is_contained_in<propositional_variable_decl_type>(m_redundant_equations));
        }

        // print the parameters and equation that are removed
        if (mcrl2::core::gsVerbose)
        {
          std::cerr << "\nremoved the following constant parameters:" << std::endl;
          std::map<propositional_variable_decl_type, std::vector<variable_type> > v = redundant_parameters();
          for (typename std::map<propositional_variable_decl_type, std::vector<variable_type> >::iterator i = v.begin(); i != v.end(); ++i)
          {
            for (typename std::vector<variable_type>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
            {
              std::cerr << "  parameter (" << mcrl2::core::pp(i->first.name()) << ", " << core::pp(*j) << ")" << std::endl;
            }
          }

          if (remove_redundant_equations)
          {
            std::cerr << "\nremoved the following equations:" << std::endl;
            const std::set<propositional_variable_decl_type> r = redundant_equations();
            for (typename std::set<propositional_variable_decl_type>::const_iterator i = r.begin(); i != r.end(); ++i)
            {
              std::cerr << "  equation " << core::pp(i->name()) << std::endl;
            }
          }
        }
      }
  };

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_CONSTELM_H
