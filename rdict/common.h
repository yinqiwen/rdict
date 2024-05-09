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
#include <cstring>
#include <string_view>
#include <vector>

namespace rdict {

namespace detail {

enum DictType {
  DICT_KV = 0,
  DICT_LIST,
  DICT_KKV,
};

struct RdictMetaHeader {
  uint64_t index_size = 0;
  uint64_t data_size = 0;
  uint16_t data_pad_size = 0;
  uint16_t magic = 0xD1C7;
  uint8_t type = 0;
};

constexpr size_t kRdictMetaHeaderSize = 64 * sizeof(uint64_t);

struct nonesuch {};

template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
  using value_t = std::false_type;
  using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
  using value_t = std::true_type;
  using type = Op<Args...>;
};

struct LenHeader {
  uint32_t capacity;
  uint32_t size;
};

template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<detail::nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template <typename T>
using detect_avalanching = typename T::is_avalanching;
template <typename T>
struct Serializer {
  static void Pack(const T& v, std::vector<uint8_t>& buffer, bool align) {
    size_t orig_len = buffer.size();
    uint32_t val_size = sizeof(T);
    if (align) {
      val_size = ((val_size + 7) & ~7);
    }
    buffer.resize(buffer.size() + val_size);
    memcpy(&buffer[0] + orig_len, &v, sizeof(T));
  }
  static size_t Unpack(const uint8_t* data, T& v, bool align) {
    memcpy(&v, data, sizeof(T));
    uint32_t val_size = sizeof(T);
    if (align) {
      val_size = ((val_size + 7) & ~7);
    }
    return val_size;
  }
  static size_t GetSize(const uint8_t* data, bool align) {
    uint32_t val_size = sizeof(T);
    if (align) {
      val_size = ((val_size + 7) & ~7);
    }
    return val_size;
  }
};

template <>
struct Serializer<std::string_view> {
  static void Pack(const std::string_view& v, std::vector<uint8_t>& buffer, bool align) {
    uint32_t v_len = static_cast<uint32_t>(v.size());
    size_t orig_len = buffer.size();
    uint32_t val_size = sizeof(LenHeader) + v.size();
    if (align) {
      val_size = ((val_size + 7) & ~7);
    }
    uint32_t capacity = val_size - sizeof(LenHeader);
    buffer.resize(buffer.size() + val_size);
    LenHeader header{capacity, v_len};
    memcpy(&buffer[0] + orig_len, &header, sizeof(LenHeader));
    memcpy(&buffer[0] + orig_len + sizeof(LenHeader), v.data(), v.size());
  }
  static size_t Unpack(const uint8_t* data, std::string_view& v, bool align) {
    LenHeader header;
    memcpy(&header, data, sizeof(LenHeader));
    std::string_view view(reinterpret_cast<const char*>(data + sizeof(LenHeader)), header.size);
    v = view;

    return sizeof(LenHeader) + header.capacity;
  }
  static size_t GetSize(const uint8_t* data, bool align) {
    LenHeader header;
    memcpy(&header, data, sizeof(LenHeader));
    return sizeof(LenHeader) + header.capacity;
  }
};
}  // namespace detail
}  // namespace rdict