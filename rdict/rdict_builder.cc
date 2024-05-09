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

#include <getopt.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string>
#include "rdict/fbs_builder.h"

static void help() {
  printf("Usage:\n");
  printf("--input(-i)      <input json data file>\n");
  printf("--output(-o)      <output data file>\n");
  printf("--schema(-s)      <schema file path>\n");
  printf("--reserve(-r)    <reserve build dataset size GB>\n");
}

int main(int argc, char** argv) {
  int c;
  std::string reserver_size_str;
  std::string src_file;
  std::string fbs_schema_path;
  std::string output_path;
  struct option long_options[] = {/* These options set a flag. */
                                  {"input", required_argument, 0, 'i'},  {"output", required_argument, 0, 'o'},
                                  {"schema", required_argument, 0, 's'}, {"reserve", optional_argument, 0, 'r'},
                                  {"help", no_argument, 0, 'h'},         {0, 0, 0, 0}};
  while (1) {
    /* getopt_long stores the option index here. */
    int option_index = 0;
    c = getopt_long(argc, argv, "hi:o:s:r:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1) break;

    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0) {
          *(long_options[option_index].flag) = atoi(optarg);
          break;
        }
        printf("option %s", long_options[option_index].name);
        if (optarg) printf(" with arg %s", optarg);
        printf("\n");
        break;
      case 'h': {
        help();
        return 0;
      }
      case 'i': {
        src_file = optarg;
        break;
      }
      case 'o': {
        output_path = optarg;
        break;
      }
      case 's': {
        fbs_schema_path = optarg;
        break;
      }
      case 'r': {
        reserver_size_str = optarg;
        break;
      }
      case '?':
        /* getopt_long already printed an error message. */
        break;

      default:
        abort();
    }
  }
  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc) printf("%s ", argv[optind++]);
    printf("\n");
    return -1;
  }
  if (src_file.empty() || fbs_schema_path.empty() || output_path.empty()) {
    help();
    return -1;
  }
  rdict::FbsDictBuilder::Options opts;
  if (!reserver_size_str.empty()) {
    int64_t v = std::stoll(reserver_size_str);
    if (v > 0) {
      opts.reserved_space_bytes = v * 1024 * 1024 * 1024;
    }
  }
  auto result = rdict::FbsDictBuilder::New(fbs_schema_path, output_path, opts);
  if (!result.ok()) {
    auto status = result.status();
    printf("schema:%s invalid:%s\n", fbs_schema_path.c_str(), status.ToString().c_str());
    return -1;
  }
  auto dict = std::move(result.value());

  std::istream* is = nullptr;
  std::ifstream file(src_file.c_str());
  if (src_file == "stdin" || src_file.empty()) {
    is = &(std::cin);
  } else {
    if (!file.is_open()) {
      printf("Open source file:%s failed.\n", src_file.c_str());
      return -1;
    }
    is = &file;
  }

  std::string line;
  while (std::getline(*is, line)) {
    if (line.empty()) {
      continue;
    }
    auto status = dict->Add(line);
    if (!status.ok()) {
      printf("Invalid line:%s with error:%s\n", line.c_str(), status.ToString().c_str());
    }
  }
  auto status = dict->Flush();
  if (!status.ok()) {
    printf("Dict flush failed error:%s\n", status.ToString().c_str());
  }
  return 0;
}