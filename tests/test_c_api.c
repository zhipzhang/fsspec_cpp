// test_c_api.c - C API 测试示例
#include "fsspec_cpp/fsspec_c.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    printf("=== fsspec C API Test ===\n");

    // 初始化
    if (fsspec_init() != 0) {
        fprintf(stderr, "Failed to init: %s\n", fsspec_last_error());
        return 1;
    }
    printf("✓ Initialized\n");

    // 创建测试文件
    const char* test_url = "file:///tmp/fsspec_c_test.txt";

    // 测试 1: 直接打开写入
    fsspec_file_t* f = fsspec_open(test_url, "w");
    if (!f) {
        fprintf(stderr, "Failed to open for write: %s\n", fsspec_last_error());
        return 1;
    }

    const char* data = "Hello from fsspec C API!\nLine 2\nLine 3";
    size_t written = fsspec_file_write(f, data, strlen(data));
    printf("✓ Written %zu bytes\n", written);

    fsspec_file_close(f);
    printf("✓ File closed\n");

    // 测试 2: fsspec_stat 获取文件信息
    printf("\n=== fsspec_stat test ===\n");
    fsspec_stat_t fst;
    if (fsspec_stat(test_url, &fst) == 0) {
        printf("✓ stat succeeded\n");
        printf("  Path:     %s\n", fst.path);
        printf("  Name:     %s\n", fst.name);
        printf("  Size:     %ld bytes\n", (long)fst.size);
        printf("  Is dir:   %s\n", fst.is_dir ? "yes" : "no");
        printf("  Modified: %.1f\n", fst.mtime);
        printf("  Protocol: %s\n", fst.protocol);
    } else {
        fprintf(stderr, "✗ stat failed: %s\n", fsspec_last_error());
    }

#ifdef __linux__
    // 测试 3: POSIX stat 兼容
    printf("\n=== POSIX stat test ===\n");
    struct stat st;
    if (fsspec_posix_stat(test_url, &st) == 0) {
        printf("✓ POSIX stat succeeded\n");
        printf("  st_size:  %ld bytes\n", (long)st.st_size);
        printf("  st_mode:  0%o (%s)\n", st.st_mode & 0777,
               S_ISDIR(st.st_mode) ? "directory" : "regular file");
        printf("  st_mtime: %ld\n", (long)st.st_mtime);
        printf("  st_uid:   %d\n", st.st_uid);
        printf("  st_gid:   %d\n", st.st_gid);
        printf("  st_nlink: %lu\n", (unsigned long)st.st_nlink);
    } else {
        fprintf(stderr, "✗ POSIX stat failed: %s\n", fsspec_last_error());
    }
#endif

    // 测试 4: 重新打开读取
    printf("\n=== Read test ===\n");
    f = fsspec_open(test_url, "r");
    if (!f) {
        fprintf(stderr, "Failed to open for read: %s\n", fsspec_last_error());
        return 1;
    }

    char buf[1024];
    size_t total = 0;
    size_t n;
    printf("--- Content ---\n");
    while ((n = fsspec_file_read(f, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, n, stdout);
        total += n;
    }
    printf("\n---------------\n");
    printf("✓ Read %zu bytes total\n", total);

    // 测试 seek/tell
    fsspec_file_seek(f, 0, 0);  // SEEK_SET
    int64_t pos = fsspec_file_tell(f);
    printf("✓ Seek to beginning, pos=%ld\n", (long)pos);

    fsspec_file_close(f);

// 测试 5: FILE* 包装 (Linux only)
#ifdef __linux__
    printf("\n=== FILE* wrapper test (GNU/Linux) ===\n");
    FILE* fp = fsspec_fopen(test_url, "r");
    if (!fp) {
        fprintf(stderr, "Failed to fsspec_fopen\n");
        return 1;
    }

    printf("✓ Opened as FILE*\n");

    // 使用标准 C I/O
    char line[256];
    printf("\n--- Lines via FILE* ---\n");
    while (fgets(line, sizeof(line), fp)) {
        printf("Line: %s", line);
    }
    printf("\n-----------------------\n");

    // 测试 fseek/ftell
    fseek(fp, 0, SEEK_SET);
    long fpos = ftell(fp);
    printf("✓ fseek/ftell works, pos=%ld\n", fpos);

    // 测试 fread
    char fbuf[100];
    size_t fn = fread(fbuf, 1, sizeof(fbuf) - 1, fp);
    fbuf[fn] = '\0';
    printf("✓ fread read %zu bytes: '%.20s...'\n", fn, fbuf);

    fclose(fp);
    printf("✓ FILE* closed\n");
#endif

    // 清理
    fsspec_cleanup();
    printf("\n=== All tests passed! ===\n");

    return 0;
}
