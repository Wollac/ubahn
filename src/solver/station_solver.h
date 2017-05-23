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

#ifndef UBAHN_SOLVER_STATION_SOLVER_H_
#define UBAHN_SOLVER_STATION_SOLVER_H_

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ilcplex/ilocplex.h"

#include "base/graph.h"
#include "solver/cplex_solver.h"

class StationSolver : public CplexSolver {
 public:
  /**
   * Initializes the solver for the problem
   * @param graph problem graph
   * @param dist arc costs
   * @param stations multimap that maps each station to its graph nodes
   * @param connection_arcs edge array that specifies which arc only represents
   * a connection
   */
  StationSolver(const leda::graph& graph, const leda::edge_array<double>& dist,
                const std::map<std::string, std::set<leda::node>>& stations,
                const leda::edge_array<bool>& connection_arcs)
      : CplexSolver(graph) {
    initializeStations(stations);
    createCplexModel(dist, stations, connection_arcs);
  }

  // disallow copy and assign
  StationSolver(const StationSolver&) = delete;
  void operator=(StationSolver) = delete;

  /** Solves the given problem, throws an exception if something goes wrong */
  void solve();

 private:
  void initializeStations(
      const std::map<std::string, std::set<leda::node>>& stations);
  void createCplexModel(
      const leda::edge_array<double>& dist,
      const std::map<std::string, std::set<leda::node>>& stations,
      const leda::edge_array<bool>& connection_arcs);

  leda::list<leda::edge> getSelectedEdges(const IloNumArray& vals) const;
  void createNewSubgraph(const IloNumArray& arc_vals,
                         leda::GRAPH<leda::node, leda::edge>& subgraph,
                         leda::node_array<leda::node>& nodemap) const;
  int getComponent(
      leda::node n, const leda::node_array<leda::node>& nodemap,
      const leda::node_array<int>& compnum,
      const std::vector<std::set<int>>& components_per_station) const;
  void createAggregatedCut(
      int comp, const leda::node_array<node>& nodemap,
      const leda::node_array<int>& compnum,
      const std::vector<std::set<int>>& components_per_station,
      IloExpr& row) const;
  void createDeaggregatedCut(
      int comp, const leda::node_array<node>& nodemap,
      const leda::node_array<int>& compnum,
      const std::vector<std::set<int>>& components_per_station,
      IloExpr& row_out, IloExpr& row_in) const;

  int getNumberOfStations() const { return _n_stations; }

  int getStation(const leda::node n) const { return _node_to_station_id[n]; }

  leda::node_array<int> _node_to_station_id;
  int _n_stations;

  // the dynamic constrained generation method should have access to private
  friend class StationLazyCallbackI;
};

#endif  // UBAHN_SOLVER_STATION_SOLVER_H_
