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

#include "graph_builder.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "LEDA/graph/basic_graph_alg.h"
#include "LEDA/graph/shortest_path.h"
#include "boost/lexical_cast.hpp"

#include "graph.h"

using leda::edge_map;
using leda::edge_array;
using leda::node_array;
using leda::node_map;
using std::endl;
using std::map;
using std::ostream;
using std::set;
using std::string;
using std::vector;

namespace {

bool isTerminalStation(const vector<string>::const_iterator it,
                       const vector<string>& stations) {
  // if the iterator points to the first or last element
  if (it == stations.begin() || it + 1 == stations.end()) {
    return true;
  }

  return false;
}

template <class Iterator>
bool isConnectingStation(Iterator it, const t_stationmap& stationMap) {
  t_stationmap::const_iterator pos = stationMap.find(*it);
  assert(pos != stationMap.end());

  if (pos->second->lines.size() > 1) {
    return true;
  }

  return false;
}

/** Checks whether there is a connecting station on that line after the current
 * station, denoted by the iterator. */
bool hasFollowingConnectingStation(vector<string>::const_iterator it,
                                   const vector<string>& stations,
                                   const t_stationmap& stationMap) {
  for (auto it2 = it + 1; it2 != stations.end(); ++it2) {
    if (isConnectingStation(it2, stationMap)) {
      return true;
    }
  }

  return false;
}

/** Checks whether there is a connecting station on that line before the current
 * station, denoted by the iterator. */
bool hasPrecedingConnectingStation(vector<string>::const_iterator it,
                                   const vector<string>& stations,
                                   const t_stationmap& stationMap) {
  for (auto it2 = stations.begin(); it2 != it; ++it2) {
    if (isConnectingStation(it2, stationMap)) return true;
  }

  return false;
}

/**
 * Returns all deg-2 chains of the graph.
 * This is defenitely not the most efficient option, but it works...
 */
vector<std::list<node>> computeDegree2Chains(
    graph& g, const edge_array<bool>& keep_edge) {
  vector<node> nodes_to_hide;

  node n;
  forall_nodes(n, g) {
    assert(g.degree(n) >= 2);

    if (g.degree(n) > 2) {
      nodes_to_hide.push_back(n);
    }
  }

  for (node n : nodes_to_hide) {
    g.hide_node(n);
  }

  // to keep an edge, we remove its source and target
  edge e;
  forall_edges(e, g) {
    if (keep_edge[e]) {
      g.hide_node(source(e));
      g.hide_node(target(e));
    }
  }

  leda::list<node> sorted_nodes;
  if (!TOPSORT(g, sorted_nodes)) {
    throw std::runtime_error("The graph contains a cycle");
  }

  // in the resulting graph each component represents a valid deg-2 chain
  node_array<int> comp_num(g);
  int no_components = COMPONENTS(g, comp_num);

  g.restore_all_nodes();
  g.restore_all_edges();

  vector<std::list<node>> chains(no_components);
  forall(n, sorted_nodes) {
    assert(g.degree(n) == 2);

    chains[comp_num[n]].push_back(n);
  }

  return chains;
}
}  // namespace

GraphBuilder::GraphBuilder(const t_stationmap& stations, const t_linemap& lines,
                           double change_cost, double switch_cost,
                           ProblemType type, bool preprocess)
    : _stations(stations),
      _lines(lines),
      _nodes_removed(false),
      _change_cost(change_cost),
      _switch_cost(switch_cost),
      _dist(_g, _change_cost),
      _connection_arcs(_g, true),
      _arc_names(_g, CHANGE_NAME),
      _node_names(_g, "") {
  // create one node for every node and every line in both directions
  map<string, map<string, node>> way_nodemap, back_nodemap;
  createNodesAndTravelArcs(way_nodemap, back_nodemap);

  if (!preprocess) {
    addAllSwitchingArcs(way_nodemap, back_nodemap);
    addAllConnectionArcs(way_nodemap, back_nodemap);
  } else {
    if (type == SEGMENT) {
      addSwitchingArcsAtTerminals(way_nodemap, back_nodemap);
    } else {
      addStationProblemSwitchingArcs(way_nodemap, back_nodemap);
    }
    addAllConnectionArcs(way_nodemap, back_nodemap);

    preprocessGraph(type);
  }

  // set the reversal information for each edge
  _g.make_map();

  checkConnectivity();
}

