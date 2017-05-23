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

#include "solver/station_solver.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "LEDA/graph/graph_alg.h"
#include "ilcplex/ilocplex.h"

#include "base/utils.h"
#include "solver/euler.h"

using LEDA::node_array;
using LEDA::edge_array;
using LEDA::edge_map;

ILOSTLBEGIN

ILOLAZYCONSTRAINTCALLBACK1(StationLazyCallback, const StationSolver*, solver) {
  IloEnv masterEnv = getEnv();

  // get the current solution
  IloNumArray x(masterEnv);
  getValues(x, solver->getCplexVars());

  // construct the graph G_x induced by the current (integral) solution x
  leda::GRAPH<node, edge> Gx;
  node_array<node> original_to_solution_graph;
  solver->createNewSubgraph(x, Gx, original_to_solution_graph);

  // we no longer need the actual solution
  x.end();

  // find all connected components in (the undirected version) Gx
  node_array<int> compnum(Gx, -1);
  COMPONENTS(Gx, compnum);

  // sort all components by the station ID they are in
  vector<set<int>> components_per_station(solver->getNumberOfStations());
  node n;
  forall_nodes(n, Gx) {
    assert(compnum[n] >= 0);

    // get the station of the current node
    const node original_node = Gx[n];
    const int station_id = solver->getStation(original_node);

    components_per_station[station_id].insert(compnum[n]);
  }

  // identify those components that have a station which they use exclusively
  set<int> components_with_excl_station;
  for (const set<int>& components : components_per_station) {
    assert(!components.empty());

    if (components.size() == 1) {
      const int component = getFirstElement(components);
      components_with_excl_station.insert(component);
    }
  }
  // we assumed, that the graph has at least one unique station
  assert(components_with_excl_station.size() >= 1);

  // cuts are only feasible, if C AND \neg{C} have an exclusive station
  if (components_with_excl_station.size() == 1) {
    // there is one tour visiting each station => feasible
    return;
  }

  for (int comp : components_with_excl_station) {
    // create a cut for each exclusive component
    IloExpr row_out(masterEnv);
    IloExpr row_in(masterEnv);
    solver->createDeaggregatedCut(comp, original_to_solution_graph, compnum,
                                  components_per_station, row_out, row_in);

    add(row_out >= 1).end();
    row_out.end();
    add(row_in >= 1).end();
    row_in.end();
  }

  return;
}

namespace {
/** Returns the number of edges leading comming from a different statation. */
int countNonStationInEdges(const set<node>& station_nodes) {
  int result = 0;

  for (node n : station_nodes) {
    edge e;
    forall_in_edges(e, n) {
      if (!contains(station_nodes, source(e))) {
        result++;
      }
    }
  }

  return result;
}

/** Returns the first station, that can only be visited by a single line. */
map<string, set<node>>::const_iterator findUniqueStation(
    const map<string, set<node>>& stations) {
  for (auto it = stations.begin(); it != stations.end(); ++it) {
    if (countNonStationInEdges(it->second) == 1) {
      return it;
    }
  }

  return stations.end();
}
}  // namespace

/**
 * Creates the cut including all those arcs either entering or leaving the
 * specified component.
 * As the components are only given on the solution graph the getComponent
 * function is used to derive components of the original graph.
 */
void StationSolver::createAggregatedCut(
    int comp, const node_array<node>& nodemap, const node_array<int>& compnum,
    const vector<set<int>>& components_per_station, IloExpr& row) const {
  edge e;
  forall_edges(e, getGraph()) {
    int comp_s =
        getComponent(source(e), nodemap, compnum, components_per_station);
    int comp_t =
        getComponent(target(e), nodemap, compnum, components_per_station);

    if ((comp_s == comp) ^ (comp_t == comp)) {
      row += getCplexVar(e);
    }
  }
}

/**
 * Creates two cuts including all those arcs entering the specified component
 * or leaving respectivly.
 * As the components are only given on the solution graph the getComponent
 * function is used to derive components of the original graph.
 */
void StationSolver::createDeaggregatedCut(
    int comp, const node_array<node>& nodemap, const node_array<int>& compnum,
    const vector<set<int>>& components_per_station, IloExpr& row_out,
    IloExpr& row_in) const {
  edge e;
  forall_edges(e, getGraph()) {
    int comp_s =
        getComponent(source(e), nodemap, compnum, components_per_station);
    int comp_t =
        getComponent(target(e), nodemap, compnum, components_per_station);

    if ((comp_s == comp) && !(comp_t == comp)) {
      row_out += getCplexVar(e);
    }
    if (!(comp_s == comp) && (comp_t == comp)) {
      row_in += getCplexVar(e);
    }
  }
}

/**
 * Returns the derived component number of a node n in the original graph.
 * If n is also included in the solution graph, use that component.
 * Otherwise return a component that also visits the station of n.
 */
int StationSolver::getComponent(
    node n, const node_array<node>& nodemap, const node_array<int>& compnum,
    const vector<set<int>>& components_per_station) const {
  node transformed = nodemap[n];
  if (transformed) {
    return compnum[transformed];
  }

  int station = getStation(n);

  assert(!components_per_station[station].empty());
  return getFirstElement(components_per_station[station]);
}

