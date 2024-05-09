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
#include "rdict/fbs_builder.h"
#include <string_view>
#include "folly/FileUtil.h"
#include "rdict/kv.h"
#include "rdict/list.h"
namespace rdict {
absl::StatusOr<std::unique_ptr<FbsDictBuilder>> FbsDictBuilder::New(const std::string& schema_path,
                                                                    const std::string& output_path,
                                                                    const Options& opts) {
  std::unique_ptr<FbsDictBuilder> p(new FbsDictBuilder);
  auto status = p->Init(schema_path, output_path, opts);
  if (!status.ok()) {
    return status;
  }
  return p;
}

template <typename T>
static absl::StatusOr<void*> new_kv_dict(const std::string& output_path, const FbsDictBuilder::Options& opts) {
  typename rdict::ReadonlyKV<T, std::string_view>::Options dict_opt;
  dict_opt.path = output_path;
  dict_opt.readonly = false;
  dict_opt.reserved_space_bytes = opts.reserved_space_bytes;
  dict_opt.bucket_count = static_cast<size_t>(opts.max_elements * 1.0 / dict_opt.max_load_factor);
  auto result = rdict::ReadonlyKV<T, std::string_view>::New(dict_opt);
  if (!result.ok()) {
    return result.status();
  }
  return result.value().release();
}

absl::Status FbsDictBuilder::Init(const std::string& schema_path, const std::string& output_path, const Options& opts) {
  std::string fbs_schema;
  if (!folly::readFile(schema_path.c_str(), fbs_schema)) {
    return absl::InvalidArgumentError("invalid flatbuffers schema path");
  }
  if (!parser_.Parse(fbs_schema.c_str())) {
    return absl::InvalidArgumentError("invalid flatbuffers schema content");
  }
  schema_parser_.Parse(fbs_schema.c_str());
  if (nullptr == parser_.root_struct_def_) {
    return absl::InvalidArgumentError("Missing 'root' table");
  }

  // printf("###name:%s\n", parser_.current_namespace_->GetFullyQualifiedName("").c_str());

  schema_parser_.Serialize();
  reflection_schema_ = reflection::GetSchema(schema_parser_.builder_.GetBufferPointer());
  auto reflection_root_table = reflection_schema_->root_table();
  auto reflection_root_fields = reflection_root_table->fields();
  for (size_t i = 0; i < reflection_root_fields->size(); i++) {
    auto field = reflection_root_fields->Get(i);
    if (field->key()) {
      key_reflection_field_ = field;
    }
  }
  if (nullptr == key_reflection_field_) {
    rdict::ReadonlyList::Options dict_opt;
    dict_opt.path = output_path;
    dict_opt.readonly = false;
    dict_opt.reserved_space_bytes = opts.reserved_space_bytes;
    auto result = rdict::ReadonlyList::New(dict_opt);
    if (!result.ok()) {
      return result.status();
    }
    dict_ = result.value().release();
  } else {
    switch (key_reflection_field_->type()->base_type()) {
      case reflection::BaseType::String: {
        auto result = new_kv_dict<std::string_view>(output_path, opts);
        if (!result.ok()) {
          return result.status();
        }
        dict_ = result.value();
        break;
      }
      case reflection::BaseType::ULong: {
        auto result = new_kv_dict<uint64_t>(output_path, opts);
        if (!result.ok()) {
          return result.status();
        }
        dict_ = result.value();
        break;
      }
      case reflection::BaseType::UInt: {
        auto result = new_kv_dict<uint32_t>(output_path, opts);
        if (!result.ok()) {
          return result.status();
        }
        dict_ = result.value();
      }
      case reflection::BaseType::Long: {
        auto result = new_kv_dict<int64_t>(output_path, opts);
        if (!result.ok()) {
          return result.status();
        }
        dict_ = result.value();
        break;
      }
      case reflection::BaseType::Int: {
        auto result = new_kv_dict<int32_t>(output_path, opts);
        if (!result.ok()) {
          return result.status();
        }
        dict_ = result.value();
        break;
      }
      default: {
        return absl::InvalidArgumentError("Unsupported 'key' field type.");
      }
    }
  }
  return absl::OkStatus();
}

absl::Status FbsDictBuilder::Add(const std::string& json) {
  if (!parser_.ParseJson(json.c_str())) {
    return absl::InvalidArgumentError("Invalid json to parse");
  }
  std::string_view content(reinterpret_cast<const char*>(parser_.builder_.GetBufferPointer()),
                           parser_.builder_.GetSize());
  auto& root = *flatbuffers::GetAnyRoot(parser_.builder_.GetBufferPointer());
  if (nullptr != key_reflection_field_) {
    switch (key_reflection_field_->type()->base_type()) {
      case reflection::BaseType::String: {
        const flatbuffers::String* field_val = flatbuffers::GetFieldS(root, *key_reflection_field_);
        std::string_view sv;
        if (nullptr != field_val) {
          sv = field_val->string_view();
        }
        rdict::ReadonlyKV<std::string_view, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<std::string_view, std::string_view>*>(dict_);
        return dict->Put(sv, content);
      }
      case reflection::BaseType::ULong: {
        uint64_t field_val = flatbuffers::GetFieldI<uint64_t>(root, *key_reflection_field_);
        rdict::ReadonlyKV<uint64_t, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<uint64_t, std::string_view>*>(dict_);
        return dict->Put(field_val, content);
      }
      case reflection::BaseType::UInt: {
        uint32_t field_val = flatbuffers::GetFieldI<uint32_t>(root, *key_reflection_field_);
        rdict::ReadonlyKV<uint32_t, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<uint32_t, std::string_view>*>(dict_);
        return dict->Put(field_val, content);
      }
      case reflection::BaseType::Long: {
        uint64_t field_val = flatbuffers::GetFieldI<uint64_t>(root, *key_reflection_field_);
        rdict::ReadonlyKV<uint64_t, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<uint64_t, std::string_view>*>(dict_);
        return dict->Put(field_val, content);
      }
      case reflection::BaseType::Int: {
        int32_t field_val = flatbuffers::GetFieldI<int32_t>(root, *key_reflection_field_);
        rdict::ReadonlyKV<int32_t, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<int32_t, std::string_view>*>(dict_);
        return dict->Put(field_val, content);
      }
      default: {
        return absl::InvalidArgumentError("Unsupported 'key' field type.");
      }
    }
  } else {
    rdict::ReadonlyList* dict = reinterpret_cast<rdict::ReadonlyList*>(dict_);
    return dict->Add(content);
  }
}

absl::Status FbsDictBuilder::Flush() {
  if (nullptr != key_reflection_field_) {
    switch (key_reflection_field_->type()->base_type()) {
      case reflection::BaseType::String: {
        rdict::ReadonlyKV<std::string_view, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<std::string_view, std::string_view>*>(dict_);
        return dict->Commit();
      }
      case reflection::BaseType::ULong: {
        rdict::ReadonlyKV<uint64_t, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<uint64_t, std::string_view>*>(dict_);
        return dict->Commit();
      }
      case reflection::BaseType::UInt: {
        rdict::ReadonlyKV<uint32_t, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<uint32_t, std::string_view>*>(dict_);
        return dict->Commit();
      }
      case reflection::BaseType::Long: {
        rdict::ReadonlyKV<uint64_t, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<uint64_t, std::string_view>*>(dict_);
        return dict->Commit();
      }
      case reflection::BaseType::Int: {
        rdict::ReadonlyKV<int32_t, std::string_view>* dict =
            reinterpret_cast<rdict::ReadonlyKV<int32_t, std::string_view>*>(dict_);
        return dict->Commit();
      }
      default: {
        return absl::InvalidArgumentError("Unsupported 'key' field type.");
      }
    }
  } else {
    rdict::ReadonlyList* dict = reinterpret_cast<rdict::ReadonlyList*>(dict_);
    return dict->Commit();
  }
  return absl::OkStatus();
}
}  // namespace rdict