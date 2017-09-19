class Blob {
private:
  HANDLE heap_;
  LPVOID buffer_;
  DWORD size_;

  void Release();

public:
  Blob();
  Blob(DWORD size);
  Blob(Blob &&other);
  ~Blob();

  operator PBYTE();
  operator LPCBYTE() const;
  Blob &operator=(Blob &&other);
  DWORD Size() const;
  bool Alloc(DWORD size);
  void Dump(std::wostream &os, size_t width, size_t ellipsis) const;
};
