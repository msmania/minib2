class AddressBar : public BaseWindow<AddressBar> {
private:
  const int BUTTON_WIDTH = 30;

  class AddressEdit {
  private:
    static LRESULT CALLBACK SubProc(HWND hwnd,
                                    UINT msg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    UINT_PTR uIdSubclass,
                                    DWORD_PTR dwRefData);
    LRESULT CALLBACK SubProcInternal(HWND hwnd,
                                     UINT msg,
                                     WPARAM w,
                                     LPARAM l,
                                     UINT_PTR id);
    HWND hwnd_;
  public:
    AddressEdit();
    bool Create(HWND parent, const RECT &rect, LPCWSTR caption);
    operator HWND() const;
  };

  AddressEdit edit_;
  HWND buttonBack_;
  HWND buttonRefresh_;
  HWND buttonForward_;

  bool InitChildControls();
  void Resize();

public:
  AddressBar();
  LPCWSTR ClassName() const;
  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l);
  CComBSTR GetUrlText() const;
};
