# fsspec-c

通过 fsspec 在 C/C++ 中使用多后端文件系统能力（S3、GCS、Azure、本地等）。

## 特性

- 纯 C 实现，零 C++ 依赖
- 透明访问 S3、GCS、Azure、本地等多种存储后端
- 自动处理压缩格式（gzip、zstd、bz2、lz4）
- GNU/Linux FILE* 支持（通过 fopencookie）
- 简单易用的 C API

## 依赖

- C11 编译器
- Python >= 3.8
- fsspec
- CMake >= 3.18

可选（用于压缩支持）：
- zstandard
- lz4

## 安装

```bash
pip install fsspec

# 可选压缩支持
pip install zstandard lz4

# 构建 C 库
mkdir build && cd build
cmake ..
make
sudo make install  # 可选
```

## 使用示例

### C API

```c
#include <fsspec_cpp/fsspec_c.h>
#include <stdio.h>

int main() {
    fsspec_init();
    
    // 写入文件（支持自动压缩，根据扩展名）
    fsspec_file_t* f = fsspec_open("file:///tmp/data.txt.gz", "w");
    const char* data = "Hello, fsspec-c!";
    fsspec_file_write(f, data, strlen(data));
    fsspec_file_close(f);
    
    // 读取文件（自动解压）
    f = fsspec_open("file:///tmp/data.txt.gz", "r");
    char buf[1024];
    size_t n = fsspec_file_read(f, buf, sizeof(buf));
    printf("Read %zu bytes: %.*s\n", n, (int)n, buf);
    fsspec_file_close(f);
    
    fsspec_cleanup();
    return 0;
}
```

### FILE* 包装（GNU/Linux）

```c
#include <fsspec_cpp/fsspec_c.h>
#include <stdio.h>

int main() {
    fsspec_init();
    
    // 使用标准 FILE* 接口
    FILE* fp = fsspec_fopen("s3://bucket/file.txt", "r");
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    
    fclose(fp);
    fsspec_cleanup();
    return 0;
}
```

### C++ 包装示例

```cpp
#include <fsspec_cpp/fsspec_c.h>
#include <string>

class FsspecFile {
    fsspec_file_t* file_;
public:
    FsspecFile(const std::string& url, const char* mode) {
        file_ = fsspec_open(url.c_str(), mode);
    }
    ~FsspecFile() { if (file_) fsspec_file_close(file_); }
    
    size_t read(void* buf, size_t size) {
        return fsspec_file_read(file_, buf, size);
    }
    size_t write(const void* buf, size_t size) {
        return fsspec_file_write(file_, buf, size);
    }
};

int main() {
    fsspec_init();
    
    FsspecFile f("file:///tmp/test.txt", "w");
    f.write("Hello", 5);
    
    fsspec_cleanup();
    return 0;
}
```

## 支持的 URL 格式

| 协议 | 示例 URL | 说明 |
|------|----------|------|
| 本地文件 | `file:///path/to/file` | 本地文件系统 |
| S3 | `s3://bucket/key` | Amazon S3 |
| GCS | `gs://bucket/key` | Google Cloud Storage |
| Azure | `az://container/key` | Azure Blob Storage |
| HTTP | `http://host/file` | HTTP(S) 访问 |

## 自动压缩

根据文件扩展名自动选择压缩格式：

| 扩展名 | 格式 |
|--------|------|
| `.gz` | gzip |
| `.zst` | zstandard |
| `.bz2` | bzip2 |
| `.lz4` | lz4 |

示例：
```c
fsspec_open("file:///data.json.gz", "w");   // 自动 gzip 压缩
fsspec_open("s3://bucket/log.zst", "r");   // 自动 zstd 解压
```

## 构建测试

```bash
mkdir build && cd build
cmake ..
make -j4
ctest --output-on-failure
```

## API 参考

### 初始化/清理
- `int fsspec_init()` - 初始化 Python 解释器
- `void fsspec_cleanup()` - 清理资源

### 文件操作
- `fsspec_file_t* fsspec_open(const char* url, const char* mode)` - 打开文件
- `size_t fsspec_file_read(fsspec_file_t* f, void* buf, size_t size)` - 读取
- `size_t fsspec_file_write(fsspec_file_t* f, const void* buf, size_t size)` - 写入
- `int64_t fsspec_file_seek(fsspec_file_t* f, int64_t offset, int whence)` - 定位
- `int64_t fsspec_file_tell(fsspec_file_t* f)` - 获取当前位置
- `int fsspec_file_close(fsspec_file_t* f)` - 关闭文件

### FILE* 包装（Linux）
- `FILE* fsspec_fopen(const char* url, const char* mode)` - 打开为 FILE*

### 文件系统操作
- `fsspec_fs_t* fsspec_fs_from_url(const char* url)` - 获取文件系统
- `bool fsspec_fs_exists(fsspec_fs_t* fs, const char* path)` - 检查存在
- `int fsspec_fs_remove(fsspec_fs_t* fs, const char* path)` - 删除文件

### 文件信息
- `int fsspec_stat(const char* url, fsspec_stat_t* st)` - 获取文件信息

## 许可证

MIT
