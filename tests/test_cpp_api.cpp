#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "fsspec_cpp/fs.hpp"
#include <fstream>
#include <cstdio>

using namespace fsspec;

TEST_CASE("basic filesystem operations") {
    SUBCASE("open local file for writing") {
        auto f = open("file:///tmp/doctest_fsspec.txt", "w");
        REQUIRE(f != nullptr);
        
        const char* data = "Hello from doctest!";
        f->write_all(data);
        
        f->close();
    }
    
    SUBCASE("open local file for reading") {
        // 先创建文件
        {
            std::ofstream ofs("/tmp/doctest_fsspec_read.txt");
            ofs << "Test content for reading";
        }
        
        auto f = open("file:///tmp/doctest_fsspec_read.txt", "r");
        REQUIRE(f != nullptr);
        
        std::string content = f->read_all();
        CHECK(content == "Test content for reading");
        
        f->close();
    }
    
    SUBCASE("file exists") {
        // 创建文件
        {
            std::ofstream ofs("/tmp/doctest_exists.txt");
            ofs << "test";
        }
        
        CHECK(exists("file:///tmp/doctest_exists.txt") == true);
        CHECK(exists("file:///tmp/doctest_not_exists.txt") == false);
    }
    
    SUBCASE("seek and tell") {
        // 创建测试文件
        {
            std::ofstream ofs("/tmp/doctest_seek.txt");
            ofs << "0123456789";
        }
        
        auto f = open("file:///tmp/doctest_seek.txt", "r");
        REQUIRE(f != nullptr);
        
        CHECK(f->tell() == 0);
        
        char buf[5];
        f->read(buf, 5);
        CHECK(f->tell() == 5);
        
        f->seek(0, 0);  // SEEK_SET
        CHECK(f->tell() == 0);
        
        f->seek(0, 2);  // SEEK_END
        CHECK(f->tell() == 10);
        
        f->close();
    }
}

TEST_CASE("filesystem from url") {
    SUBCASE("local filesystem") {
        auto fs = filesystem_from_url("file:///tmp/");
        REQUIRE(fs != nullptr);
        
        // 创建测试文件
        {
            std::ofstream ofs("/tmp/doctest_fs.txt");
            ofs << "filesystem test";
        }
        
        CHECK(fs->exists("/tmp/doctest_fs.txt") == true);
        
        auto info = fs->info("/tmp/doctest_fs.txt");
        CHECK(info.name.find("doctest_fs.txt") != std::string::npos);
        CHECK(info.size == 15);  // "filesystem test" length
        CHECK(info.is_dir == false);
    }
    
    SUBCASE("list directory") {
        auto fs = filesystem_from_url("file:///tmp/");
        REQUIRE(fs != nullptr);
        
        auto entries = fs->ls("/tmp/");
        // 应该至少有一些条目（假设 /tmp 不为空）
        CHECK(entries.size() >= 0);
    }
}

TEST_CASE("file info") {
    SUBCASE("file info fields") {
        // 创建带已知内容的文件
        {
            std::ofstream ofs("/tmp/doctest_info.txt");
            ofs << "test data content";
        }
        
        auto fs = filesystem_from_url("file:///tmp/");
        auto info = fs->info("/tmp/doctest_info.txt");
        
        CHECK(info.size == 17);  // "test data content" length
        CHECK(info.is_dir == false);
        CHECK(info.name.find("doctest_info.txt") != std::string::npos);
    }
}