void GraphBuilder::checkConnectivity() {
  node_array<int> compnum(_g);
  int num_components = COMPONENTS(_g, compnum);

  if (num_components > 1) {
    node first_node = _g.first_node();

    node n;
    forall_nodes(n, _g) {
      if (compnum[n] != compnum[first_node]) {
        std::ostringstream errBuf;
        errBuf << "No connection between stations " << _node_names[first_node]
               << " and " << _node_names[n];
        throw std::runtime_error(errBuf.str());
      }
    }
  }
}

/**
 * Adds a connection edge, only if there is a non connection edge going out of
 * t and at least one non-connection edge going into s.
 */
edge GraphBuilder::addConnectionEdge(const node s, const node t) {
  edge e;

  bool has_in = false;
  forall_in_edges(e, s) {
    if (!_connection_arcs[e]) {
      has_in = true;
      break;
    }
  }

  bool has_out = false;
  forall_out_edges(e, t) {
    if (!_connection_arcs[e]) {
      has_out = true;
      break;
    }
  }

  // changing more than once in a row makes no sense
  if (has_in && has_out) {
    return _g.new_edge(s, t);
  }

  return nullptr;
}

/** Add all nodes to the graph and the arcs that correspond to riding. */
void GraphBuilder::createNodesAndTravelArcs(
    map<string, map<string, node>>& way_nodemap,
    map<string, map<string, node>>& back_nodemap) {
  // add the nodes and edges for the original direction
  for (auto it = _lines.begin(); it != _lines.end(); ++it) {
    const string& lineName = it->second->name;
    const vector<string>& lineStations = it->second->stations;

    node lastNode = nullptr;

    int station_index = 0;
    for (const string& station : lineStations) {
      // add a new node in the given direction
      const node currentNode = _g.new_node();
      way_nodemap[lineName][station] = currentNode;
      _node_names[currentNode] = station;

      _station_nodes[station].insert(currentNode);

      // connect it with the last station, and set the arc variables accordingly
      if (lastNode) {
        edge e = _g.new_edge(lastNode, currentNode);
        _arc_names[e] = lineName;
        _connection_arcs[e] = false;

        const int travel_time = it->second->times[station_index] -
                                it->second->times[station_index - 1];
        assert(travel_time > 0);
        _dist[e] = travel_time;
      }
      lastNode = currentNode;
      station_index++;
    }
  }

  // add the nodes and edges for the reversed direction
  for (auto it = _lines.begin(); it != _lines.end(); ++it) {
    const string& lineName = it->second->name;
    const vector<string>& lineStations = it->second->stations;

    node lastNode = nullptr;

    int station_index = lineStations.size() - 1;
    for (auto it2 = lineStations.rbegin(); it2 != lineStations.rend(); ++it2) {
      const string& station = *it2;

      node currentNode = _g.new_node();
      back_nodemap[lineName][station] = currentNode;
      _node_names[currentNode] = station;

      _station_nodes[station].insert(currentNode);

      // connect it with the last station, and set the arc variables accordingly
      if (lastNode) {
        edge e = _g.new_edge(lastNode, currentNode);
        _arc_names[e] = lineName;
        _connection_arcs[e] = false;

        const int travel_time = it->second->times[station_index + 1] -
                                it->second->times[station_index];
        assert(travel_time > 0);
        _dist[e] = travel_time;
      }
      lastNode = currentNode;
      station_index--;
    }
  }
}

void GraphBuilder::addAllSwitchingArcs(
    map<string, map<string, node>>& way_nodemap,
    map<string, map<string, node>>& back_nodemap) {
  for (auto it = _lines.begin(); it != _lines.end(); ++it) {
    const string& lineName = it->second->name;
    const vector<string>& lineStations = it->second->stations;

    for (const string& station_name : lineStations) {
      edge way_edge = _g.new_edge(way_nodemap[lineName][station_name],
                                  back_nodemap[lineName][station_name]);
      _dist[way_edge] = _switch_cost;
      _connection_arcs[way_edge] = true;

      edge back_edge = _g.new_edge(back_nodemap[lineName][station_name],
                                   way_nodemap[lineName][station_name]);
      _dist[back_edge] = _switch_cost;
      _connection_arcs[back_edge] = true;
    }
  }
}

