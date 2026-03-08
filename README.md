# fsspec-cpp

通过 nanobind 在 C++ 中使用 fsspec 的多后端文件系统能力。

## 特性

- 在 C++ 中透明访问 S3、GCS、Azure、本地等多种存储后端
- 利用 fsspec 成熟的生态系统，无需重复造轮子
- 基于 nanobind 的高性能 Python-C++ 绑定
- 现代 CMake 构建系统

## 依赖

- C++17 或更高
- Python >= 3.8
- nanobind
- fsspec

## 安装

```bash
pip install .
```

## 使用示例

```cpp
#include "fsspec_cpp/fs.hpp"

int main() {
    // 打开 S3 文件
    auto f = fsspec::open("s3://bucket/path/to/file.txt");
    
    // 读取内容
    std::string content = f.read();
    
    // 写入
    fsspec::open("s3://bucket/output.txt", "w")
        .write("Hello from C++!");
}
```

## 许可证

MIT
