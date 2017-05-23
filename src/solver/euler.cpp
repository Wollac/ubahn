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

#include "solver/euler.h"

#include <list>
#include <stdexcept>

#include "base/graph.h"

std::list<edge> Euler::getEulerTour(node start) {
  std::list<edge> cycle;

  // an empty graph has an empty Euler Tour
  if (_g.empty()) {
    return cycle;
  }

  _arc_visited.init(_g, false);

  // if no start node is specified, just take the first node in the graph
  if (!start) {
    start = _g.first_node();
  }

  // start the recursion
  eulerRec(start, start, cycle);

  return cycle;
}

void Euler::eulerRec(node current, node start, std::list<edge>& cycle) {
  std::list<edge>::iterator listPos;

  bool first_edge = true;
  edge e;
  forall_out_edges(e, current) {
    if (!_arc_visited[e]) {
      if (first_edge) {
        first_edge = false;

        // at the current edge and mark visited
        cycle.push_back(e);
        _arc_visited[e] = true;

        // we need a pointer to the last actual element and not the dummy
        // cycle.end()
        listPos = cycle.end();
        listPos--;

        // continue from the target node
        eulerRec(target(e), start, cycle);
      } else {
        // all edge after the first on get new cycles

        std::list<edge> newCycle;
        eulerRec(current, current, newCycle);

        // now we can insert the new cycle before the marked position
        cycle.insert(listPos, newCycle.begin(), newCycle.end());
      }
    }
  }

  // if there is no further edge to take and we are not back in the start node,
  // then the graph must be wrong
  if (first_edge && current != start) {
    throw std::runtime_error("The Graph is not Eulerian");
  }
}
