#include <windows.h>
#include <iostream>
#include <assert.h>
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
    bits_ = nullptr;
    lineSizeInBytes_ = 0;
  }
}

DIB DIB::LoadFromStream(std::istream &is, HDC dc, HANDLE section) {
  DIB dib;
  Blob blob;
  BITMAPFILEHEADER fh = {0};
  BITMAPINFOHEADER ih = {0};
  Blob bitmapInfo;
  LPVOID bits = nullptr;

  is.read(reinterpret_cast<LPSTR>(&fh), sizeof(fh));
  if (!is || fh.bfType != 0x4D42) {
    Log(L"Invalid bitmap data.\n");
    goto cleanup;
  }

  is.read(reinterpret_cast<LPSTR>(&ih), sizeof(ih));
  if (!is
      || ih.biPlanes != 1
      || ih.biCompression != BI_RGB) {
    Log(L"Unsupported bitmap data.\n");
    goto cleanup;
  }

  const auto numColorEntries = ih.biBitCount >= 24 ? 0 : 1 << ih.biBitCount;
  const auto colorTableSize = numColorEntries * sizeof(RGBQUAD);

  if (!bitmapInfo.Alloc(sizeof(BITMAPINFOHEADER) + colorTableSize)) {
    Log(L"Failed to allocate memory.\n");
    goto cleanup;
  }

  auto &bi = *(bitmapInfo.As<BITMAPINFO>());
  bi.bmiHeader = ih;
  if (!is.read(reinterpret_cast<LPSTR>(&bi.bmiColors), colorTableSize)) {
    Log(L"Failed to load color table.\n");
    goto cleanup;
  }

  if (!is.seekg(fh.bfOffBits, std::ios::beg)) {
    Log(L"Invalid offset.\n");
    goto cleanup;
  }

  const auto lineSizeInBytes = int((ih.biWidth * ih.biBitCount + 31) / 32) * 4;
  const auto numPixels = lineSizeInBytes * std::abs(ih.biHeight);

  if (blob.Alloc(numPixels)) {
    if (!is.read(blob.As<char>(), numPixels)) {
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
                                     section,
                                     /*dwOffset*/0)) {
    dib.info_ = std::move(bitmapInfo);
    dib.lineSizeInBytes_ = lineSizeInBytes;
    dib.bitmap_ = bitmap;
    dib.bits_ = bits;
    memcpy(bits, blob, blob.Size());
  }

cleanup:
  return dib;
}

static Blob CreateBitmapInfo(LONG width,
                             LONG height,
                             WORD bitCount,
                             bool initWithGrayscaleTable) {
  int colorTableEntries = bitCount >= 24 ? 0 : (1 << bitCount);
  Blob blob(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * colorTableEntries);
  if (auto p = blob.As<BITMAPINFO>()) {
    auto &ih = p->bmiHeader;
    ih = {0};
    ih.biSize = sizeof(ih);
    ih.biWidth = width;
    ih.biHeight = height;
    ih.biPlanes = 1;
    ih.biBitCount = bitCount;
    ih.biCompression = BI_RGB;
    if (initWithGrayscaleTable) {
      int delta = colorTableEntries >= 2 ? (255 / (colorTableEntries - 1)) : 0;
      for (int i = 0; i < colorTableEntries; ++i) {
        auto &col = p->bmiColors[i];
        col.rgbRed = col.rgbGreen = col.rgbBlue = static_cast<BYTE>(i * delta);
        col.rgbReserved = 0;
      }
    }
  }
  return blob;
}

DIB DIB::CreateNew(HDC dc,
                   WORD bitCount,
                   LONG width,
                   LONG height,
                   HANDLE section,
                   bool initWithGrayscaleTable) {
  DIB dib;
  if (width > 0 && height != 0) {
    Blob bitmapInfoBlob = CreateBitmapInfo(width,
                                           height,
                                           bitCount,
                                           initWithGrayscaleTable);
    auto &bi = *(bitmapInfoBlob.As<BITMAPINFO>());
    auto &ih = bi.bmiHeader;
    LPVOID bits = nullptr;
    if (auto bitmap = CreateDIBSection(dc,
                                       &bi,
                                       DIB_RGB_COLORS,
                                       &bits,
                                       section,
                                       /*dwOffset*/0)) {
      dib.info_ = std::move(bitmapInfoBlob);
      dib.bitmap_ = bitmap;
      dib.lineSizeInBytes_ = int((ih.biWidth * ih.biBitCount + 31) / 32) * 4;
      dib.bits_ = bits;
    }
    else {
      Log(L"CreateDIBSection failed - %08x\n", GetLastError());
    }
  }
  return dib;
}

DIB::DIB()
  : lineSizeInBytes_(0),
    bitmap_(nullptr),
    bits_(nullptr)
{}

DIB::DIB(DIB &&other)
  : info_(std::move(other.info_)),
    lineSizeInBytes_(other.lineSizeInBytes_),
    bitmap_(other.bitmap_),
    bits_(other.bits_) {
  other.bitmap_ = nullptr;
  other.lineSizeInBytes_ = 0;
  other.bits_ = nullptr;
}

