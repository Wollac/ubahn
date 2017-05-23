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

#ifndef UBAHN_TRANSPORT_DEFS_H_
#define UBAHN_TRANSPORT_DEFS_H_

#include <map>
#include <set>
#include <string>
#include <vector>

const std::string CHANGE_NAME = "<->";

class Station {
 public:
  explicit Station(const std::string name, const std::string location = "")
      : name(name), location(location), lines() {}

  const std::string name;
  const std::string location;
  std::set<std::string> lines;
};

typedef std::map<std::string, Station*> t_stationmap;

class Line {
 public:
  explicit Line(const std::string name) : name(name), stations() {}

  std::string name;
  std::vector<std::string> stations;
  std::vector<unsigned int> times;
};

typedef std::map<std::string, Line*> t_linemap;

enum ProblemType { STATION, SEGMENT };

#endif  // UBAHN_TRANSPORT_DEFS_H_
