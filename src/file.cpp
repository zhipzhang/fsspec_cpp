#include "fsspec_cpp/fs.hpp"
#include <sstream>

namespace fsspec {

std::string File::read_all() {
    std::ostringstream oss;
    char buffer[8192];
    size_t n;
    while ((n = read(buffer, sizeof(buffer))) > 0) {
        oss.write(buffer, n);
    }
    return oss.str();
}

void File::write_all(const std::string& data) {
    const char* ptr = data.data();
    size_t remaining = data.size();
    while (remaining > 0) {
        size_t written = write(ptr, remaining);
        if (written == 0) break;
        ptr += written;
        remaining -= written;
    }
}

} // namespace fsspec
