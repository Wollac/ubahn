/*
 * Copyright 2017 Wolfgang Welz welzwo@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UBAHN_SOLVER_CPLEX_SOLVER_H_
#define UBAHN_SOLVER_CPLEX_SOLVER_H_

#include <list>

#include "ilcplex/ilocplex.h"

#include "base/graph.h"
#include "transport_defs.h"

class CplexSolver {
 public:
  // number of threads that should be used
  static const int NUM_THREADS = 1;

  explicit CplexSolver(const leda::graph& graph)
      : _g(graph), _cplex(nullptr), _model(nullptr), _solution_found(false) {
    _model = new IloModel(_env);

    _cplex = new IloCplex(*_model);
    _epInt = _cplex->getParam(IloCplex::EpInt);

    // force one thread only
    _cplex->setParam(IloCplex::Threads, NUM_THREADS);

#ifndef NDEBUG
    // save the model as an lp file if we are in debug mode
    _cplex->exportModel("traffic.lp");
#endif
  }

  virtual ~CplexSolver() {
    if (_cplex) {
      _cplex->end();
      delete _cplex;
    }
    if (_model) delete _model;

    _env.end();
  }

  virtual void solve() = 0;

  const std::list<leda::edge>& getSolutionTour() throw(std::runtime_error) {
    if (!_solution_found) throw std::runtime_error("No solution available");

    return _solution_tour;
  }

  const double& getSolutionValue() throw(std::runtime_error) {
    if (!_solution_found) throw std::runtime_error("No solution available");

    return _solution_value;
  }

  const double& getTime() throw(std::runtime_error) {
    if (!_solution_found) throw std::runtime_error("No solution available");

    return _solving_time;
  }

 protected:
  void solve(bool use_callback, IloCplex::Callback cb = nullptr);

  void buildSolutionTour(const IloIntArray& int_vals);

  IloEnv& getCplexEnv() { return _env; }
  IloModel* getCplexModel() { return _model; }
  const IloNum& getEpInt() const { return _epInt; }

  const leda::graph& getGraph() const { return _g; }

  int getNumberOfNodes() const { return _g.number_of_nodes(); }

  int getCplexId(const leda::edge e) const { return _edge_to_var_id[e]; }

  IloNumVar getCplexVar(const leda::edge e) const {
    int id = getCplexId(e);
    return _edge_vars[id];
  }
  const IloNumVarArray& getCplexVars() const { return _edge_vars; }

  template <typename T>
  bool isZero(const T& val) const {
    if (val >= -_epInt && val <= _epInt) {
      return true;
    }
    return false;
  }

  template <typename T>
  bool isOne(const T& val) const {
    if (val >= 1. - _epInt && val <= 1. + _epInt) {
      return true;
    }
    return false;
  }

  IloNumVarArray _edge_vars;
  leda::edge_array<int> _edge_to_var_id;

 private:
  const leda::graph& _g;  ///< Reference to the problem graph

  /// CPLEX related attributes
  IloEnv _env;
  IloCplex* _cplex;
  IloModel* _model;
  IloNum _epInt;

  /// the solution is stored in the next variables
  bool _solution_found;
  double _solution_value;
  double _solving_time;
  std::list<leda::edge> _solution_tour;
};

#endif  // UBAHN_SOLVER_CPLEX_SOLVER_H_
