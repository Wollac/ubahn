// Copyright 2017 Wolfgang Welz welzwo@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "solver/cplex_solver.h"

#include <list>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "solver/euler.h"

using std::set;
using std::vector;

using LEDA::node_array;
using LEDA::edge_array;
using LEDA::edge_map;

void CplexSolver::solve(bool use_callback, IloCplex::Callback cb) {
  // reset the current solution
  _solution_found = false;
  _solution_value = 0.0;
  _solution_tour.clear();

  try {
    if (use_callback) {
      // only set this parameter, so that we don't get a warning at runtime
      _cplex->setParam(IloCplex::MIPSearch, IloCplex::Traditional);

      // use the subtour callback
      _cplex->use(cb);
    }

    bool cplex_solved = _cplex->solve();

    // the problem should be infeasible or solved optimally
    if (!cplex_solved || _cplex->getStatus() != IloAlgorithm::Optimal) {
      throw std::runtime_error("Invalid model: No optimal solution found");
    }

    _solving_time = _cplex->getTime();
    _solution_value = _cplex->getObjValue();

    IloNumArray x(_env);
    _cplex->getValues(x, getCplexVars());

    // this already throws a runtime_error if the garph is not eulerian
    buildSolutionTour(x.toIntArray());
    x.end();

    // if everything went well up to this point, we actually found a valid
    // solutions
    _solution_found = true;
  } catch (const IloCplex::Exception& e) {
    std::ostringstream errBuf;
    errBuf << "Cplex Exception: " << e.getMessage();
    throw std::runtime_error(errBuf.str());
  }
}

void CplexSolver::buildSolutionTour(const IloIntArray& int_vals) {
  leda::graph eulerGraph;

  node_array<node> eulerNodes(_g, 0);

  // in the new graph make a copy for every node in the original graph
  node n;
  forall_nodes(n, _g) {
    const node eulerNode = eulerGraph.new_node();

    eulerNodes[n] = eulerNode;
  }

  edge_map<edge> edgeRef(eulerGraph);

  // now add only the used arcs to the euler graph
  node startNode = nullptr;
  edge e;
  forall_edges(e, _g) {
    // the start node is the first node, that has an outgoing arc
    if (!startNode && int_vals[getCplexId(e)] > 0) {
      startNode = eulerNodes[source(e)];
    }

    // if the value is greater than 1, we need to construct multiple parallel
    // edges
    for (int i = 1; i <= int_vals[getCplexId(e)]; i++) {
      const edge eulerEdge =
          eulerGraph.new_edge(eulerNodes[source(e)], eulerNodes[target(e)]);
      edgeRef[eulerEdge] = e;
    }
  }
  assert(startNode);

  Euler euler(eulerGraph);

  std::list<edge> eulerTour;
  try {
    eulerTour = euler.getEulerTour(startNode);
  } catch (const std::runtime_error& e) {
    std::ostringstream errBuf;
    errBuf << "Invalid solution: " << e.what();
    throw std::runtime_error(errBuf.str());
  }

  int selected_arcs = 0;
  forall_edges(e, _g) { selected_arcs += int_vals[getCplexId(e)]; }

  if (selected_arcs != eulerTour.size()) {
    throw std::runtime_error("Invalid solution: Solution contains sub tours");
  }

  // finally, transform the Euler Tour back to the original graph
  _solution_tour.clear();
  for (edge e : eulerTour) {
    edge original_edge = edgeRef[e];
    _solution_tour.push_back(original_edge);
  }
}
