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
#include <string>
#include <string_view>
// #include "flatbuffers/idl.h"
#include "folly/Demangle.h"
#include "folly/FileUtil.h"
#include "rdict/fbs_kv.h"
#include "rdict/fbs_list.h"
#include "rdict/tests/monster_generated.h"
#include "rdict/tests/simple_kv_generated.h"
#include "rdict/tests/simple_list_generated.h"

int main() {
  auto result = rdict::FbsKv<std::string_view, test::rdict::DictEntry>::Load("./simple_kv.rdict");
  if (!result.ok()) {
    auto status = result.status();
    printf("invalid:%s\n", status.ToString().c_str());
    return -1;
  }
  auto dict = std::move(result.value());
  printf("dict size:%lld\n", dict->Size());

  for (int i = 1; i <= 10; i++) {
    std::string name = std::to_string(i);
    auto val_result = dict->Get(name);
    const auto* entry = val_result.value();
    printf("kv entry name:%s, id:%lld  mana:%d, hp:%d\n", name.c_str(), entry->id(), entry->mana(), entry->hp());
  }
  printf("\n");

  auto result1 = rdict::FbsList<test::rdict::ListEntry>::Load("./simple_list.rdict");
  if (!result1.ok()) {
    auto status = result1.status();
    printf("invalid:%s\n", status.ToString().c_str());
    return -1;
  }
  auto list = std::move(result1.value());
  printf("list size:%lld\n", list->Size());

  for (size_t i = 0; i < list->Size(); i++) {
    auto val_result = list->Get(i);
    const auto* entry = val_result.value();
    printf("list entry name:%s, id:%lld  mana:%d, hp:%d\n", entry->name()->c_str(), entry->id(), entry->mana(),
           entry->hp());
  }

  printf("###%s\n", folly::demangle(typeid(rdict::FbsList<test::rdict::ListEntry>::fbs_type)).c_str());
  return 0;
}