void GraphBuilder::addSwitchingArcsAtTerminals(
    map<string, map<string, node>>& way_nodemap,
    map<string, map<string, node>>& back_nodemap) {
  for (auto it = _lines.begin(); it != _lines.end(); ++it) {
    const string& lineName = it->second->name;
    const vector<string>& lineStations = it->second->stations;

    for (vector<string>::const_iterator it2 = lineStations.begin();
         it2 != lineStations.end(); ++it2) {
      if (isTerminalStation(it2, lineStations)) {
        if (it2 == lineStations.begin()) {
          edge e = _g.new_edge(back_nodemap[lineName][*it2],
                               way_nodemap[lineName][*it2]);
          _dist[e] = _switch_cost;
          _connection_arcs[e] = true;
        } else {
          edge e = _g.new_edge(way_nodemap[lineName][*it2],
                               back_nodemap[lineName][*it2]);
          _dist[e] = _switch_cost;
          _connection_arcs[e] = true;
        }
      }
    }
  }
}

void GraphBuilder::addStationProblemSwitchingArcs(
    map<string, map<string, node>>& way_nodemap,
    map<string, map<string, node>>& back_nodemap) {
  for (auto it = _lines.begin(); it != _lines.end(); ++it) {
    const string& lineName = it->second->name;
    const vector<string>& lineStations = it->second->stations;

    // add arcs for wiitching the direction of the same line
    for (vector<string>::const_iterator it2 = lineStations.begin();
         it2 != lineStations.end(); ++it2) {
      // never switch at a changing station
      if (isConnectingStation(it2, _stations)) {
        continue;
      }

      // switching can occur if the current station is a terminal station...
      if (isTerminalStation(it2, lineStations)) {
        if (it2 == lineStations.begin()) {
          const edge e = _g.new_edge(back_nodemap[lineName][*it2],
                                     way_nodemap[lineName][*it2]);
          _dist[e] = _switch_cost;
          _connection_arcs[e] = true;
        } else {
          const edge e = _g.new_edge(way_nodemap[lineName][*it2],
                                     back_nodemap[lineName][*it2]);
          _dist[e] = _switch_cost;
          _connection_arcs[e] = true;
        }

        continue;
      }

      // never switch between the terminal and the first/last changing station
      if (!hasPrecedingConnectingStation(it2, lineStations, _stations) ||
          !hasFollowingConnectingStation(it2, lineStations, _stations)) {
        continue;
      }

      // switch if the next/previous station is a connecting station
      // todo: This is not allwyas correct!
      // there might be situations with very long stations, were it could pay of
      // to switch twice before and after that station. I doubt that this would
      // occure in practice though
      if (isConnectingStation(it2 + 1, _stations)) {
        const edge e = _g.new_edge(way_nodemap[lineName][*it2],
                                   back_nodemap[lineName][*it2]);

        _dist[e] = _switch_cost;
        _connection_arcs[e] = true;
      }
      if (isConnectingStation(it2 - 1, _stations)) {
        const edge e = _g.new_edge(back_nodemap[lineName][*it2],
                                   way_nodemap[lineName][*it2]);

        _dist[e] = _switch_cost;
        _connection_arcs[e] = true;
      }
    }
  }
}

