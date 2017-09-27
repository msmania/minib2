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
  Blob info_;
  HBITMAP bitmap_;
  DWORD lineSizeInBytes_;
  LPVOID bits_;

  void Release();

public:
  static DIB LoadFromStream(std::istream &is, HDC dc, HANDLE section);
  static DIB CreateNew(HDC dc,
                       WORD bitCount,
                       LONG width,
                       LONG height,
                       HANDLE section,
                       bool initWithGrayscaleTable);
  static DIB CaptureFromHDC(HDC sourceDC,
                            WORD bitCount,
                            DWORD &width,
                            DWORD &height,
                            HANDLE section);

  DIB();
  DIB(DIB &&other);
  ~DIB();

  operator HBITMAP();
  DIB &operator=(DIB &&other);
  const BITMAPINFO *GetBitmapInfo() const;
  RGBQUAD *GetColorTable();
  LPBYTE GetBits();
  std::ostream &Save(std::ostream &os) const;
  void CopyTo(Blob &blob) const;
  bool ConvertToGrayscale(HDC dc, HANDLE section);
  LPBYTE At(DWORD x, DWORD y);
  LPCBYTE At(DWORD x, DWORD y) const;
};
