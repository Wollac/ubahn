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

#ifndef UBAHN_IO_XML_READER_H_
#define UBAHN_IO_XML_READER_H_

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#include "xercesc/parsers/XercesDOMParser.hpp"

#include "transport_defs.h"

class XMLReader {
 public:
  XMLReader();
  ~XMLReader();

  // disallow copy and assign
  XMLReader(const XMLReader&) = delete;
  void operator=(XMLReader) = delete;

  /** Parses the given XML file. throws an exception if something went wrong */
  void readTransportFile(const std::string&);

  /** Prints some basic information about the read transportation network */
  void printStatistic(std::ostream& O = std::cout) const;

  const t_stationmap& getStations() const { return _stations; }
  const t_linemap& getLines() const { return _lines; }

 private:
  void extractStations(XERCES_CPP_NAMESPACE::DOMElement* eStations);
  void setLineStations(XERCES_CPP_NAMESPACE::DOMElement* eLine, Line* l);
  void extractLines(XERCES_CPP_NAMESPACE::DOMElement* eStations);

  /// Result is stored here
  t_stationmap _stations;
  t_linemap _lines;

  XERCES_CPP_NAMESPACE::XercesDOMParser* _parser;  ///< the Xerces DOM parser

  // Internal class use only. Hold Xerces data in UTF-16 SMLCh type.
  XMLCh* TAG_stations;
  XMLCh* TAG_station;
  XMLCh* TAG_lines;
  XMLCh* TAG_line;

  XMLCh* TAG_name;
  XMLCh* TAG_location;
  XMLCh* TAG_time;
};

#endif  // UBAHN_IO_XML_READER_H_
