#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "blob.h"

void Log(LPCWSTR Format, ...);

void Blob::Release() {
  if (buffer_) {
    HeapFree(heap_, 0, buffer_);
    buffer_ = nullptr;
    size_ = 0;
  }
}

Blob::Blob()
  : heap_(GetProcessHeap()),
    buffer_(nullptr),
    size_(0)
{}

Blob::Blob(SIZE_T size)
  : heap_(GetProcessHeap()),
    buffer_(nullptr),
    size_(0)
{
  Alloc(size);
}

Blob::Blob(Blob &&other)
  : heap_(GetProcessHeap()),
    buffer_(nullptr),
    size_(0) {
  std::swap(buffer_, other.buffer_);
  std::swap(heap_, other.heap_);
  std::swap(size_, other.size_);
}

Blob::~Blob() {
  Release();
}

Blob::operator PBYTE() {
  return reinterpret_cast<PBYTE>(buffer_);
}

Blob::operator LPCBYTE() const {
  return reinterpret_cast<LPCBYTE>(buffer_);
}

Blob &Blob::operator=(Blob &&other) {
  if (this != &other) {
    Release();
    heap_ = other.heap_;
    buffer_ = other.buffer_;
    size_ = other.size_;
    other.buffer_ = nullptr;
    other.size_ = 0;
  }
  return *this;
}

SIZE_T Blob::Size() const {
  return size_;
}

bool Blob::Alloc(SIZE_T size) {
  if (buffer_) {
    buffer_ = HeapReAlloc(GetProcessHeap(), 0, buffer_, size);
    if (buffer_) {
      size_ = size;
    }
    else {
      Log(L"HeapReAlloc failed - %08x\n", GetLastError());
    }
  }
  else if (size > 0) {
    buffer_ = HeapAlloc(GetProcessHeap(), 0, size);
    if (buffer_) {
      size_ = size;
    }
    else {
      Log(L"HeapAlloc failed - %08x\n", GetLastError());
    }
  }
  return buffer_ != nullptr;
}

void Blob::Dump(std::wostream &os, size_t width, size_t ellipsis) const {
  if (auto p = reinterpret_cast<LPCBYTE>(buffer_)) {
    os << L"Total: " << size_ << L" (=0x"
       << std::hex << size_ << L") bytes\r\n";

    size_t bytesLeft = min(size_, ellipsis);
    int lineCount = 0;
    while (bytesLeft) {
      size_t i;
      for (i = 0; bytesLeft && i < width; ++i) {
        if (i == 0)
          os << std::hex << std::setfill(L'0') << std::setw(4)
             << (lineCount * width) << L':';

        if (i > 0 && i % 8 == 0)
          os << L"  " << std::hex << std::setfill(L'0') << std::setw(2) << *(p++);
        else
          os << L' ' << std::hex << std::setfill(L'0') << std::setw(2) << *(p++);

        --bytesLeft;
      }
      ++lineCount;
      if (i == width) os << L"\r\n";
    }

    if (size_ > ellipsis)
      os << L" ...\r\n";
  }
}
