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
#include <gtest/gtest.h>
#include <string_view>
#include "rdict/list.h"

TEST(Rdict, simple_strs) {
  uint64_t test_count = 1000000;
  rdict::ReadonlyList::Options opts;
  opts.readonly = false;
  opts.path = "./test_list_rdict";
  auto result = rdict::ReadonlyList::New(opts);
  auto dict = std::move(result.value());
  std::string data = "hello,world";
  for (uint64_t i = 1; i < test_count; i++) {
    std::string add_content = data + std::to_string(i);
    dict->Add(add_content);
  }
  dict->Commit();
  dict.reset();

  opts.readonly = true;
  auto result1 = rdict::ReadonlyList::New(opts);
  auto dict1 = std::move(result1.value());
  for (uint64_t i = 0; i < test_count - 1; i++) {
    auto val = dict1->Get(i);
    std::string add_content = data + std::to_string(i + 1);
    ASSERT_EQ(val.value(), add_content);
  }
  ASSERT_EQ(dict1->Size(), test_count - 1);
}