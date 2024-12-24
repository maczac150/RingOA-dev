#include "file_io.hpp"

#include <fstream>
#include <sstream>

#include "logger.hpp"

namespace utils {

FileIo::FileIo(const bool debug, const std::string ext)
    : debug_(debug), ext_(ext) {
}

void FileIo::WriteValueToFile(const std::string &file_path, const uint32_t data, const bool append) {
    // Open the file
    std::ofstream file;
    if (!OpenFile(file, file_path, LOCATION, append)) {
        exit(EXIT_FAILURE);
    }
    // Write the value to the file
    file << data << "\n";
    // Close the file
    file.close();
    utils::Logger::DebugLog(LOCATION, "Value has been written to the file (" + file_path + this->ext_ + ")", this->debug_);
}

void FileIo::WriteVectorToFile(const std::string &file_path, const std::vector<uint32_t> &data, const bool append) {
    // Open the file
    std::ofstream file;
    if (!OpenFile(file, file_path, LOCATION, append)) {
        exit(EXIT_FAILURE);
    }
    // Write the vector to the file
    file << data.size() << "\n";
    for (size_t i = 0; i < data.size(); i++) {
        file << data[i];
        if (i < data.size() - 1) {
            file << ",";
        }
    }
    file << "\n";
    // Close the file
    file.close();
    utils::Logger::DebugLog(LOCATION, "Numbers have been written to the file (" + file_path + this->ext_ + ")", this->debug_);
}

void FileIo::WriteStringToFile(const std::string &file_path, const std::string &data, const bool append) {
    // Open the file
    std::ofstream file;
    if (!OpenFile(file, file_path, LOCATION, append)) {
        exit(EXIT_FAILURE);
    }
    // Write the string to the file
    file << data << "\n";
    // Close the file
    file.close();
    utils::Logger::DebugLog(LOCATION, "String have been written to the file (" + file_path + this->ext_ + ")", this->debug_);
}

void FileIo::WriteStringVectorToFile(const std::string &file_path, const std::vector<std::string> &data, const bool append) {
    // Open the file
    std::ofstream file;
    if (!OpenFile(file, file_path, LOCATION, append)) {
        exit(EXIT_FAILURE);
    }
    // Write the string vector to the file
    for (size_t i = 0; i < data.size(); i++) {
        file << data[i] << "\n";
    }
    // Close the file
    file.close();
    utils::Logger::DebugLog(LOCATION, "Strings have been written to the file (" + file_path + this->ext_ + ")", this->debug_);
}

void FileIo::ReadValueFromFile(const std::string &file_path, uint32_t &data) {
    // Open the file for reading
    std::ifstream file;
    if (OpenFile(file, file_path, LOCATION)) {
        file >> data;
    }
    // Close the file
    file.close();
    utils::Logger::DebugLog(LOCATION, "Value read from file (" + file_path + this->ext_ + "): " + std::to_string(data), this->debug_);
}

void FileIo::ReadVectorFromFile(const std::string &file_path, std::vector<uint32_t> &data) {
    // Open the file
    std::ifstream file;
    if (OpenFile(file, file_path, LOCATION)) {
        // Read the number of elements from the first line of the file
        uint32_t              size = ReadNumCountFromFile(file, LOCATION);
        std::vector<uint32_t> vec;
        vec.reserve(size);
        std::string line;
        if (std::getline(file, line)) {
            SplitStringToUint32(line, vec);
        }

        // Close the file
        file.close();
        data = std::move(vec);
    }
}

void FileIo::ReadStringFromFile(const std::string &file_path, std::string &data) {
    // Open the file
    std::ifstream file;
    if (OpenFile(file, file_path, LOCATION)) {
        std::string line;
        std::getline(file, line);
        // Close the file
        file.close();
        data = line;
    }
}

void FileIo::ClearFileContents(const std::string &file_path) {
    // Open and clear the file
    std::ofstream file;
    if (OpenFile(file, file_path, LOCATION)) {
        // Close the file
        file.close();
    }
}

bool FileIo::OpenFile(std::ofstream &file, const std::string &file_path, const std::string &location, const bool append) {
    std::ios_base::openmode mode = append ? std::ios::app : std::ios::trunc;
    file.open(file_path + this->ext_, mode);
    if (!file.is_open()) {
        utils::Logger::FatalLog(location, "Failed to open file for writing. (" + file_path + this->ext_ + ")");
        return false;
    }
    return true;
}

bool FileIo::OpenFile(std::ifstream &file, const std::string &file_path, const std::string &location) {
    file.open(file_path + this->ext_);
    if (!file.is_open()) {
        utils::Logger::FatalLog(location, "Failed to open file for reading. (" + file_path + this->ext_ + ")");
        return false;
    }
    return true;
}

uint32_t FileIo::ReadNumCountFromFile(std::ifstream &file, const std::string &location) {
    uint32_t    count = 0;
    std::string line;
    // Read the number of elements from the first line of the file
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        if (!(iss >> count)) {
            utils::Logger::FatalLog(location, "Invalid file format: Unable to read the number of elements");
            return 0U;
        }
    }
    return count;
}

void FileIo::SplitStringToUint32(const std::string &str, std::vector<uint32_t> &data) {
    std::istringstream iss(str);
    std::string        token;

    while (std::getline(iss, token, ',')) {
        uint32_t value = std::stoull(token);
        data.push_back(value);
    }
}

}    // namespace utils