void GraphBuilder::addAllConnectionArcs(
    map<string, map<string, node>>& way_nodemap,
    map<string, map<string, node>>& back_nodemap) {
  for (auto it = _stations.begin(); it != _stations.end(); ++it) {
    const string& station_name = it->second->name;

    for (set<string>::const_iterator line_it = it->second->lines.begin();
         line_it != it->second->lines.end(); ++line_it) {
      for (set<string>::const_iterator line_it2 = line_it;
           line_it2 != it->second->lines.end(); ++line_it2) {
        if (line_it2 == line_it) continue;

        setDistance(addConnectionEdge(way_nodemap[*line_it][station_name],
                                      way_nodemap[*line_it2][station_name]),
                    _change_cost);
        setDistance(addConnectionEdge(way_nodemap[*line_it][station_name],
                                      back_nodemap[*line_it2][station_name]),
                    _change_cost);
        setDistance(addConnectionEdge(back_nodemap[*line_it][station_name],
                                      way_nodemap[*line_it2][station_name]),
                    _change_cost);
        setDistance(addConnectionEdge(back_nodemap[*line_it][station_name],
                                      back_nodemap[*line_it2][station_name]),
                    _change_cost);

        setDistance(addConnectionEdge(way_nodemap[*line_it2][station_name],
                                      way_nodemap[*line_it][station_name]),
                    _change_cost);
        setDistance(addConnectionEdge(way_nodemap[*line_it2][station_name],
                                      back_nodemap[*line_it][station_name]),
                    _change_cost);
        setDistance(addConnectionEdge(back_nodemap[*line_it2][station_name],
                                      way_nodemap[*line_it][station_name]),
                    _change_cost);
        setDistance(addConnectionEdge(back_nodemap[*line_it2][station_name],
                                      back_nodemap[*line_it][station_name]),
                    _change_cost);
      }
    }
  }
}

/** Removes all the nodes from the graph that have indeg and outdeg of one. */
void GraphBuilder::preprocessGraph(ProblemType type) {
  vector<std::list<node>> chains = computeDegree2Chains(_g, _connection_arcs);

  vector<node> redundant_nodes;
  for (const std::list<node>& list : chains) {
    // for the station problem we must assure that all stations must be visited
    if (type == STATION) {
      // the chain can only be removed, if either the node left of the chain or
      // the node right of the cahin must always be visited
      const node pred_node = source(_g.in_edges(list.front()).front());
      const node succ_node = target(_g.out_edges(list.back()).front());

      if (_g.indeg(pred_node) > 1 && _g.indeg(succ_node) > 1) {
        continue;
      }
    }

    // remove the chain very inefficiently node by node
    for (node n : list) {
      assert(_g.indeg(n) == 1 && _g.outdeg(n) == 1);

      edge in_edge = _g.in_edges(n).front();
      edge out_edge = _g.out_edges(n).front();

      assert(_arc_names[in_edge].compare(_arc_names[out_edge]) == 0);

      const edge new_edge = _g.new_edge(source(in_edge), target(out_edge));
      _dist[new_edge] = _dist[in_edge] + _dist[out_edge];
      _connection_arcs[new_edge] = false;
      _arc_names[new_edge] = _arc_names[in_edge];

      redundant_nodes.push_back(n);
      _g.del_node(n);
    }
  }

  // remove the deleted node also from the stations map
  for (auto map_it = _station_nodes.begin(); map_it != _station_nodes.end();) {
    for (node n : redundant_nodes) {
      map_it->second.erase(n);
    }

    if (map_it->second.empty()) {
      _station_nodes.erase(map_it++);
    } else {
      ++map_it;
    }
  }
}

void GraphBuilder::printStatistics(std::ostream& O) const {
  node_array<int> compnum(_g, 0);
  const int nComponents = COMPONENTS(_g, compnum);

  int nStations = 0;
  double stationSum = 0.0;
  edge e;
  forall_edges(e, _g) {
    if (!_connection_arcs[e]) {
      nStations++;
      stationSum += _dist[e];
    }
  }

  O << "Graph statistics:" << endl;
  O << " Nodes: " << _g.number_of_nodes() << endl;
  O << " Arcs: " << _g.number_of_edges() << endl;
  O << " Components: " << nComponents << endl;
  O << " Avg. station cost: " << stationSum / nStations << endl;
  O << " Changing cost: " << _change_cost << endl;
}

class MaxStringLength {
 public:
  MaxStringLength() : _max_length(0) {}
  MaxStringLength(MaxStringLength&&) = default;

  void operator()(const string& s) {
    _max_length = std::max(_max_length, s.length());
  }
  size_t getMaxLength() const { return _max_length; }

 private:
  size_t _max_length;
};

