class Blob {
private:
  HANDLE heap_;
  LPVOID buffer_;
  SIZE_T size_;

  void Release();

public:
  Blob();
  Blob(SIZE_T size);
  Blob(Blob &&other);
  ~Blob();

  operator PBYTE();
  operator LPCBYTE() const;
  template<class T> T *As() {
    return reinterpret_cast<T*>(buffer_);
  }
  template<class T> const T *As() const {
    return reinterpret_cast<const T*>(buffer_);
  }
  Blob &operator=(Blob &&other);
  SIZE_T Size() const;
  bool Alloc(SIZE_T size);
  void Dump(std::wostream &os, size_t width, size_t ellipsis) const;
};