DIB::~DIB() {
  Release();
}

DIB::operator HBITMAP() {
  return bitmap_;
}

const BITMAPINFO *DIB::GetBitmapInfo() const {
  return info_.As<BITMAPINFO>();
}

RGBQUAD *DIB::GetColorTable() {
  return info_.As<BITMAPINFO>()->bmiColors;
}

LPBYTE DIB::GetBits() {
  return reinterpret_cast<LPBYTE>(bits_);
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
  if (bitmap_) {
    const auto &ih = GetBitmapInfo()->bmiHeader;
    const SIZE_T numPixels = lineSizeInBytes_ * std::abs(ih.biHeight);

    if (numPixels > (1ull << 32)) {
      os.setstate(std::ios::failbit);
      return os;
    }

    BITMAPFILEHEADER fh = {0};
    fh.bfType = 0x4D42;
    fh.bfSize = sizeof(fh) + info_.Size() + numPixels;
    fh.bfOffBits = sizeof(fh) + info_.Size();
    os.write(reinterpret_cast<LPCSTR>(&fh), sizeof(fh));
    os.write(info_.As<char>(), info_.Size());
    os.write(reinterpret_cast<LPCSTR>(bits_), numPixels);
  }
  return os;
}

void DIB::CopyTo(Blob &blob) const {
  if (bitmap_) {
    const auto &ih = GetBitmapInfo()->bmiHeader;
    const SIZE_T numPixels = lineSizeInBytes_ * std::abs(ih.biHeight);
    if (blob.Alloc(bits_ ? numPixels : 0)) {
      memcpy(blob, bits_, blob.Size());
    }
  }
}

LPBYTE DIB::At(DWORD x, DWORD y) {
  const auto &ih = GetBitmapInfo()->bmiHeader;
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

LONG GetMagic(HDC dc) {return 1;}

// https://msdn.microsoft.com/en-us/library/windows/desktop/dd183402(v=vs.85).aspx
DIB DIB::CaptureFromHDC(HDC sourceDC,
                        WORD bitCount,
                        DWORD &width,
                        DWORD &height,
                        HANDLE section) {
  auto magic = GetMagic(sourceDC);
  Log(L"Magic factor: %d\n", magic);
  width *= magic;
  height *= magic;

  DIB dib;
  if (HDC memDC = CreateCompatibleDC(sourceDC)) {
    if (HBITMAP compatibleBitmap = CreateCompatibleBitmap(sourceDC,
                                                          width,
                                                          height)) {
      SelectObject(memDC, compatibleBitmap);
      if (BitBlt(memDC,
                 0, 0, width, height,
                 sourceDC,
                 0, 0,
                 SRCCOPY)) {
        // There is no meaning to initialize the color table because
        // GetDIBits overwrites it anyway.
        auto newDib = CreateNew(sourceDC,
                                bitCount,
                                width,
                                height,
                                section,
                                /*initWithGrayscaleTable*/false);
        if (GetDIBits(sourceDC,
                      compatibleBitmap,
                      0,
                      height,
                      newDib.bits_,
                      newDib.info_.As<BITMAPINFO>(),
                      DIB_RGB_COLORS)) {
          dib = std::move(newDib);
        }
        else {
          Log(L"GetDIBits failed - %08x\n", GetLastError());
        }
      }
      else {
        Log(L"BitBlt failed - %08x\n", GetLastError());
      }
      assert(DeleteObject(compatibleBitmap));
    }
    DeleteDC(memDC);
  }
  return dib;
}

bool DIB::ConvertToGrayscale(HDC dc, HANDLE section) {
  bool ret = false;
  const float B2YF = 0.114f;
  const float G2YF = 0.587f;
  const float R2YF = 0.299f;
  const auto &bi = GetBitmapInfo();
  if (bitmap_
      && info_
      && bi->bmiHeader.biBitCount == 32
      && bi->bmiHeader.biHeight > 0) {
    const DWORD width = bi->bmiHeader.biWidth;
    const DWORD height = bi->bmiHeader.biHeight;
    auto grayscale = CreateNew(dc,
                               /*bitCount*/8,
                               width,
                               height,
                               section,
                               /*initWithGrayscaleTable*/true);
    auto bitsSrc = reinterpret_cast<CONST RGBQUAD *>(bits_);
    for (DWORD y = 0; y < height; ++y) {
      auto bitsDst = reinterpret_cast<LPBYTE>(grayscale.bits_)
                     + grayscale.lineSizeInBytes_ * y;
      for (DWORD x = 0; x < width; ++x) {
        *(bitsDst++) = static_cast<BYTE>(R2YF * bitsSrc->rgbRed
                                         + G2YF * bitsSrc->rgbGreen
                                         + B2YF * bitsSrc->rgbBlue);
        ++bitsSrc;
      }
    }
    std::swap(*this, grayscale);
    ret = true;
  }
  return ret;
}