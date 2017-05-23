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

#include "xml_reader.h"

#include <sys/stat.h>

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "boost/lexical_cast.hpp"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/util/PlatformUtils.hpp"
#include "xercesc/util/XMLString.hpp"

#include "transport_defs.h"

using std::string;
using std::map;
using std::set;
using std::ostringstream;
using std::ostream;
using std::endl;
using std::runtime_error;

XERCES_CPP_NAMESPACE_USE

XMLReader::XMLReader() {
  try {
    XMLPlatformUtils::Initialize();
  } catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    std::cerr << "Error during initialization: " << message << endl;
    XMLString::release(&message);
  }

  TAG_station = XMLString::transcode("station");
  TAG_stations = XMLString::transcode("stations");
  TAG_lines = XMLString::transcode("lines");
  TAG_line = XMLString::transcode("line");
  TAG_name = XMLString::transcode("name");
  TAG_location = XMLString::transcode("location");
  TAG_time = XMLString::transcode("time");

  _parser = new XercesDOMParser();
}

XMLReader::~XMLReader() {
  for (map<string, Station*>::iterator it = _stations.begin();
       it != _stations.end(); ++it) {
    delete it->second;
  }
  for (map<string, Line*>::iterator it = _lines.begin(); it != _lines.end();
       ++it) {
    delete it->second;
  }

  delete _parser;

  try {
    XMLString::release(&TAG_station);
    XMLString::release(&TAG_stations);
    XMLString::release(&TAG_lines);
    XMLString::release(&TAG_line);
    XMLString::release(&TAG_name);
    XMLString::release(&TAG_time);
  } catch (...) {
    std::cerr << "Unknown exception encountered in TagNamesdtor" << endl;
  }

  // Terminate Xerces
  try {
    XMLPlatformUtils::Terminate();  // Terminate after release of memory
  } catch (xercesc::XMLException& e) {
    char* message = xercesc::XMLString::transcode(e.getMessage());

    std::cerr << "XML toolkit teardown error: " << message << endl;
    XMLString::release(&message);
  }
}

void XMLReader::extractStations(DOMElement* eStations) {
  DOMNodeList* nlStations = eStations->getElementsByTagName(TAG_station);

  set<string> stationNames;

  for (unsigned int i = 0; i < nlStations->getLength(); i++) {
    DOMNode* currentNode = nlStations->item(i);
    if (currentNode->getNodeType() &&
        currentNode->getNodeType() == DOMNode::ELEMENT_NODE) {
      // Found node which is an Element. Re-cast node as element
      DOMElement* currentElement =
          dynamic_cast<xercesc::DOMElement*>(currentNode);

      char* name = XMLString::transcode(currentElement->getAttribute(TAG_name));

      std::string location = "";
      if (currentElement->getAttribute(TAG_location)) {
        location =
            XMLString::transcode(currentElement->getAttribute(TAG_location));
      }

      if (CHANGE_NAME.compare(name) == 0) {
        ostringstream errBuf;
        errBuf << "Invalid station name: " << name;
        throw runtime_error(errBuf.str());
      }
      if (_stations.find(name) == _stations.end()) {
        _stations[name] = new Station(name, location);
      }

      XMLString::release(&name);
    }
  }
}

void XMLReader::setLineStations(DOMElement* eLine, Line* l) {
  DOMNodeList* nlStations = eLine->getElementsByTagName(TAG_station);

  set<string> stationNames;

  for (unsigned int i = 0; i < nlStations->getLength(); i++) {
    DOMNode* currentNode = nlStations->item(i);
    if (currentNode->getNodeType() &&
        currentNode->getNodeType() == DOMNode::ELEMENT_NODE) {
      // Found node which is an Element. Re-cast node as element
      DOMElement* currentElement =
          dynamic_cast<xercesc::DOMElement*>(currentNode);

      char* name = XMLString::transcode(currentElement->getAttribute(TAG_name));

      l->stations.push_back(name);
      map<string, Station*>::iterator pos = _stations.find(name);

      if (pos == _stations.end()) {
        ostringstream errBuf;
        errBuf << "Station " << name << " is not in station list";
        throw runtime_error(errBuf.str());
      } else {
        pos->second->lines.insert(l->name);
      }

      XMLString::release(&name);

      char* timeStr =
          XMLString::transcode(currentElement->getAttribute(TAG_time));

      try {
        uint time = boost::lexical_cast<unsigned int>(timeStr);
        l->times.push_back(time);
      } catch (boost::bad_lexical_cast const&) {
        ostringstream errBuf;
        errBuf << "Station " << name << " has no valid travel time";
        throw runtime_error(errBuf.str());
      }

      XMLString::release(&timeStr);
    }
  }
}

