#pragma once

#include <Arduino.h>
#include <Dir.h>
#include <File.h>
#include <FlashIAPBlockDevice.h>
#include <LittleFileSystem.h>
#include <fcntl.h>
#include <platform/mbed_retarget.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <utility>

class Apollo3File : public Stream {
  mbed::File* _file;
  mbed::Dir* _dir;
  mbed::FileSystem* _filesystem;
  bool _has_peeked;
  uint8_t _peeked;
  bool _is_directory;
  uint32_t _entry_size;
  char _path[256];
  char _name[NAME_MAX + 1];

public:
  explicit Apollo3File(mbed::File* file = NULL, mbed::Dir* dir = NULL,
                       mbed::FileSystem* filesystem = NULL,
                       const char* path = NULL, const char* name = NULL,
                       bool is_directory = false, uint32_t entry_size = 0)
    : _file(file), _dir(dir), _filesystem(filesystem),
      _has_peeked(false), _peeked(0),
      _is_directory(is_directory), _entry_size(entry_size) {
    _path[0] = '\0';
    _name[0] = '\0';
    if (path) {
      strncpy(_path, path, sizeof(_path) - 1);
      _path[sizeof(_path) - 1] = '\0';
    }
    if (name) {
      strncpy(_name, name, sizeof(_name) - 1);
      _name[sizeof(_name) - 1] = '\0';
    }
  }

  Apollo3File(const Apollo3File&) = delete;
  Apollo3File& operator=(const Apollo3File&) = delete;

  Apollo3File(Apollo3File&& other)
    : _file(other._file), _dir(other._dir), _filesystem(other._filesystem),
      _has_peeked(other._has_peeked),
      _peeked(other._peeked), _is_directory(other._is_directory),
      _entry_size(other._entry_size) {
    memcpy(_path, other._path, sizeof(_path));
    memcpy(_name, other._name, sizeof(_name));
    other._file = NULL;
    other._dir = NULL;
    other._filesystem = NULL;
    other._has_peeked = false;
    other._path[0] = '\0';
    other._name[0] = '\0';
  }

  Apollo3File& operator=(Apollo3File&& other) {
    if (this != &other) {
      close();
      _file = other._file;
      _dir = other._dir;
      _filesystem = other._filesystem;
      _has_peeked = other._has_peeked;
      _peeked = other._peeked;
      _is_directory = other._is_directory;
      _entry_size = other._entry_size;
      memcpy(_path, other._path, sizeof(_path));
      memcpy(_name, other._name, sizeof(_name));
      other._file = NULL;
      other._dir = NULL;
      other._filesystem = NULL;
      other._has_peeked = false;
      other._path[0] = '\0';
      other._name[0] = '\0';
    }
    return *this;
  }

  ~Apollo3File() { close(); }

  operator bool() const { return _file != NULL || _dir != NULL || _name[0] != '\0'; }

  int available() override {
    if (!_file) return 0;
    off_t remaining = _file->size() - _file->tell();
    if (_has_peeked) remaining++;
    if (remaining <= 0) return 0;
    return remaining > INT_MAX ? INT_MAX : (int)remaining;
  }

  int read() override {
    uint8_t value;
    return read(&value, 1) == 1 ? value : -1;
  }

  int read(uint8_t* buffer, size_t length) {
    if (!_file || !buffer || length == 0) return 0;

    size_t read_count = 0;
    if (_has_peeked) {
      buffer[read_count++] = _peeked;
      _has_peeked = false;
    }

    if (read_count < length) {
      ssize_t result = _file->read(buffer + read_count, length - read_count);
      if (result < 0) return read_count > 0 ? (int)read_count : -1;
      read_count += (size_t)result;
    }
    return (int)read_count;
  }

  int peek() override {
    if (!_file) return -1;
    if (!_has_peeked) {
      ssize_t result = _file->read(&_peeked, 1);
      if (result != 1) return -1;
      _has_peeked = true;
    }
    return _peeked;
  }

  void flush() override {
    if (_file) _file->sync();
  }

  size_t write(uint8_t value) override {
    return write(&value, 1);
  }

  size_t write(const uint8_t* buffer, size_t length) override {
    if (!_file || !buffer) return 0;
    ssize_t result = _file->write(buffer, length);
    return result < 0 ? 0 : (size_t)result;
  }

  int readBytes(uint8_t* buffer, size_t length) {
    return read(buffer, length);
  }

  bool seek(uint32_t position) {
    if (!_file) return false;
    _has_peeked = false;
    return _file->seek(position, SEEK_SET) == (off_t)position;
  }

  uint32_t position() const {
    if (!_file) return 0;
    off_t position = _file->tell();
    return position < 0 ? 0 : (uint32_t)position;
  }

