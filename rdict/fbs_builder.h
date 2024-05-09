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

#include <memory>
#include "absl/status/statusor.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/reflection.h"

namespace rdict {
class FbsDictBuilder {
 public:
  struct Options {
    size_t max_elements = 0;
    size_t reserved_space_bytes = 100 * 1024 * 1024 * 1024LL;
    Options() {}
  };

  static absl::StatusOr<std::unique_ptr<FbsDictBuilder>> New(const std::string& schema_path,
                                                             const std::string& output_path,
                                                             const Options& opts = Options{});

  absl::Status Add(const std::string& json);
  absl::Status Flush();

 private:
  absl::Status Init(const std::string& schema_path, const std::string& output_path, const Options& opts);

  flatbuffers::Parser parser_;
  flatbuffers::Parser schema_parser_;
  const reflection::Schema* reflection_schema_ = nullptr;
  const reflection::Field* key_reflection_field_ = nullptr;
  void* dict_ = nullptr;
};
}  // namespace rdict