void XMLReader::extractLines(DOMElement* eStations) {
  DOMNodeList* nlStations = eStations->getElementsByTagName(TAG_line);

  for (unsigned int i = 0; i < nlStations->getLength(); i++) {
    DOMNode* currentNode = nlStations->item(i);
    if (currentNode->getNodeType() &&
        currentNode->getNodeType() == DOMNode::ELEMENT_NODE) {
      // Found node which is an Element. Re-cast node as element
      DOMElement* currentElement =
          dynamic_cast<xercesc::DOMElement*>(currentNode);

      char* name = XMLString::transcode(currentElement->getAttribute(TAG_name));

      if (CHANGE_NAME.compare(name) == 0) {
        ostringstream errBuf;
        errBuf << "Invalid line name: " << name;
        throw runtime_error(errBuf.str());
      }
      if (_lines.find(name) != _lines.end()) {
        ostringstream errBuf;
        errBuf << "Line " << name << " is defined twice";
        throw runtime_error(errBuf.str());
      }

      Line* currentLine = _lines[name] = new Line(name);
      setLineStations(currentElement, currentLine);

      XMLString::release(&name);
    }
  }
}

void XMLReader::readTransportFile(const std::string& xmlFile) {
  // Test to see if the file is ok.

  struct stat fileStatus;

  int iretStat = stat(xmlFile.c_str(), &fileStatus);
  if (iretStat != 0) throw(runtime_error("Cannot open file"));

  _parser->setValidationScheme(XercesDOMParser::Val_Always);
  _parser->setDoNamespaces(true);  // optional

  ErrorHandler* errHandler = dynamic_cast<ErrorHandler*>(new HandlerBase());
  _parser->setErrorHandler(errHandler);

  try {
    _parser->parse(xmlFile.c_str());
  } catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    ostringstream errBuf;
    errBuf << "Xerces Exception: " << message;
    XMLString::release(&message);
    // now we can throw this message
    throw runtime_error(errBuf.str());
  } catch (const DOMException& toCatch) {
    char* message = XMLString::transcode(toCatch.msg);
    ostringstream errBuf;
    errBuf << "Xerces Exception: " << message;
    XMLString::release(&message);
    // now we can throw this message
    throw runtime_error(errBuf.str());
  } catch (...) {
    throw runtime_error("Xerces Exception: Unexpected Exception");
  }

  DOMDocument* xmlDoc = _parser->getDocument();
  assert(xmlDoc);

  DOMElement* eRoot = xmlDoc->getDocumentElement();
  if (!eRoot) {
    throw runtime_error("Empty XML Document");
  }

  DOMNodeList* nlChilds = eRoot->getChildNodes();
  if (!nlChilds || nlChilds->getLength() == 0) {
    throw runtime_error("Invalid XML Document");
  }

  for (unsigned int i = 0; i < nlChilds->getLength(); i++) {
    DOMNode* currentNode = nlChilds->item(i);
    if (currentNode->getNodeType() &&
        currentNode->getNodeType() == DOMNode::ELEMENT_NODE) {
      // Found node which is an Element. Re-cast node as element
      DOMElement* currentElement =
          dynamic_cast<xercesc::DOMElement*>(currentNode);

      if (XMLString::equals(currentElement->getTagName(), TAG_stations)) {
        extractStations(currentElement);
      } else if (XMLString::equals(currentElement->getTagName(), TAG_lines)) {
        extractLines(currentElement);
      }
    }
  }

  for (map<string, Station*>::iterator it = _stations.begin();
       it != _stations.end(); ++it) {
    if (it->second->lines.empty()) {
      ostringstream errBuf;
      errBuf << "Station " << it->second->name << " is not visited by any line";
      throw runtime_error(errBuf.str());
    }
  }

  delete errHandler;
}

void XMLReader::printStatistic(ostream& O) const {
  int connecting = 0;
  for (map<string, Station*>::const_iterator it = _stations.begin();
       it != _stations.end(); ++it) {
    if (it->second->lines.size() > 1) connecting++;
  }

  O << "Network statistics:" << endl;
  O << " Number of stations: " << _stations.size() << endl;
  O << " Number of connecting stations: " << connecting << endl;
  O << " Number of lines: " << _lines.size() << endl;
  O << " Lines:" << endl;

  for (map<string, Line*>::const_iterator it = _lines.begin();
       it != _lines.end(); ++it) {
    O << "  Line " << it->second->name << " has " << it->second->stations.size()
      << " stations" << endl;
  }
}
