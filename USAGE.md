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

## 使用示例 (main.c)

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

## 选项

| 选项 | 默认 | 说明 |
|------|------|------|
| `FSSPEC_BUILD_TESTS` | ON | 构建测试 |
| `FSSPEC_BUILD_SHARED` | ON | 构建动态库 (OFF 则静态库) |
