# fsspec-cpp

通过 nanobind 在 C/C++ 中使用 fsspec 的多后端文件系统能力。

## 特性

- 在 C++ 中透明访问 S3、GCS、Azure、本地等多种存储后端
- 利用 fsspec 成熟的生态系统，无需重复造轮子
- 基于 nanobind 的高性能 Python-C++ 绑定
- 现代 CMake 构建系统
- **C API 支持** - 纯 C 接口，兼容任何 C 库
- **GNU/Linux FILE* 支持** - 通过 fopencookie 包装成标准 FILE*

## 依赖

- C++17 或更高
- C11 (C API)
- Python >= 3.8
- nanobind
- fsspec

## 安装

### Python 模块
```bash
pip install .
```

### C API 共享库
```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

## 使用示例

### C++
```cpp
#include "fsspec_cpp/fs.hpp"

int main() {
    // 打开 S3 文件
    auto f = fsspec::open("s3://bucket/path/to/file.txt");
    
    // 读取内容
    std::string content = f->read_all();
    
    // 写入
    auto out = fsspec::open("s3://bucket/output.txt", "w");
    out->write_all("Hello from C++!");
}
```

### C API
```c
#include <fsspec_cpp/fsspec_c.h>
#include <stdio.h>

int main() {
    fsspec_init();
    
    // 直接打开 URL
    fsspec_file_t* f = fsspec_open("s3://bucket/file.txt", "r");
    
    char buf[1024];
    size_t n;
    while ((n = fsspec_file_read(f, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, n, stdout);
    }
    
    fsspec_file_close(f);
    fsspec_cleanup();
}
```

### C API + FILE* (GNU/Linux only)
```c
#include <fsspec_cpp/fsspec_c.h>
#include <stdio.h>

int main() {
    fsspec_init();
    
    // 返回标准 FILE*，可用 fread/fwrite/fseek/ftell/fclose
    FILE* fp = fsspec_fopen("s3://bucket/file.txt", "r");
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    
    fclose(fp);  // 自动关闭底层 fsspec 文件
    fsspec_cleanup();
}
```

## 许可证

MIT
