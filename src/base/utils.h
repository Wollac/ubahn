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

#ifndef UBAHN_BASE_UTILS_H_
#define UBAHN_BASE_UTILS_H_

#include <algorithm>

template <class Container>
auto getFirstElement(const Container& container)
    -> decltype(*(container.begin())) {
  return *(container.begin());
}

template <class Container, typename T>
bool contains(const Container& container, const T& val) {
  return std::find(container.begin(), container.end(), val) != container.end();
}

#endif  // UBAHN_BASE_UTILS_H_
