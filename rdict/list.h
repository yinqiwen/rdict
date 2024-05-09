/*
** BSD 3-Clause License
**
** Copyright (c) 2023, qiyingwang <qiyingwang@tencent.com>, the respective contributors, as shown by the AUTHORS file.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** * Redistributions of source code must retain the above copyright notice, this
** list of conditions and the following disclaimer.
**
** * Redistributions in binary form must reproduce the above copyright notice,
** this list of conditions and the following disclaimer in the documentation
** and/or other materials provided with the distribution.
**
** * Neither the name of the copyright holder nor the names of its
** contributors may be used to endorse or promote products derived from
** this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include "rdict/common.h"
#include "rdict/mmap_file.h"

namespace rdict {
class ReadonlyList {
 public:
  struct Options {
    std::string path;
    size_t reserved_space_bytes = 100 * 1024 * 1024 * 1024LL;
    bool readonly = false;
  };
  static absl::StatusOr<std::unique_ptr<ReadonlyList>> New(const Options& opt);
  absl::Status Add(std::string_view s);
  size_t Size() const;
  absl::StatusOr<std::string_view> Get(size_t idx) const;
  absl::Status Commit();

 protected:
  static constexpr uint32_t k_meta_reserved_space = 64;
  struct IndexMeta {
    size_t size = 0;
    size_t capcity = 0;
    size_t offset_32bits_num = 0;
  };
  ReadonlyList() {}
  absl::Status LoadIndex(bool ignore_nonexist);
  absl::Status Init(const Options& opt);
  uint64_t GetOffset(size_t idx) const;

  Options opt_;
  IndexMeta* meta_ = nullptr;
  uint32_t* offsets_ = nullptr;
  std::vector<uint8_t> index_buffer_;
  std::unique_ptr<MmapFile> data_mmap_file_;
  std::vector<uint8_t> rdict_header_buffer_;
  detail::RdictMetaHeader* header_ = nullptr;
};

}  // namespace rdict