/**
 * Returns all the edges that hava a solution value of one.
 * Throw an excpetion, if the solution is not binary,
 */
leda::list<edge> StationSolver::getSelectedEdges(
    const IloNumArray& vals) const {
  leda::list<edge> edges;

  edge e;
  forall_edges(e, getGraph()) {
    int var_id = getCplexId(e);

    if (isOne(vals[var_id])) {
      edges.push_back(e);
    } else if (!isZero(vals[var_id])) {
      throw std::runtime_error("Illegal value: Not binary");
    }
  }

  return edges;
}

/**
 * Creates the graph induced by the given solution.
 * The graph itself contains references to the original nodes/edges.
 */
void StationSolver::createNewSubgraph(const IloNumArray& arc_vals,
                                      leda::GRAPH<node, edge>& subgraph,
                                      node_array<node>& nodemap) const {
  leda::list<edge> selected_edges = getSelectedEdges(arc_vals);

  // create a subgraph induced by the selected edges
  CopyGraph(subgraph, getGraph(), selected_edges);

  // fill the node map, to map subgraph nodes to original
  nodemap.init(getGraph(), nullptr);
  node n;
  forall_nodes(n, subgraph) { nodemap[subgraph[n]] = n; }
}

void StationSolver::solve() {
  CplexSolver::solve(true, StationLazyCallback(getCplexEnv(), this));
}

/** Initializes structures for the station infos and checks for valid input. */
void StationSolver::initializeStations(const map<string, set<node>>& stations) {
  if (findUniqueStation(stations) == stations.end()) {
    throw std::runtime_error(
        "Invalid input: Optimality can only be guaranteed, if the graph "
        "contains at least one unique station");
  }

  const graph& graph = getGraph();

  _node_to_station_id.init(graph, -1);
  set<node> nodes_in_stations;

  _n_stations = 0;
  for (const auto& station : stations) {
    for (node n : station.second) {
      if (contains(nodes_in_stations, n)) {
        ostringstream errBuf;
        errBuf << "Invalid input: Station " << station.first
               << " has a station allready contained in a different station";
        throw runtime_error(errBuf.str());
      }

      _node_to_station_id[n] = _n_stations;
      nodes_in_stations.insert(n);
    }
    _n_stations++;
  }

  if (graph.number_of_nodes() != nodes_in_stations.size()) {
    throw runtime_error(
        "Invalid input: Not all nodes are assigned to stations");
  }
}

/** Creates the actual MIP model. */
void StationSolver::createCplexModel(
    const edge_array<double>& dist,
    const map<string, std::set<node>>& station_names,
    const edge_array<bool>& connection_arcs) {
  IloEnv env = getCplexEnv();

  IloObjective obj = IloMinimize(env);
  getCplexModel()->add(obj);

  int n_nodes = 0;
  node_array<int> node_to_constraint_id(getGraph(), -1);

  node n;
  forall_nodes(n, getGraph()) { node_to_constraint_id[n] = n_nodes++; }

  // for every node the indegree must be equal to the out degree
  IloRangeArray in_out_cons = IloRangeArray(env, n_nodes, 0, 0);
  getCplexModel()->add(in_out_cons);

  _edge_vars = IloNumVarArray(env);
  _edge_to_var_id.init(getGraph(), -1);

  // (1) create a variable for every arc
  int narcs = 0;
  edge e;
  forall_edges(e, getGraph()) {
    const node s = source(e);
    const node t = target(e);

    ostringstream name;
    name << "x#" << s->id() << "_" << t->id();

    // the arc variable has its distance as the cost and must fulfill the in/out
    // degree constraints
    IloBoolVar var(obj(dist[e]) + in_out_cons[node_to_constraint_id[s]](-1) +
                       in_out_cons[node_to_constraint_id[t]](1),
                   name.str().c_str());

    _edge_vars.add(var);
    _edge_to_var_id[e] = narcs++;
  }
  getCplexModel()->add(_edge_vars);
  in_out_cons.end();

  // (2) we create the constraints, that every station is visited at least once
  IloRangeArray out_cons = IloRangeArray(env);

  int cluster_id = 0;
  for (auto& station : station_names) {
    IloExpr lhs(env);

    // each cluster containing all the nodes corresponding to one station should
    // have at least one outgoing arc
    const set<node>& station_nodes = station.second;
    for (node n : station_nodes) {
      edge e;
      forall_out_edges(e, n) {
        assert(source(e) == n);

        if (station_nodes.find(target(e)) == station_nodes.end()) {
          assert(!connection_arcs[e]);
          lhs += getCplexVar(e);
        }
      }
    }

    ostringstream name;
    name << "cl#" << cluster_id++;
    IloRange constr(env, 1.0, lhs, IloInfinity, name.str().c_str());

    out_cons.add(constr);
    lhs.end();
  }
  getCplexModel()->add(out_cons);
  out_cons.end();
}
