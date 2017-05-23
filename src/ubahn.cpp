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

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>

#include "base/timer.h"
#include "graph_builder.h"
#include "io/xml_reader.h"
#include "solver/station_solver.h"

const char DEFAULT_FILE[] = "ubahn.xml";

const double CHANGING_TIME = 5.0;
const double SWITCHING_TIME = 5.0;
const bool PREPROCESSING = true;
const ProblemType TYPE = STATION;

using std::cout;
using std::endl;
using std::cerr;
using std::unique_ptr;

int main(int argc, char* args[]) {
  XMLReader reader;

  std::string file = DEFAULT_FILE;
  if (argc > 1) {
    file = args[1];
  }

  cout << "Opening transportation network file: " << file << endl;
  try {
    reader.readTransportFile(file);
  } catch (const std::runtime_error& toCatch) {
    cerr << "Error while parsing file: " << endl << toCatch.what() << endl;
    return 1;
  }

  reader.printStatistic();
  cout << endl;

  GraphBuilder ubahnGraph(reader.getStations(), reader.getLines(),
                          CHANGING_TIME, SWITCHING_TIME, TYPE, PREPROCESSING);
  ubahnGraph.printStatistics();
  cout << endl;

  unique_ptr<CplexSolver> solver;
  switch (TYPE) {
    case STATION:
      solver = unique_ptr<CplexSolver>(new StationSolver(
          ubahnGraph.getGraph(), ubahnGraph.getDist(),
          ubahnGraph.getStationNodes(), ubahnGraph.getConnections()));
      break;
    default:
      std::ostringstream err_buf;
      err_buf << "Problem type " << TYPE << " is not supported";
      throw std::runtime_error(err_buf.str());
  }

  cout << "Solving the problem..." << endl;
  Timer solve_timer;
  solver->solve();
  cout << "Done." << endl;

  cout << "Solving took " << solve_timer << " ms."
       // << " (Spent " << solver.getCutGenerationTime() << " ms on cuts)"
       << endl
       << endl;
  cout << "Visiting all stations takes approximately "
       << solver->getSolutionValue() << " minutes"
       << " (assuming that changing takes " << CHANGING_TIME
       << " minutes on average)." << endl;

  // ubahnGraph.printStaticMapURL(solver.getSolutionTour(), true, cout);
  // ubahnGraph.saveTexTour(solver.getSolutionTour(), "Zoologischer Garten",
  // true);

  try {
    ubahnGraph.printTour(solver->getSolutionTour(), "Zoologischer Garten",
                         true);
  } catch (const std::runtime_error& toCatch) {
    ubahnGraph.printTour(solver->getSolutionTour());
  }

  return 0;
}