void GraphBuilder::getTourOutput(const std::list<edge>& tour, bool compact,
                                 vector<string>* column) const {
  column[0].push_back("Start");
  column[1].push_back("Line");
  column[2].push_back("Destination");
  column[3].push_back("Time (m)");
  double time = 0;

  for (std::list<edge>::const_iterator it = tour.begin(); it != tour.end();
       ++it) {
    assert(column[2].size() == 1 ||
           column[2].back().compare(_node_names[source(*it)]) == 0);

    time += _dist[*it];

    if (compact && column[1].size() > 1 &&
        column[1].back().compare(_arc_names[*it]) == 0 &&
        column[2].back().compare(_node_names[source(*it)]) == 0) {
      column[2].pop_back();
      column[2].push_back(_node_names[target(*it)]);

      column[3].pop_back();
      column[3].push_back(boost::lexical_cast<string>(round(time)));
    } else {
      column[0].push_back(_node_names[source(*it)]);
      column[2].push_back(_node_names[target(*it)]);
      column[1].push_back(_arc_names[*it]);
      column[3].push_back(boost::lexical_cast<string>(round(time)));
    }
  }

  if (compact) {
    int i = 0;
    for (vector<string>::iterator it = column[1].begin();
         it != column[1].end();) {
      if (it->compare(CHANGE_NAME) == 0 && it != column[1].begin() &&
          it + 1 != column[1].end()) {
        it = column[1].erase(it);
        column[0].erase(column[0].begin() + i);
        column[2].erase(column[2].begin() + i);
        column[3].erase(column[3].begin() + i);
      } else {
        ++it;
        ++i;
      }
    }
  }
}

void GraphBuilder::saveTexTour(const std::list<edge>& tour, string start,
                               bool compact, ostream& O) const {
  std::list<edge>::const_iterator startPos = tour.end();
  for (std::list<edge>::const_iterator it = tour.begin(); it != tour.end();
       ++it) {
    if (_node_names[source(*it)].compare(start) == 0) {
      startPos = it;
    }
  }
  if (startPos == tour.end()) {
    std::ostringstream errBuf;
    errBuf << "Station " << start << " is not in the tour";
    throw std::runtime_error(errBuf.str());
  }

  std::list<edge> myTour;

  myTour.insert(myTour.end(), startPos, tour.end());
  myTour.insert(myTour.end(), tour.begin(), startPos);

  saveTexTour(myTour, compact, O);
}

std::string space2plus(std::string text) {
  for (std::string::iterator it = text.begin(); it != text.end(); ++it) {
    if (*it == ' ') {
      *it = '+';
    }
  }
  return text;
}

void GraphBuilder::printLocations(const std::list<edge>& tour,
                                  ostream& O) const {
  std::string last = "";
  for (std::list<edge>::const_iterator it = tour.begin(); it != tour.end();
       ++it) {
    if (last.compare(_node_names[source(*it)]) != 0) {
      const Station* station = _stations.find(_node_names[source(*it)])->second;
      O << station->location << endl;
    }
    last = _node_names[source(*it)];
  }
}

/*
void GraphBuilder::printStaticMapURL(const std::list<edge>& tour, bool
compact,
                                     ostream& O) const {
  vector<string> column[4];
  getTourOutput(tour, compact, column);

  O << "http://maps.googleapis.com/maps/api/"
       "staticmap?path=color:0x0000ff|weight:5";

  for (uint i = 1; i < column[0].size(); i++) {
    std::string s = space2plus(column[0][i]);

    O << "|U+Bahnhof+" << s << ",+Berlin";
  }

  O << "|U+Bahnhof+" << space2plus(column[0][1]) << ",+Berlin"
    << "&zoom=11&scale=2&size=640x480&sensor=false" << endl;
}
*/

void GraphBuilder::saveTexTour(const std::list<edge>& tour, bool compact,
                               ostream& O) const {
  vector<string> column[4];
  getTourOutput(tour, compact, column);

  O << "\\documentclass{article}" << endl;
  O << "\\usepackage{booktabs}" << endl;
  O << "\\usepackage[utf8]{inputenc}" << endl;
  O << "\\begin{document}" << endl;

  O << "\\begin{tabular}{lllr}" << endl << "\\toprule" << endl;
  O << column[0][0] << " & " << column[1][0] << " & " << column[2][0] << " & "
    << column[3][0] << "\\\\" << endl
    << "\\midrule" << endl;
  for (uint i = 1; i < column[0].size(); i++) {
    O << column[0][i] << " & " << column[1][i] << " & " << column[2][i] << " & "
      << column[3][i] << "\\\\" << endl;
  }
  O << "\\bottomrule" << endl << "\\end {tabular}" << endl;

  O << "\\end{document}" << endl;
}

