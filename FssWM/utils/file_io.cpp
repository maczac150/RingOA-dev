#include "file_io.h"

namespace fsswm {

// Clears the contents of the specified file
void FileIo::ClearFileContents(const std::string &file_path) {
    std::ofstream file(AddExtension(file_path), std::ios::trunc);    // Truncate mode clears the content
    file.close();
}

// Opens a file for writing
bool FileIo::OpenFileForWrite(std::ofstream &file, const std::string &file_path, bool append) {
    std::ios::openmode mode = std::ios::out;
    if (append)
        mode |= std::ios::app;    // Append mode
    file.open(file_path, mode);
    return file.is_open();
}

// Opens a file for reading
bool FileIo::OpenFileForRead(std::ifstream &file, const std::string &file_path) {
    file.open(file_path, std::ios::in);    // Read mode
    return file.is_open();
}

}    // namespace fsswm
