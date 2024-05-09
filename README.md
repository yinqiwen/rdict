# fbs-dict

fbs-dict(Flatbuffers Dict) 目的在于构建一个高性能的和语言无关的基于flatbuffers只读dict索引，适用于多种应用场景；


# 使用场景


## 搜广推后台

 典型的使用场景如下：    
 

 - 离线（spark/shell）构建词典文件
 - 推送词典文件到在线服务
 - 在线服务reload词典文件
 - 在线服务代码查询使用dict

## 设备终端

使用方式与上类似

## 数据统计与服务

数据提供者将计算后数据以fbs-dict格式导出,即可通过标准流程提供服务


# 特性


## 零解析开销

load文件不需要任何解析步骤，开销主要在从file到内存的IO；     
如若在支持mmap环境下使用，IO开销也可忽略；

## 零内存分配与零拷贝

查询读取不需要任何数据拷贝和内存分配；       
对比常规的嵌入式kv存储都需要将key/value从存储服务中拷贝出来，过程中还存在至少1次内存分配与释放

## 内存紧凑
数据主要存储在格式为连续的flatbuffers数组中，几乎和原始数据相等的空间占用；    

## 多语言支持
基本上flatbuffers支持的语言都可支持(C++/golang/java/rust/...)   

## dict格式
- HashMap, 
- List,
- KKV,类似redis的hash (TODO)


# 使用步骤
## 定义flatbuffers schema
```protobuf
namespace test.rdict;

table DictEntry {
  name:string(key);
  mana:short = 150;
  hp:short = 100;
  id:long;
}

root_type DictEntry;
```
如果Root Table的字段力有`key`属性，则表明需要构建kv类型dict； 否则则是构建`list`数组类型dict

## 离线构建二进制文件
```sh
./rdict_builder -i <json file path>  -s <flatbuffers schema file path> -o <output dict file path>
```
这里的input也支持stdin以管道方式执行:    
```sh
cat <json file path> | ./rdict_builder -i stdin  -s <flatbuffers schema file path> -o <output dict file path>
```

`rdict_builder`定义在当前lib代码库中，使用前需要单独编译；  


## 服务加载
### C++

```cpp
#include "rdict/fbs_kv.h"

auto dict_result = rdict::FbsKv<std::string, ::test::rdict::DictEntry>::Load("./fbs_dict_file");
if(!dict_result.ok()){
    // load failed
}
auto dict = std::move(dict_result.value());
auto val_result = dict->Get("key");
if(!val_result.ok()){
    // get failed
}
const ::test::rdict::DictEntry* val = val_result.value();
```
注意`rdict::FbsKv`是一个模板类， 其中第一个类型参数需要和schema中定义的`key`类型一致，第二个类型则是schema中定义的root table类型：   
flatbuffers schema中定义的key字段的类型和c++中类型映射如下, 只支持以下类型定义为key字段：     
```sh
string -> std::string_view
ulong  -> uint64_t
long   -> int64_t
int    -> int32_t
uint   -> uint32_t
```

### Go(TODO)


## 与CMOD的对比
CMOD已知的问题：
- 内存占用偏大（malloc本身 + hashmap实现放大）
- 构建端与使用端若boost版本不一致，存在数据无法读出的可能
- 只支持C++服务
- schema更改无向前向后兼容能力（修改后的schema无法读旧的构建数据； 旧的schema也无法读新schema构建数据）
- 若key为string类型， 查询时还存在一次string对象分配拷贝（fbs-dict无）

fbs-dict限制：
- 只支持kv/list两种类型
- kv格式下， key只能为string/int等几种类型

fbs-dict优势：
- 内存占用小
- 与boost无关
- 支持golang
- 基于flatbuffers，支持前后兼容性


