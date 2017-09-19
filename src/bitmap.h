class SafeDC {
private:
  HWND hwnd_;
  HDC dc_;

  SafeDC(HWND hwnd, HDC dc);

public:
  static SafeDC Get(HWND hwnd);
  static SafeDC CreateMemDC(HWND hwnd);

  ~SafeDC();
  operator HDC();
};

class DIB {
private:
  BITMAPINFO info_;
  HBITMAP bitmap_;
  DWORD lineSizeInBytes_;
  LPVOID bits_;

  void Release();

public:
  static DIB LoadFromStream(std::istream &is, HDC dc);
  static DIB CreateNew(HDC dc, WORD bitCount, LONG width, LONG height);

  DIB();
  DIB(DIB &&other);
  ~DIB();
  operator HBITMAP();
  DIB &operator=(DIB &&other);
  std::ostream &Save(std::ostream &os) const;
  void CopyTo(Blob &blob) const;
  LPBYTE At(DWORD x, DWORD y);
  LPCBYTE At(DWORD x, DWORD y) const;
};
