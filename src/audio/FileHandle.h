#pragma once
#include <cstdio>
#include <string>

/**
 * FileHandle - RAII wrapper for FILE* operations
 * 
 * Provides automatic file handle management with proper cleanup
 * in case of exceptions or early returns. Prevents file handle leaks.
 */
class FileHandle {
public:
    FileHandle() : file_(nullptr) {}
    
    explicit FileHandle(const std::string& filename, const std::string& mode) 
        : file_(nullptr) {
        open(filename, mode);
    }
    
    // Move constructor
    FileHandle(FileHandle&& other) noexcept : file_(other.file_) {
        other.file_ = nullptr;
    }
    
    // Move assignment
    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            close();
            file_ = other.file_;
            other.file_ = nullptr;
        }
        return *this;
    }
    
    // Disable copy operations
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    
    ~FileHandle() {
        close();
    }
    
    bool open(const std::string& filename, const std::string& mode) {
        close(); // Close any existing file first
        file_ = fopen(filename.c_str(), mode.c_str());
        return file_ != nullptr;
    }
    
    void close() {
        if (file_) {
            fclose(file_);
            file_ = nullptr;
        }
    }
    
    bool is_open() const { return file_ != nullptr; }
    
    FILE* get() const { return file_; }
    
    // Convenience operators for direct FILE* operations
    operator FILE*() const { return file_; }
    operator bool() const { return is_open(); }
    
    // File operations wrappers
    size_t write(const void* data, size_t size, size_t count) {
        return file_ ? fwrite(data, size, count, file_) : 0;
    }
    
    size_t read(void* data, size_t size, size_t count) {
        return file_ ? fread(data, size, count, file_) : 0;
    }
    
    int flush() {
        return file_ ? fflush(file_) : EOF;
    }
    
    long tell() const {
        return file_ ? ftell(file_) : -1;
    }
    
    int seek(long offset, int whence) {
        return file_ ? fseek(file_, offset, whence) : -1;
    }

private:
    FILE* file_;
};