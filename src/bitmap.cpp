#include <windows.h>
#include <iostream>
#include "blob.h"
#include "bitmap.h"

void Log(LPCWSTR format, ...);

SafeDC SafeDC::Get(HWND hwnd) {
  return SafeDC(hwnd, GetDC(hwnd));
}

SafeDC SafeDC::CreateMemDC(HWND hwnd) {
  auto realDC = SafeDC::Get(hwnd);
  return SafeDC(nullptr, CreateCompatibleDC(realDC));
}

SafeDC::SafeDC(HWND hwnd, HDC dc)
  : hwnd_(hwnd), dc_(dc)
{}

SafeDC::~SafeDC() {
  if (dc_) {
    if (hwnd_) {
      ReleaseDC(hwnd_, dc_);
    }
    else {
      DeleteDC(dc_);
    }
  }
}

SafeDC::operator HDC() {
  return dc_;
}

void DIB::Release() {
  if (bitmap_) {
    DeleteObject(bitmap_);
    bitmap_ = nullptr;
    memset(&info_, 0, sizeof(info_));
    lineSizeInBytes_ = 0;
    bits_ = nullptr;
  }
}

DIB DIB::LoadFromStream(std::istream &is, HDC dc) {
  DIB dib;
  Blob blob;
  BITMAPFILEHEADER fh = {0};
  BITMAPINFO bi = {0};
  auto &ih = bi.bmiHeader;
  LPVOID bits = nullptr;

  is.read(reinterpret_cast<LPSTR>(&fh), sizeof(fh));
  if (!is || fh.bfType != 0x4D42) {
    Log(L"Invalid bitmap data.\n");
    goto cleanup;
  }

  is.read(reinterpret_cast<LPSTR>(&ih), sizeof(ih));
  if (!is
      || ih.biPlanes != 1
      || ih.biBitCount < 24
      || ih.biCompression != BI_RGB) {
    Log(L"Unsupported bitmap data.\n");
    goto cleanup;
  }

  if (!is.seekg(fh.bfOffBits, std::ios::beg)) {
    Log(L"Invalid offset.\n");
    goto cleanup;
  }

  const auto lineSizeInBytes = int((ih.biWidth * ih.biBitCount + 31) / 32) * 4;
  const auto numPixels = lineSizeInBytes * std::abs(ih.biHeight);

  if (blob.Alloc(numPixels)) {
    if (!is.read(reinterpret_cast<LPSTR>(LPBYTE(blob)), numPixels)) {
      Log(L"Failed to load pixel data.\n");
      goto cleanup;
    }
  }
  else {
    Log(L"Failed to allocate memory.\n");
    goto cleanup;
  }

  if (auto bitmap = CreateDIBSection(dc,
                                     &bi,
                                     DIB_RGB_COLORS,
                                     &bits,
                                     /*hSection*/nullptr,
                                     /*dwOffset*/0)) {
    dib.info_.bmiHeader = ih;
    dib.bitmap_ = bitmap;
    dib.lineSizeInBytes_ = lineSizeInBytes;
    dib.bits_ = bits;
    memcpy(bits, blob, blob.Size());
  }

cleanup:
  return dib;
}

DIB DIB::CreateNew(HDC dc, WORD bitCount, LONG width, LONG height) {
  DIB dib;
  if (bitCount >= 24 && width >= 0 && height != 0) {
    BITMAPINFO bi = {0};
    auto &ih = bi.bmiHeader;
    ih.biSize = sizeof(ih);
    ih.biWidth = width;
    ih.biHeight = height;
    ih.biPlanes = 1;
    ih.biBitCount = bitCount;
    ih.biCompression = BI_RGB;
    LPVOID bits = nullptr;
    if (auto bitmap = CreateDIBSection(dc,
                                       &bi,
                                       DIB_RGB_COLORS,
                                       &bits,
                                       /*hSection*/nullptr,
                                       /*dwOffset*/0)) {
      dib.info_ = bi;
      dib.bitmap_ = bitmap;
      dib.lineSizeInBytes_ = int((ih.biWidth * ih.biBitCount + 31) / 32) * 4;
      dib.bits_ = bits;
    }
  }
  return dib;
}

DIB::DIB()
  : info_({0}),
    bitmap_(nullptr),
    lineSizeInBytes_(0),
    bits_(nullptr)
{}

DIB::DIB(DIB &&other)
  : info_(other.info_),
    bitmap_(other.bitmap_),
    lineSizeInBytes_(other.lineSizeInBytes_),
    bits_(other.bits_) {
  info_ = {0};
  bitmap_ = nullptr;
  lineSizeInBytes_ = 0;
  bits_ = nullptr;
}

DIB::~DIB() {
  Release();
}

DIB::operator HBITMAP() {
  return bitmap_;
}

DIB &DIB::operator=(DIB &&other) {
  if (this != &other) {
    Release();
    std::swap(info_, other.info_);
    std::swap(bitmap_, other.bitmap_);
    std::swap(lineSizeInBytes_, other.lineSizeInBytes_);
    std::swap(bits_, other.bits_);
  }
  return *this;
}

std::ostream &DIB::Save(std::ostream &os) const {
  const auto &ih = info_.bmiHeader;
  const auto numPixels = lineSizeInBytes_ * std::abs(ih.biHeight);

  BITMAPFILEHEADER fh = {0};
  fh.bfType = 0x4D42;
  fh.bfSize = sizeof(fh) + sizeof(ih) + numPixels;
  fh.bfOffBits = sizeof(fh) + sizeof(ih);

  os.write(reinterpret_cast<LPCSTR>(&fh), sizeof(fh));
  os.write(reinterpret_cast<LPCSTR>(&ih), sizeof(ih));
  os.write(reinterpret_cast<LPCSTR>(bits_), numPixels);
  return os;
}

void DIB::CopyTo(Blob &blob) const {
  const auto &ih = info_.bmiHeader;
  const auto numPixels = lineSizeInBytes_ * std::abs(ih.biHeight);
  if (blob.Alloc(bits_ ? numPixels : 0)) {
    memcpy(blob, bits_, blob.Size());
  }
}

LPBYTE DIB::At(DWORD x, DWORD y) {
  const auto &ih = info_.bmiHeader;
  auto p = reinterpret_cast<LPBYTE>(bits_);
  if (!bits_
      || x >= static_cast<DWORD>(ih.biWidth)
      || y >= static_cast<DWORD>(std::abs(ih.biHeight))) {
    return nullptr;
  }
  else if (ih.biHeight >= 0) {
    p += lineSizeInBytes_ * (ih.biHeight - 1 - y);
  }
  else {
    p += lineSizeInBytes_ * y;
  }
  return p + (x * ih.biBitCount / 8);
}

LPCBYTE DIB::At(DWORD x, DWORD y) const {
  return const_cast<DIB*>(this)->At(x, y);
}