vector<string>::iterator findStation(Line* line, const string station_name) {
  for (auto it = line->stations.begin(); it != line->stations.end(); ++it) {
    if (it->compare(station_name) == 0) {
      return it;
    }
  }

  return line->stations.end();
}

void GraphBuilder::printSwitchingStatistics(const std::list<edge>& tour,
                                            ostream& O) const {
  O << "Switching stations:" << endl;

  string lastLine = "";
  for (std::list<edge>::const_iterator it = tour.begin(); it != tour.end();) {
    edge e = *it++;

    if (_node_names[source(e)].compare(_node_names[target(e)]) == 0) {
      if (it != tour.end()) {
        string lineName = _arc_names[*it];

        if (lastLine.compare(lineName) == 0) {
          Line* line = _lines.at(lineName);

          const string& station_name = _node_names[source(e)];

          auto it = findStation(line, station_name);

          bool terminal = isTerminalStation(it, line->stations);

          bool next_is_transfer =
              terminal ? false : isConnectingStation(it + 1, _stations);
          bool prev_is_transfer =
              terminal ? false : isConnectingStation(it - 1, _stations);

          O << " " << lineName << " " << _node_names[source(e)] << " "
            << terminal << prev_is_transfer
            << isConnectingStation(it, _stations) << next_is_transfer << endl;
        }
      }
    }

    lastLine = _arc_names[e];
  }
}

void GraphBuilder::printTour(const std::list<edge>& tour, string start,
                             bool compact, ostream& O) const {
  std::list<edge>::const_iterator startPos = tour.end();
  for (std::list<edge>::const_iterator it = tour.begin(); it != tour.end();
       ++it) {
    if (_node_names[source(*it)].compare(start) == 0) {
      startPos = it;
    }
  }
  if (startPos == tour.end()) {
    std::ostringstream errBuf;
    errBuf << "Station " << start << " is not in the tour";
    throw std::runtime_error(errBuf.str());
  }

  std::list<edge> myTour;

  myTour.insert(myTour.end(), startPos, tour.end());
  myTour.insert(myTour.end(), tour.begin(), startPos);

  // printSwitchingStatistics(myTour, O);

  printTour(myTour, compact, O);
}

int GraphBuilder::tourGetChanges(const std::list<edge>& tour) const {
  int nchanges = 0;
  for (std::list<edge>::const_iterator it = tour.begin(); it != tour.end();
       ++it) {
    if (_arc_names[*it].compare(CHANGE_NAME) == 0) {
      nchanges++;
    }
  }

  return nchanges;
}

void GraphBuilder::printTour(const std::list<edge>& tour, bool compact,
                             ostream& O) const {
  // if nodes were removed due to preprocessing always print in the compact
  // way
  if (_nodes_removed) {
    compact = true;
  }

  edge_array<int> edge_count(_g, 0);
  for (std::list<edge>::const_iterator it = tour.begin(); it != tour.end();
       ++it) {
    edge_count[*it]++;
  }

  edge e;
  forall_edges(e, _g) {
    if (edge_count[e] > 1) {
      O << "The segment " << _node_names[source(e)] << "->"
        << _node_names[target(e)] << " is used " << edge_count[e] << " times."
        << endl;
    }
  }

  vector<string> column[4];
  getTourOutput(tour, compact, column);

  O << "The following tour contains " << tourGetChanges(tour) << " changes"
    << endl;

  uint length[4];
  for (int i = 0; i < 4; i++) {
    MaxStringLength strLength =
        std::for_each(column[i].begin(), column[i].end(), MaxStringLength());
    length[i] = strLength.getMaxLength();
  }

  for (uint i = 0; i < column[0].size(); i++) {
    O << " ";
    for (int j = 0; j < 4; j++) {
      O << column[j][i] << std::setw(length[j] - column[j][i].size() + 2)
        << "\t";
    }
    O << endl;
  }
}
