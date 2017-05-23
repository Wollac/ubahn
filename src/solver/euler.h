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

#ifndef UBAHN_SOLVER_EULER_H_
#define UBAHN_SOLVER_EULER_H_

#include <list>

#include "graph.h"

/** Implementation of Hierholzer's algorithm for finding an Euler Tour. */
class Euler {
 public:
  explicit Euler(const leda::graph& eulerGraph) : _g(eulerGraph) {}

  // disallow copy and assign
  Euler(const Euler&) = delete;
  void operator=(Euler) = delete;

  std::list<leda::edge> getEulerTour(leda::node start = nullptr);

 private:
  void eulerRec(const leda::node current, const leda::node start,
                std::list<leda::edge>& cycle);

  const leda::graph& _g;
  leda::edge_array<bool> _arc_visited;
};

#endif  // UBAHN_SOLVER_EULER_H_
