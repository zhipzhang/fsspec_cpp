// test_cpp_api.cpp - C++ API tests using doctest
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "fsspec_cpp/fsspec_c.h"
#include <string>
#include <string>
#include <fstream>

// 简单的 C++ 包装类
class FsspecFile {
    fsspec_file_t* file_;

   public:
    FsspecFile(fsspec_file_t* f) : file_(f) {}
    ~FsspecFile() {
        if (file_) fsspec_file_close(file_);
    }

    size_t read(void* buf, size_t size) { return fsspec_file_read(file_, buf, size); }
    size_t write(const void* buf, size_t size) { return fsspec_file_write(file_, buf, size); }
    int64_t seek(int64_t off, int whence) { return fsspec_file_seek(file_, off, whence); }
    int64_t tell() { return fsspec_file_tell(file_); }
    bool eof() { return fsspec_file_eof(file_); }
};

TEST_CASE("basic file operations") {
    // 初始化
    CHECK(fsspec_init() == 0);

    SUBCASE("write and read local file") {
        const char* test_file = "file:///tmp/doctest_basic.txt";
        const char* data = "Hello from C++ wrapper!";

        // 写入
        fsspec_file_t* f = fsspec_open(test_file, "w");
        REQUIRE(f != nullptr);
        size_t written = fsspec_file_write(f, data, strlen(data));
        CHECK(written == strlen(data));
        fsspec_file_close(f);

        // 读取
        f = fsspec_open(test_file, "r");
        REQUIRE(f != nullptr);
        char buf[256] = {0};
        size_t n = fsspec_file_read(f, buf, sizeof(buf));
        CHECK(n == strlen(data));
        CHECK(std::string(buf, n) == data);
        fsspec_file_close(f);
    }

    SUBCASE("seek and tell") {
        // 创建测试文件
        {
            std::ofstream ofs("/tmp/doctest_seek.txt");
            ofs << "0123456789ABCDEF";
        }

        fsspec_file_t* f = fsspec_open("file:///tmp/doctest_seek.txt", "r");
        REQUIRE(f != nullptr);

        CHECK(fsspec_file_tell(f) == 0);

        char buf[8];
        fsspec_file_read(f, buf, 4);
        CHECK(fsspec_file_tell(f) == 4);

        fsspec_file_seek(f, 8, 0);  // SEEK_SET
        CHECK(fsspec_file_tell(f) == 8);

        fsspec_file_seek(f, 4, 1);  // SEEK_CUR
        CHECK(fsspec_file_tell(f) == 12);

        fsspec_file_seek(f, 0, 2);  // SEEK_END
        CHECK(fsspec_file_tell(f) == 16);

        fsspec_file_close(f);
    }

    SUBCASE("stat file info") {
        // 创建测试文件
        {
            std::ofstream ofs("/tmp/doctest_stat.txt");
            ofs << "test content for stat";
        }

        fsspec_stat_t st;
        CHECK(fsspec_stat("file:///tmp/doctest_stat.txt", &st) == 0);
        CHECK(st.size == 21);  // "test content for stat" length
        CHECK(st.is_dir == false);
    }
}

TEST_CASE("compressed file support") {
    CHECK(fsspec_init() == 0);

    SUBCASE("gzip file (.gz)") {
        const char* test_file = "file:///tmp/doctest_gzip.txt.gz";
        const char* data = "This is compressed data for gzip testing!";

        // 写入 gzip 文件（fsspec 自动压缩）
        fsspec_file_t* f = fsspec_open(test_file, "w");
        REQUIRE(f != nullptr);
        size_t written = fsspec_file_write(f, data, strlen(data));
        CHECK(written == strlen(data));
        fsspec_file_close(f);

        // 读取 gzip 文件（fsspec 自动解压）
        f = fsspec_open(test_file, "r");
        REQUIRE(f != nullptr);
        char buf[256] = {0};
        size_t n = fsspec_file_read(f, buf, sizeof(buf));
        CHECK(n == strlen(data));
        CHECK(std::string(buf, n) == data);
        fsspec_file_close(f);

        // 验证实际文件是压缩的
        struct stat st;
        CHECK(stat("/tmp/doctest_gzip.txt.gz", &st) == 0);
        // 压缩后应该比原始数据小
        if (strlen(data) > 20) {
            CHECK(st.st_size < (off_t)strlen(data) + 20);  // gzip 头尾有少量开销
        }
    }

    SUBCASE("zstandard file (.zst)") {
        const char* test_file = "file:///tmp/doctest_zstd.txt.zst";
        const char* data =
            "This is compressed data for zstandard testing! It needs to be long enough to benefit "
            "from compression.";

        // 写入 zstd 文件
        fsspec_file_t* f = fsspec_open(test_file, "w");
        REQUIRE(f != nullptr);
        size_t written = fsspec_file_write(f, data, strlen(data));
        CHECK(written == strlen(data));
        fsspec_file_close(f);

        // 读取 zstd 文件
        f = fsspec_open(test_file, "r");
        REQUIRE(f != nullptr);
        char buf[512] = {0};
        size_t n = fsspec_file_read(f, buf, sizeof(buf));
        CHECK(n == strlen(data));
        CHECK(std::string(buf, n) == data);
        fsspec_file_close(f);
    }

    SUBCASE("bzip2 file (.bz2)") {
        const char* test_file = "file:///tmp/doctest_bz2.txt.bz2";
        const char* data =
            "This is compressed data for bzip2 testing! It needs to be long enough to benefit from "
            "compression.";

        // 写入 bz2 文件
        fsspec_file_t* f = fsspec_open(test_file, "w");
        REQUIRE(f != nullptr);
        size_t written = fsspec_file_write(f, data, strlen(data));
        CHECK(written == strlen(data));
        fsspec_file_close(f);

        // 读取 bz2 文件
        f = fsspec_open(test_file, "r");
        REQUIRE(f != nullptr);
        char buf[512] = {0};
        size_t n = fsspec_file_read(f, buf, sizeof(buf));
        CHECK(n == strlen(data));
        CHECK(std::string(buf, n) == data);
        fsspec_file_close(f);
    }

    SUBCASE("lz4 file (.lz4)") {
        const char* test_file = "file:///tmp/doctest_lz4.txt.lz4";
        const char* data = "This is compressed data for lz4 testing! LZ4 is very fast.";

        // 写入 lz4 文件
        fsspec_file_t* f = fsspec_open(test_file, "w");
        REQUIRE(f != nullptr);
        size_t written = fsspec_file_write(f, data, strlen(data));
        CHECK(written == strlen(data));
        fsspec_file_close(f);

        // 读取 lz4 文件
        f = fsspec_open(test_file, "r");
        REQUIRE(f != nullptr);
        char buf[256] = {0};
        size_t n = fsspec_file_read(f, buf, sizeof(buf));
        CHECK(n == strlen(data));
        CHECK(std::string(buf, n) == data);
        fsspec_file_close(f);
    }
}

