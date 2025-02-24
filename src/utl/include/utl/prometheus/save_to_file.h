// MIT License

// Copyright (c) 2021 biaks (ianiskr@gmail.com)

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include "registry.h"
#include "text_serializer.h"

namespace utl {
class SaveToFile
{
  std::chrono::seconds period{1};
  std::string filename;
  std::thread worker_thread;
  std::shared_ptr<PrometheusRegistry> registry_ptr{nullptr};
  bool must_die{false};

  void save_data()
  {
    if (registry_ptr) {
      std::fstream out_file_stream;
      out_file_stream.open(filename, std::fstream::out | std::fstream::binary);
      if (out_file_stream.is_open()) {
        TextSerializer::Serialize(out_file_stream, registry_ptr->Collect());
        out_file_stream.close();
      }
    }
  }

  void worker_function()
  {
    // it need for fast shutdown this thread when SaveToFile destructor is
    // called
    const uint64_t divider = 100;
    uint64_t fraction = divider;
    for (;;) {
      std::chrono::milliseconds period_ms
          = std::chrono::duration_cast<std::chrono::milliseconds>(period);
      std::this_thread::sleep_for(period_ms / divider);
      if (must_die) {
        save_data();
        return;
      }
      if (--fraction == 0) {
        fraction = divider;
        save_data();
      }
    }
  }

 public:
  SaveToFile() = default;

  ~SaveToFile()
  {
    must_die = true;
    worker_thread.join();
  }

  SaveToFile(std::shared_ptr<PrometheusRegistry>& registry_,
             const std::chrono::seconds& period_,
             const std::string& filename_)
  {
    set_registry(registry_);
    set_delay(period_);
    set_out_file(filename_);
  }

  void set_delay(const std::chrono::seconds& new_period)
  {
    period = new_period;
  }

  bool set_out_file(const std::string& filename_)
  {
    filename = filename_;
    std::fstream out_file_stream;
    out_file_stream.open(filename, std::fstream::out | std::fstream::binary);
    bool open_success = out_file_stream.is_open();
    out_file_stream.close();
    return open_success;
  }

  void set_registry(std::shared_ptr<PrometheusRegistry>& new_registry_ptr)
  {
    registry_ptr = new_registry_ptr;
  }
};
}  // namespace utl
