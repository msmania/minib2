#include <windows.h>
#include <windowsx.h>
#include <strsafe.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <blob.h>
#include <bitmap.h>

void Log(LPCWSTR format, ...) {
  WCHAR linebuf[1024];
  va_list v;
  va_start(v, format);
  StringCbVPrintf(linebuf, sizeof(linebuf), format, v);
  OutputDebugString(linebuf);
}

static DIB LoadFromBlob(const std::string &blob, HWND hwnd) {
  DIB dib;
  if (auto memDC = SafeDC::CreateMemDC(hwnd)) {
    std::istringstream iss(blob, std::ios::binary | std::ios::in);
    dib = DIB::LoadFromStream(iss, memDC);
  }
  return dib;
}

TEST(DIB, StreamLoadSave) {
  BYTE Bitmap5x3[] = {
    0x42, 0x4d, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x80, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  const auto offsetToHeight = 0x16;
  const BYTE Pixels[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x80, 0xff, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  auto bitmapStr = std::string(reinterpret_cast<LPCSTR>(Bitmap5x3),
                               ARRAYSIZE(Bitmap5x3));
  DIB dib = LoadFromBlob(bitmapStr, /*hwnd*/nullptr);
  EXPECT_NE(HBITMAP(dib), nullptr);

  std::ostringstream oss;
  ASSERT_TRUE(dib.Save(oss));
  EXPECT_EQ(oss.str(), bitmapStr);

  Blob pixels;
  dib.CopyTo(pixels);
  EXPECT_EQ(memcmp(pixels, Pixels, sizeof(Pixels)), 0);

  const BYTE red[] = {0x01, 0x00, 0xff};
  const BYTE orange[] = {0x01, 0x80, 0xff};
  const BYTE yellow[] = {0x01, 0xff, 0xff};
  EXPECT_EQ(memcmp(dib.At(0, 0), red, 3), 0);
  EXPECT_EQ(memcmp(dib.At(1, 1), orange, 3), 0);
  EXPECT_EQ(memcmp(dib.At(2, 2), yellow, 3), 0);
  EXPECT_EQ(dib.At(5, 0), nullptr);
  EXPECT_EQ(dib.At(0, 3), nullptr);

  // Negative height DIB
  auto &height = *reinterpret_cast<PLONG>(Bitmap5x3 + offsetToHeight);
  height = -height;
  bitmapStr = std::string(reinterpret_cast<LPCSTR>(Bitmap5x3),
                          ARRAYSIZE(Bitmap5x3));
  dib = LoadFromBlob(bitmapStr, /*hwnd*/nullptr);
  EXPECT_NE(HBITMAP(dib), nullptr);
  EXPECT_EQ(memcmp(dib.At(0, 2), red, 3), 0);
  EXPECT_EQ(memcmp(dib.At(1, 1), orange, 3), 0);
  EXPECT_EQ(memcmp(dib.At(2, 0), yellow, 3), 0);
}

TEST(DIB, CreateDIBSection) {
  if (auto memDC = SafeDC::CreateMemDC(/*hwnd*/nullptr)) {
    DIB dib = DIB::CreateNew(memDC, 24, 5, 3);
    EXPECT_NE(HBITMAP(dib), nullptr);

    Blob bits;
    dib.CopyTo(bits);
    EXPECT_EQ(bits.Size(), 48);

    dib = DIB::CreateNew(memDC, 16, 100, 4);
    EXPECT_EQ(HBITMAP(dib), nullptr);

    dib = DIB::CreateNew(memDC, 24, 9, -2);
    EXPECT_NE(HBITMAP(dib), nullptr);
    dib.CopyTo(bits);
    EXPECT_EQ(bits.Size(), 28 * 2);
  }
}
