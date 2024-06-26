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
#include "flatbuffers/flatbuffers.h"
#include "rdict/list.h"

namespace rdict {
template <typename FBS>
class FbsList : public ReadonlyList {
 public:
  using fbs_type = FBS;
  static absl::StatusOr<std::unique_ptr<FbsList>> Load(const std::string& path, size_t reserved_space_bytes = 0) {
    std::unique_ptr<FbsList> p(new FbsList);
    ReadonlyList::Options opts;
    opts.readonly = true;
    opts.path = path;
    opts.reserved_space_bytes = reserved_space_bytes;
    auto status = p->Init(opts);
    if (!status.ok()) {
      return status;
    }
    return p;
  }

  absl::StatusOr<const FBS*> Get(size_t idx) const {
    auto val = ReadonlyList::Get(idx);
    if (!val.ok()) {
      return val.status();
    }
    std::string_view value = val.value();
    return flatbuffers::GetRoot<FBS>(value.data());
  }

 private:
  FbsList() {}
};
}  // namespace rdict