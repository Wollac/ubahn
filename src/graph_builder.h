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

#ifndef UBAHN_GRAPH_BUILDER_H_
#define UBAHN_GRAPH_BUILDER_H_

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/graph.h"
#include "transport_defs.h"

class GraphBuilder {
 public:
  GraphBuilder(const t_stationmap& stations, const t_linemap& lines,
               double change_cost, double switch_cost, ProblemType type,
               bool preprocess = true);

  // disallow copy and assign
  GraphBuilder(const GraphBuilder&) = delete;
  void operator=(GraphBuilder) = delete;

  void printStatistics(std::ostream& O = std::cout) const;

  void saveTexTour(const std::list<leda::edge>& tour, std::string start,
                   bool compact = true, std::ostream& O = std::cout) const;
  void saveTexTour(const std::list<leda::edge>& tour, bool compact = true,
                   std::ostream& O = std::cout) const;
  void printLocations(const std::list<leda::edge>& tour, std::ostream& O) const;

  void printTour(const std::list<leda::edge>& tour, std::string start,
                 bool compact = true, std::ostream& O = std::cout) const;
  void printTour(const std::list<leda::edge>& tour, bool compact = true,
                 std::ostream& O = std::cout) const;

  const leda::graph& getGraph() { return _g; }
  const leda::edge_map<double>& getDist() { return _dist; }
  const std::map<std::string, std::set<leda::node>>& getStationNodes() {
    return _station_nodes;
  }
  const leda::edge_map<bool>& getConnections() { return _connection_arcs; }

 private:
  void createNodesAndTravelArcs(
      std::map<std::string, std::map<std::string, leda::node>>& way_nodemap,
      std::map<std::string, std::map<std::string, leda::node>>& back_nodemap);

  void addAllSwitchingArcs(
      std::map<std::string, std::map<std::string, leda::node>>& way_nodemap,
      std::map<std::string, std::map<std::string, leda::node>>& back_nodemap);
  void addSwitchingArcsAtTerminals(
      std::map<std::string, std::map<std::string, leda::node>>& way_nodemap,
      std::map<std::string, std::map<std::string, leda::node>>& back_nodemap);
  void addStationProblemSwitchingArcs(
      std::map<std::string, std::map<std::string, leda::node>>& way_nodemap,
      std::map<std::string, std::map<std::string, leda::node>>& back_nodemap);

  void addAllConnectionArcs(
      std::map<std::string, std::map<std::string, leda::node>>& way_nodemap,
      std::map<std::string, std::map<std::string, leda::node>>& back_nodemap);

  void checkConnectivity();
  void preprocessGraph(ProblemType type);

  void printSwitchingStatistics(const std::list<leda::edge>& tour,
                                std::ostream& O) const;
  void getTourOutput(const std::list<leda::edge>& tour, bool compact,
                     std::vector<std::string>* column) const;
  int tourGetChanges(const std::list<leda::edge>& tour) const;

  leda::edge addConnectionEdge(const leda::node s, const leda::node t);

  void setDistance(const leda::edge e, double dist) {
    if (e) {
      _dist[e] = dist;
      _connection_arcs[e] = true;
    }
  }

  const t_stationmap _stations;
  const t_linemap _lines;

  bool _nodes_removed;

  const double _change_cost;
  const double _switch_cost;

  leda::graph _g;
  leda::edge_map<double> _dist;

  std::map<std::string, std::set<leda::node>> _station_nodes;
  leda::edge_map<bool> _connection_arcs;

  leda::edge_map<std::string> _arc_names;
  leda::node_map<std::string> _node_names;
};

#endif  // UBAHN_GRAPH_BUILDER_H_
