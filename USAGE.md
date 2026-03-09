# 在父项目中使用 fsspec-c

## 方法 1: add_subdirectory

```cmake
# 父 CMakeLists.txt
cmake_minimum_required(VERSION 3.18)
project(my_project)

# 添加 fsspec-c 子目录
add_subdirectory(third_party/fsspec_cpp)

# 你的可执行文件
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE fsspec::fsspec_c)
```

## 方法 2: find_package (安装后)

```bash
# 安装 fsspec-c
cd fsspec_cpp && mkdir build && cd build
cmake .. -DFSSPEC_BUILD_TESTS=OFF
make && sudo make install
```

```cmake
# 父 CMakeLists.txt
find_package(fsspec_c REQUIRED)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE fsspec::fsspec_c)
```

## 使用示例

### 示例 1: C API

```c
#include <fsspec_cpp/fsspec_c.h>
#include <stdio.h>

int main() {
    fsspec_init();
    
    // 写入
    fsspec_file_t* f = fsspec_open("file:///tmp/test.txt", "w");
    fsspec_file_write(f, "Hello", 5);
    fsspec_file_close(f);
    
    // 读取
    f = fsspec_open("file:///tmp/test.txt", "r");
    char buf[128];
    size_t n = fsspec_file_read(f, buf, sizeof(buf));
    printf("Read: %.*s\n", (int)n, buf);
    fsspec_file_close(f);
    
    fsspec_cleanup();
    return 0;
}
```

### 示例 2: 标准 FILE* 接口 (Linux only)

使用 `fsspec_fopen()` 获得标准 `FILE*`，可用任何标准 C I/O 函数：

```c
#include <fsspec_cpp/fsspec_c.h>
#include <stdio.h>

int main() {
    fsspec_init();
    
    // 使用标准 FILE* 接口 - 支持任何协议 (s3://, gs://, etc.)
    FILE* fp = fsspec_fopen("s3://bucket/data.txt", "r");
    if (!fp) {
        perror("fopen failed");
        return 1;
    }
    
    // 使用标准 C 函数
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    
    // 或者二进制读写
    fseek(fp, 0, SEEK_SET);
    char buf[1024];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    printf("Read %zu bytes\n", n);
    
    fclose(fp);
    fsspec_cleanup();
    return 0;
}
```

**支持的函数：** `fread`, `fwrite`, `fgets`, `fgetc`, `fseek`, `ftell`, `rewind`, `fclose`

**限制：** 仅 GNU/Linux (使用 `fopencookie`)

## 选项

| 选项 | 默认 | 说明 |
|------|------|------|
| `FSSPEC_BUILD_TESTS` | ON | 构建测试 |
| `FSSPEC_BUILD_SHARED` | ON | 构建动态库 (OFF 则静态库) |
