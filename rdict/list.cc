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
#include "rdict/list.h"
#include <cstdint>
#include <cstring>
#include <vector>
#include "folly/File.h"
#include "folly/FileUtil.h"
#include "rdict/common.h"

namespace rdict {
absl::StatusOr<std::unique_ptr<ReadonlyList>> ReadonlyList::New(const Options& opt) {
  std::unique_ptr<ReadonlyList> p(new ReadonlyList);
  auto status = p->Init(opt);
  if (!status.ok()) {
    return status;
  }
  return p;
}

absl::Status ReadonlyList::LoadIndex(bool ignore_nonexist) {
  if (data_mmap_file_->GetWriteOffset() < detail::kRdictMetaHeaderSize) {
    return absl::InvalidArgumentError("invalid rdict file with too small length");
  }
  if (opt_.readonly) {
    header_ = reinterpret_cast<detail::RdictMetaHeader*>(data_mmap_file_->GetRawData());
    uint8_t* read_index_data =
        data_mmap_file_->GetRawData() + detail::kRdictMetaHeaderSize + header_->data_size + header_->data_pad_size;
    offsets_ = reinterpret_cast<uint32_t*>(read_index_data + k_meta_reserved_space);
    meta_ = reinterpret_cast<IndexMeta*>(read_index_data);
  } else {
    memcpy(&rdict_header_buffer_[0], data_mmap_file_->GetRawData(), detail::kRdictMetaHeaderSize);
    header_ = reinterpret_cast<detail::RdictMetaHeader*>(&rdict_header_buffer_[0]);
    index_buffer_.resize(header_->index_size);
    memcpy(&index_buffer_[0],
           data_mmap_file_->GetRawData() + detail::kRdictMetaHeaderSize + header_->data_size + header_->data_pad_size,
           header_->index_size);
    offsets_ = reinterpret_cast<uint32_t*>(&index_buffer_[k_meta_reserved_space]);
    meta_ = reinterpret_cast<IndexMeta*>(&index_buffer_[0]);
  }
  data_mmap_file_->ResetWriteOffset(detail::kRdictMetaHeaderSize + header_->data_size);
  return absl::OkStatus();
}

absl::Status ReadonlyList::Init(const Options& opt) {
  opt_ = opt;
  std::string data_path = opt.path;
  MmapFile::Options data_opts;
  data_opts.path = data_path;
  data_opts.readonly = opt.readonly;
  data_opts.reserved_space_bytes = opt.reserved_space_bytes;
  data_opts.truncate = opt.truncate;

  auto data_file_result = MmapFile::Open(data_opts);
  if (!data_file_result.ok()) {
    return data_file_result.status();
  }
  data_mmap_file_ = std::move(data_file_result.value());
  rdict_header_buffer_.resize(detail::kRdictMetaHeaderSize);
  header_ = reinterpret_cast<detail::RdictMetaHeader*>(&rdict_header_buffer_[0]);
  if (data_mmap_file_->GetWriteOffset() == 0) {
    *header_ = detail::RdictMetaHeader{};
    header_->type = detail::DictType::DICT_LIST;
    uint64_t default_capacity = 1024 * 1024;
    index_buffer_.resize(k_meta_reserved_space + default_capacity * sizeof(uint64_t));
    offsets_ = reinterpret_cast<uint32_t*>(&index_buffer_[k_meta_reserved_space]);
    meta_ = reinterpret_cast<IndexMeta*>(&index_buffer_[0]);
    meta_->capcity = default_capacity;
    meta_->size = 0;
    data_mmap_file_->ResetWriteOffset(detail::kRdictMetaHeaderSize);
    return absl::OkStatus();
  } else {
    // read exist data
    return LoadIndex(false);
  }
}

absl::Status ReadonlyList::Add(std::string_view s) {
  uint32_t allign_len = (s.size() + sizeof(uint32_t) + 7) & ~7;
  auto result = data_mmap_file_->Add(s.data(), s.size());
  if (!result.ok()) {
    return result.status();
  }
  uint64_t offset = result.value();
  std::vector<uint8_t> pad(allign_len - s.size());
  uint32_t actual_len = s.size();
  memcpy(&pad[0] + pad.size() - sizeof(uint32_t), &actual_len, sizeof(uint32_t));
  result = data_mmap_file_->Add(pad.data(), pad.size());
  if (!result.ok()) {
    return result.status();
  }

  if (meta_->size == meta_->capcity) {
    size_t current_cap = meta_->capcity;
    index_buffer_.resize(current_cap * 2 * sizeof(uint64_t) + k_meta_reserved_space);
    offsets_ = reinterpret_cast<uint32_t*>(&index_buffer_[k_meta_reserved_space]);
    meta_ = reinterpret_cast<IndexMeta*>(&index_buffer_[0]);
    meta_->capcity = current_cap * 2;
  }

  if (offset <= UINT32_MAX) {
    meta_->offset_32bits_num++;
    offsets_[meta_->size] = offset;
  } else {
    uint32_t offset_32bits_pad_num =
        meta_->offset_32bits_num % 2 == 0 ? meta_->offset_32bits_num : meta_->offset_32bits_num + 1;
    uint32_t* write_offsets = offsets_ + offset_32bits_pad_num + (meta_->size - meta_->offset_32bits_num) * 2;
    memcpy(write_offsets, &offset, sizeof(uint64_t));
  }
  meta_->size++;
  return absl::OkStatus();
}
size_t ReadonlyList::Size() const { return nullptr == meta_ ? 0 : meta_->size; }

uint64_t ReadonlyList::GetOffset(size_t idx) const {
  if (idx < meta_->offset_32bits_num) {
    return offsets_[idx];
  } else {
    uint32_t offset_32bits_pad_num =
        meta_->offset_32bits_num % 2 == 0 ? meta_->offset_32bits_num : meta_->offset_32bits_num + 1;
    const uint64_t* read_offsets =
        reinterpret_cast<const uint64_t*>(offsets_ + offset_32bits_pad_num + (idx - meta_->offset_32bits_num) * 2);
    return *read_offsets;
  }
}
absl::StatusOr<std::string_view> ReadonlyList::Get(size_t idx) const {
  if ((idx) >= meta_->size) {
    return absl::OutOfRangeError("invalid idx to get data");
  }
  uint64_t offset = GetOffset(idx);
  auto data_start = data_mmap_file_->GetRawData() + offset;
  uint64_t next_offset = data_mmap_file_->GetWriteOffset();
  if ((idx + 1) < meta_->size) {
    next_offset = GetOffset(idx + 1);
  }
  uint64_t len = next_offset - offset;
  uint32_t act_len = 0;
  memcpy(&act_len, data_start + len - sizeof(uint32_t), sizeof(uint32_t));
  if (act_len > len) {
    return absl::InternalError("corrutpted data with invalid length content");
  }
  // printf("##size:%lld,offset_32bits_num:%lld\n", meta_->size, meta_->offset_32bits_num);
  // printf("###size:%lld, len:%lld,next_offset:%lld,offset:%lld \n", len, act_len, next_offset, offset);
  return std::string_view(reinterpret_cast<const char*>(data_start), act_len);
}
absl::Status ReadonlyList::Commit() {
  uint32_t offset_32bits_pad_num =
      meta_->offset_32bits_num % 2 == 0 ? meta_->offset_32bits_num : meta_->offset_32bits_num + 1;
  size_t dump_len = k_meta_reserved_space + offset_32bits_pad_num * sizeof(uint32_t) +
                    (meta_->size - meta_->offset_32bits_num) * sizeof(uint64_t);
  meta_->capcity = meta_->size;
  index_buffer_.resize(dump_len);
  uint64_t data_len = data_mmap_file_->GetWriteOffset() - detail::kRdictMetaHeaderSize;
  uint64_t data_pad_len = (data_len + 7) & ~7;
  header_->data_size = data_len;
  header_->index_size = index_buffer_.size();
  header_->data_pad_size = data_pad_len - data_len;
  memcpy(data_mmap_file_->GetRawData(), header_, detail::kRdictMetaHeaderSize);

  // printf("###header_ data_size:%lld,dump_len:%lld\n", header_->data_size, dump_len);

  std::vector<uint8_t> data_pad(header_->data_pad_size);
  if (data_pad.size() > 0) {
    auto result = data_mmap_file_->Add(data_pad.data(), data_pad.size());
    if (!result.ok()) {
      return result.status();
    }
  }
  auto result = data_mmap_file_->Add(index_buffer_.data(), index_buffer_.size());
  if (!result.ok()) {
    return result.status();
  }
  // std::string index_path = opt_.path_prefix + ".index";
  // if (!folly::writeFile(index_buffer_, index_path.c_str())) {
  //   int err = errno;
  //   return absl::ErrnoToStatus(err, "write rdict index file failed.");
  // }
  result = data_mmap_file_->ShrinkToFit();
  return result.status();
}
}  // namespace rdict