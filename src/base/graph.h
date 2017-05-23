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

/*
 * graph.h
 *
 * A very simple library that resolves the ECLIPSE problem with LEDA
 */

#ifndef UBAHN_GRAPH_H_
#define UBAHN_GRAPH_H_

#include <cassert>
#include <climits>
#include <cstdio>
#include <sstream>
#include <string>

#include "LEDA/graph/graph.h"

using leda::graph;
using leda::node;
using leda::edge;

// no Idea what LEDA_FORALL_PREAMBLE is for, it is just causing troubles
#undef LEDA_FORALL_PREAMBLE
#define LEDA_FORALL_PREAMBLE

#endif  // UBAHN_GRAPH_H_
