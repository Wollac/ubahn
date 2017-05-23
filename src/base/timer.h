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

#ifndef UBAHN_BASE_TIMER_H_
#define UBAHN_BASE_TIMER_H_

#include <chrono>
#include <type_traits>

/** A very simple stop watch using c++11 chrono. */
class Timer {
  typedef
      typename std::conditional<std::chrono::high_resolution_clock::is_steady,
                                std::chrono::high_resolution_clock,
                                std::chrono::steady_clock>::type clock_type;

 public:
  explicit Timer(bool run = true) {
    Reset();
    if (run) {
      Start();
    }
  }

  Timer(Timer&&) = default;

  // disallow copy and assign
  Timer(const Timer&) = delete;
  void operator=(Timer) = delete;

  void Reset() {
    _is_running = false;
    _offset = clock_type::duration(0);
  }

  void Start() {
    if (!_is_running) {
      _is_running = true;
      _start_time = clock_type::now();
    }
  }

  void Stop() {
    if (_is_running) {
      _offset += (clock_type::now() - _start_time);
      _is_running = false;
    }
  }

  bool is_running() const { return _is_running; }

  clock_type::duration Elapsed() const {
    return _offset + (clock_type::now() - _start_time);
  }

  template <typename D>
  D Elapsed() const {
    return std::chrono::duration_cast<D>(Elapsed());
  }

  template <typename T, typename Traits>
  friend std::basic_ostream<T, Traits>& operator<<(
      std::basic_ostream<T, Traits>& out, const Timer& timer) {
    return out << timer.Elapsed<std::chrono::milliseconds>().count();
  }

 private:
  bool _is_running;
  clock_type::time_point _start_time;
  clock_type::duration _offset;
};

#endif  // UBAHN_BASE_TIMER_H_