  uint32_t size() const {
    if (_name[0] != '\0') return _entry_size;
    if (!_file) return 0;
    off_t size = _file->size();
    return size < 0 ? 0 : (uint32_t)size;
  }

  bool isDirectory() const { return _is_directory; }

  const char* name() const { return _name; }

  Apollo3File openNextFile() {
    if (!_dir) return Apollo3File();

    struct dirent entry;
    if (_dir->read(&entry) <= 0) return Apollo3File();

    char full_path[sizeof(_path) + sizeof(entry.d_name) + 2];
    int path_length = snprintf(full_path, sizeof(full_path), "%s%s%s",
                               _path, _path[0] && _path[strlen(_path) - 1] != '/' ? "/" : "",
                               entry.d_name);
    if (path_length < 0 || (size_t)path_length >= sizeof(full_path)) return Apollo3File();

    struct stat info;
    if (!_filesystem || _filesystem->stat(full_path, &info) != 0) return Apollo3File();
    bool is_directory = (info.st_mode & S_IFMT) == S_IFDIR;
    uint32_t entry_size = info.st_size < 0 ? 0 : (uint32_t)info.st_size;
    return Apollo3File(NULL, NULL, _filesystem, NULL, entry.d_name, is_directory, entry_size);
  }

  void close() {
    if (_file) {
      _file->close();
      delete _file;
      _file = NULL;
      _filesystem = NULL;
    }
    if (_dir) {
      _dir->close();
      delete _dir;
      _dir = NULL;
      _filesystem = NULL;
    }
    _has_peeked = false;
  }
};

typedef Apollo3File File;

class Apollo3FileSystem {
  static const uint32_t FLASH_FS_ADDRESS = 0xDE000;
  static const uint32_t FLASH_FS_SIZE = 0x20000;
  static const uint32_t FLASH_FS_BLOCK_SIZE = 0x2000;

  FlashIAPBlockDevice _block_device;
  mbed::LittleFileSystem _filesystem;

  int makeOpenFlags(const char* mode, bool truncate) const {
    if (!mode || mode[0] == 'r') {
      return mode && mode[1] == '+' ? O_RDWR : O_RDONLY;
    }
    if (mode[0] == 'w') {
      int flags = O_WRONLY | O_CREAT;
      if (truncate || mode[1] != '+') flags |= O_TRUNC;
      return flags;
    }
    if (mode[0] == 'a') {
      return O_WRONLY | O_CREAT | O_APPEND;
    }
    return -1;
  }

public:
  Apollo3FileSystem()
    : _block_device(FLASH_FS_ADDRESS, FLASH_FS_SIZE),
      _filesystem("meshcore", NULL, MBED_LFS_READ_SIZE, MBED_LFS_PROG_SIZE,
                  FLASH_FS_BLOCK_SIZE, MBED_LFS_LOOKAHEAD) { }

  int begin() {
    int status = _filesystem.mount(&_block_device);
    if (status == 0) return 0;

    Serial.print("ERROR: filesystem mount failed, formatting storage: ");
    Serial.println(status);
    return _filesystem.reformat(&_block_device);
  }

  Apollo3File open(const char* path, const char* mode = "r", bool truncate = false) {
    int flags = makeOpenFlags(mode, truncate);
    if (flags < 0) return Apollo3File();
    if (!path || path[0] == '\0') path = "/";

    struct stat info;
    if (_filesystem.stat(path, &info) == 0 && (info.st_mode & S_IFMT) == S_IFDIR) {
      mbed::Dir* dir = new mbed::Dir();
      if (!dir) return Apollo3File();
      if (dir->open(&_filesystem, path) != 0) {
        delete dir;
        return Apollo3File();
      }
      return Apollo3File(NULL, dir, &_filesystem, path);
    }

    mbed::File* file = new mbed::File();
    if (!file) return Apollo3File();

    if (file->open(&_filesystem, path, flags) != 0) {
      delete file;
      return Apollo3File();
    }
    return Apollo3File(file);
  }

  bool exists(const char* path) {
    struct stat info;
    return path && _filesystem.stat(path, &info) == 0;
  }

  bool mkdir(const char* path) {
    return path && _filesystem.mkdir(path, 0777) == 0;
  }

  bool remove(const char* path) {
    return path && _filesystem.remove(path) == 0;
  }

  bool format() {
    return _filesystem.reformat(&_block_device) == 0;
  }

  uint32_t usedBytes() {
    struct statvfs info;
    if (_filesystem.statvfs("/", &info) != 0) return 0;
    return (uint32_t)(info.f_blocks - info.f_bfree) * info.f_bsize;
  }

  uint32_t totalBytes() {
    struct statvfs info;
    if (_filesystem.statvfs("/", &info) != 0) return 0;
    return (uint32_t)info.f_blocks * info.f_bsize;
  }

  mbed::FileSystem* getMbedFileSystem() { return &_filesystem; }
};
