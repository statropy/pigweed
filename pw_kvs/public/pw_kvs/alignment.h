// Copyright 2020 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#pragma once

#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <utility>

#include "pw_kvs/output.h"
#include "pw_span/span.h"
#include "pw_status/status_with_size.h"

namespace pw {

// Returns the value rounded down to the nearest multiple of alignment.
constexpr size_t AlignDown(size_t value, size_t alignment) {
  return (value / alignment) * alignment;
}

// Returns the value rounded up to the nearest multiple of alignment.
constexpr size_t AlignUp(size_t value, size_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

// Returns the number of padding bytes required to align the provided length.
constexpr size_t Padding(size_t length, size_t alignment) {
  return AlignUp(length, alignment) - length;
}

// Class for managing aligned writes. Stores data in an intermediate buffer and
// calls an output function with aligned data as the buffer becomes full. Any
// bytes remaining in the buffer are written to the output when Flush() is
// called or the AlignedWriter goes out of scope.
class AlignedWriter {
 public:
  AlignedWriter(span<std::byte> buffer, size_t alignment_bytes, Output& writer)
      : buffer_(buffer.data()),
        write_size_(AlignDown(buffer.size(), alignment_bytes)),
        alignment_bytes_(alignment_bytes),
        output_(writer),
        bytes_written_(0),
        bytes_in_buffer_(0) {
    // TODO(hepler): Add DCHECK to ensure that buffer.size() >= alignment_bytes.
  }

  ~AlignedWriter() { Flush(); }

  // Writes bytes to the AlignedWriter. The output may be called if the internal
  // buffer is filled.
  Status Write(span<const std::byte> data);

  Status Write(const void* data, size_t size) {
    return Write(span(static_cast<const std::byte*>(data), size));
  }

  // Flush and reset the AlignedWriter. Any remaining bytes in the buffer are
  // zero-padded to an alignment boundary and written to the output. Flush is
  // automatically called when the AlignedWriter goes out of scope.
  StatusWithSize Flush();

 private:
  std::byte* const buffer_;
  const size_t write_size_;
  const size_t alignment_bytes_;

  Output& output_;
  size_t bytes_written_;
  size_t bytes_in_buffer_;
};

// Declares an AlignedWriter with a built-in buffer.
template <size_t kBufferSize>
class AlignedWriterBuffer : public AlignedWriter {
 public:
  template <typename... Args>
  AlignedWriterBuffer(Args&&... aligned_writer_args)
      : AlignedWriter(buffer_, std::forward<Args>(aligned_writer_args)...) {}

 private:
  std::byte buffer_[kBufferSize];
};

// Writes data from multiple buffers using an AlignedWriter.
template <size_t kBufferSize>
StatusWithSize AlignedWrite(Output& output,
                            size_t alignment_bytes,
                            span<const span<const std::byte>> data) {
  AlignedWriterBuffer<kBufferSize> buffer(alignment_bytes, output);

  for (const span<const std::byte>& chunk : data) {
    if (Status status = buffer.Write(chunk); !status.ok()) {
      return StatusWithSize(status);
    }
  }

  return buffer.Flush();
}

// Calls AlignedWrite with an initializer list.
template <size_t kBufferSize>
StatusWithSize AlignedWrite(Output& output,
                            size_t alignment_bytes,
                            std::initializer_list<span<const std::byte>> data) {
  return AlignedWrite<kBufferSize>(
      output, alignment_bytes, span(data.begin(), data.size()));
}

}  // namespace pw