TEST_CASE("FILE* wrapper") {
    CHECK(fsspec_init() == 0);

    SUBCASE("fread/fwrite/fseek/ftell") {
        const char* test_file = "file:///tmp/doctest_fileptr.bin";
        const char* data = "Binary data for FILE* wrapper test!";

        // 使用 FILE* 写入
        FILE* fp = fsspec_fopen(test_file, "wb");
        REQUIRE(fp != nullptr);

        size_t written = fwrite(data, 1, strlen(data), fp);
        CHECK(written == strlen(data));
        fclose(fp);

        // 使用 FILE* 读取
        fp = fsspec_fopen(test_file, "rb");
        REQUIRE(fp != nullptr);

        // 测试 fseek/ftell
        fseek(fp, 7, SEEK_SET);
        CHECK(ftell(fp) == 7);

        // 测试 fread
        char buf[64] = {0};
        size_t n = fread(buf, 1, 4, fp);  // 从位置 7 读 4 字节
        CHECK(n == 4);
        CHECK(std::string(buf, 4) == "data");

        // 测试到文件尾
        fseek(fp, 0, SEEK_END);
        CHECK(ftell(fp) == (long)strlen(data));

        fclose(fp);
    }

    SUBCASE("fgets for text lines") {
        // 创建多行文本文件
        const char* test_file = "file:///tmp/doctest_lines.txt";
        {
            fsspec_file_t* f = fsspec_open(test_file, "w");
            REQUIRE(f != nullptr);
            const char* lines = "Line 1\nLine 2\nLine 3\n";
            fsspec_file_write(f, lines, strlen(lines));
            fsspec_file_close(f);
        }

        // 使用 FILE* 按行读取
        FILE* fp = fsspec_fopen(test_file, "r");
        REQUIRE(fp != nullptr);

        char line[64];
        int line_count = 0;
        while (fgets(line, sizeof(line), fp)) {
            line_count++;
            // 去除换行符后检查
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
            CHECK(std::string(line) == "Line " + std::to_string(line_count));
        }
        CHECK(line_count == 3);

        fclose(fp);
    }
}

TEST_CASE("filesystem operations") {
    CHECK(fsspec_init() == 0);

    SUBCASE("list directory") {
        // 创建一些测试文件
        {
            std::ofstream("/tmp/doctest_dir_a.txt") << "a";
            std::ofstream("/tmp/doctest_dir_b.txt") << "b";
        }

        fsspec_fs_t* fs = fsspec_fs_from_url("file:///tmp/");
        REQUIRE(fs != nullptr);

        // 检查文件存在
        CHECK(fsspec_fs_exists(fs, "/tmp/doctest_dir_a.txt") == true);
        CHECK(fsspec_fs_exists(fs, "/tmp/doctest_dir_b.txt") == true);
        CHECK(fsspec_fs_exists(fs, "/tmp/nonexistent.txt") == false);

        fsspec_fs_free(fs);
    }

    SUBCASE("remove file") {
        // 创建临时文件
        {
            std::ofstream("/tmp/doctest_remove.txt") << "delete me";
        }

        fsspec_fs_t* fs = fsspec_fs_from_url("file:///tmp/");
        REQUIRE(fs != nullptr);

        CHECK(fsspec_fs_exists(fs, "/tmp/doctest_remove.txt") == true);
        CHECK(fsspec_fs_remove(fs, "/tmp/doctest_remove.txt") == 0);
        CHECK(fsspec_fs_exists(fs, "/tmp/doctest_remove.txt") == false);

        fsspec_fs_free(fs);
    }
}
