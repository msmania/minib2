#include <windows.h>
#include <windowsx.h>
#include <strsafe.h>
#include <atlbase.h>
#include <exdisp.h>
#include <mshtmhst.h>
#include <mshtml.h>
#include <shobjidl.h>
#include <memory>
#include <fstream>
#include "resource.h"
#include "blob.h"
#include "bitmap.h"
#include "basewindow.h"
#include "site.h"
#include "addressbar.h"

void Log(LPCWSTR format, ...) {
  WCHAR linebuf[1024];
  va_list v;
  va_start(v, format);
  StringCbVPrintf(linebuf, sizeof(linebuf), format, v);
  OutputDebugString(linebuf);
}

class BrowserContainer : public BaseWindow<BrowserContainer> {
private:
  CComPtr<OleSite> site_;
  CComPtr<IWebBrowser2> wb_;

  HRESULT ActivateBrowser() {
    site_.Attach(new OleSite(hwnd()));
    if (!site_) {
      return E_POINTER;
    }

    HRESULT hr = wb_.CoCreateInstance(CLSID_WebBrowser);
    if (SUCCEEDED(hr)) {
      CComPtr<IOleObject> ole;
      hr = wb_.QueryInterface(&ole);
      if (SUCCEEDED(hr)) {
        hr = OleSetContainedObject(ole, TRUE);
        if (SUCCEEDED(hr)) {
          hr = ole->SetClientSite(site_);
          if (SUCCEEDED(hr)) {
            RECT rc;
            GetClientRect(hwnd(), &rc);
            hr = ole->DoVerb(OLEIVERB_INPLACEACTIVATE,
                             nullptr,
                             site_,
                             -1,
                             hwnd(),
                             &rc);
          }
        }
      }
    }
    return hr;
  }

  void OnDestroy() {
    if (CComQIPtr<IOleObject> ole = wb_) {
      ole->SetClientSite(nullptr);
    }
  }

  void Resize() {
    if (CComQIPtr<IOleInPlaceObject> inplace = wb_) {
      RECT clientArea;
      GetClientRect(hwnd(), &clientArea);
      inplace->SetObjectRects(&clientArea, &clientArea);
    }
  }

public:
  ~BrowserContainer() {
    Log(L"> %s\n", __FUNCTIONW__);
    Log(L"  OleSite      = %p\n", static_cast<LPVOID>(site_));
    Log(L"  IWebBrowser2 = %p\n", static_cast<LPVOID>(wb_));
  }

  LPCWSTR ClassName() const {
    return L"Minibrowser2 Container";
  }

  IWebBrowser2 *GetBrowser() {
    return wb_;
  }

  LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT ret = 0;
    switch (uMsg) {
    case WM_CREATE:
      if (FAILED(ActivateBrowser())) {
        ret = -1;
      }
      break;
    case WM_DESTROY:
      OnDestroy();
      break;
    case WM_SIZE:
      Resize();
      break;
    default:
      ret = DefWindowProc(hwnd(), uMsg, wParam, lParam);
    }
    return ret;
  }
};

class MainWindow : public BaseWindow<MainWindow> {
private:
  const int ADDRESSBAR_HEIGHT = 20;

  BrowserContainer container_;
  AddressBar addressBar_;

  bool InitChildControls() {
    RECT parentRect, addressbarArea;
    GetClientRect(hwnd(), &parentRect);
    SetRect(&addressbarArea,
            0, 0,
            parentRect.right - parentRect.left,
            ADDRESSBAR_HEIGHT);
    addressBar_.Create(L"AddressBar",
                       WS_VISIBLE | WS_CHILD,
                       /*style_ex*/0,
                       0, 0,
                       parentRect.right - parentRect.left,
                       ADDRESSBAR_HEIGHT,
                       hwnd(),
                       /*menu*/nullptr);
    container_.Create(L"Container",
                      WS_VISIBLE | WS_CHILD,
                      /*style_ex*/0,
                      0, ADDRESSBAR_HEIGHT,
                      parentRect.right - parentRect.left,
                      parentRect.bottom - parentRect.top - ADDRESSBAR_HEIGHT,
                      hwnd(),
                      /*menu*/nullptr);
    return addressBar_.hwnd() && container_.hwnd();
  }

  void Resize() {
    RECT clientSize;
    GetClientRect(hwnd(), &clientSize);
    if (addressBar_.hwnd()) {
      MoveWindow(addressBar_.hwnd(),
                 0, 0,
                 clientSize.right - clientSize.left,
                 ADDRESSBAR_HEIGHT,
                 /*bRepaint*/FALSE);
    }
    if (container_.hwnd()) {
      MoveWindow(container_.hwnd(),
                 0, ADDRESSBAR_HEIGHT,
                 clientSize.right - clientSize.left,
                 clientSize.bottom - clientSize.top - ADDRESSBAR_HEIGHT,
                 /*bRepaint*/FALSE);
    }
  }

  void DumpInfo() {
    Log(L"> %s\n", __FUNCTIONW__);
    if (CComPtr<IWebBrowser2> wb = container_.GetBrowser()) {
      Log(L"  WebBrowser = %p\n", static_cast<LPVOID>(wb));
      CComPtr<IDispatch> dispatch;
      if (SUCCEEDED(wb->get_Document(&dispatch))) {
        Log(L"  Document   = %p\n", static_cast<LPVOID>(dispatch));
      }
    }
  }

  CComPtr<IFileSaveDialog> savedialog_;

  bool ShowSaveDialog(LPCWSTR defaultName,
                      std::wstring &filepath) {
    bool ret = false;
    filepath = L"";
    if (savedialog_ == nullptr) {
      if (SUCCEEDED(savedialog_.CoCreateInstance(CLSID_FileSaveDialog))) {
        static const COMDLG_FILTERSPEC filetypes[] = {
          {L"24-bit Bitmap", L"*.bmp;*.dib"},
        };
        savedialog_->SetFileTypes(ARRAYSIZE(filetypes), filetypes);
        savedialog_->SetDefaultExtension(L"bmp");
      }
    }

    if (savedialog_) {
      savedialog_->SetFileName(defaultName);
      CComPtr<IShellItem> item;
      if (SUCCEEDED(savedialog_->Show(hwnd()))
          && SUCCEEDED(savedialog_->GetResult(&item))) {
        LPWSTR displayName = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &displayName))) {
          ret = true;
          filepath = displayName;
          CoTaskMemFree(displayName);
        }
      }
    }
    return ret;
  }

  void ScreenShot() {
    if (CComPtr<IWebBrowser2> wb = container_.GetBrowser()) {
      long width, height;
      if (SUCCEEDED(wb->get_Width(&width))
          && width > 0
          && SUCCEEDED(wb->get_Height(&height))
          && height > 0) {
        RECT scrollerRect;
        SetRect(&scrollerRect, 0, 0, width, height);

        std::wstring output;
        if (!ShowSaveDialog(L"screenshot", output))
          return;

        if (auto memDC = SafeDC::CreateMemDC(hwnd())) {
          if (auto dib = DIB::CreateNew(memDC,
                                        /*bitCount*/24,
                                        width,
                                        height)) {
            auto oldBitmap = SelectBitmap(memDC, dib);
            HRESULT hr = OleDraw(wb, DVASPECT_CONTENT, memDC, &scrollerRect);
            if (SUCCEEDED(hr)) {
              std::ofstream os(output, std::ios::binary);
              if (os.is_open()) {
                dib.Save(os);
              }
            }
            else {
              Log(L"OleDraw failed - %08x\n", hr);
            }
            SelectBitmap(memDC, oldBitmap);
          }
        }
      }
    }
  }

public:
  LPCWSTR ClassName() const {
    return L"Minibrowser2 MainWindow";
  }

  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) {
    LRESULT ret = 0;
    switch (msg) {
    case WM_CREATE:
      if (!InitChildControls()) {
        return -1;
      }
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_SIZE:
      Resize();
      break;
    case WM_COMMAND:
      switch (LOWORD(w)) {
      case ID_BROWSE:
        if (CComPtr<IWebBrowser> wb = container_.GetBrowser()) {
          wb->Navigate(addressBar_.GetUrlText(), nullptr, nullptr, nullptr, nullptr);
        }
        break;
      case ID_BROWSE_BACK:
        if (CComPtr<IWebBrowser2> wb = container_.GetBrowser()) {
          wb->GoBack();
        }
        break;
      case ID_BROWSE_FORWARD:
        if (CComPtr<IWebBrowser2> wb = container_.GetBrowser()) {
          wb->GoForward();
        }
        break;
      case ID_BROWSE_REFRESH:
        if (CComPtr<IWebBrowser2> wb = container_.GetBrowser()) {
          CComVariant level(REFRESH_NORMAL);
          wb->Refresh2(&level);
        }
        break;
      case ID_DEBUG_DUMPINFO:
        DumpInfo();
        break;
      case ID_DEBUG_SCREENSHOT:
        ScreenShot();
        break;
      default:
        ret = DefWindowProc(hwnd(), msg, w, l);
        break;
      }
      break;
    default:
      ret = DefWindowProc(hwnd(), msg, w, l);
      break;
    }
    return ret;
  }
};

int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE,
                    PWSTR pCmdLine,
                    int nCmdShow) {
  const auto flags = COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE;
  if (SUCCEEDED(CoInitializeEx(nullptr, flags))) {
    if (auto p = std::make_unique<MainWindow>()) {
      if (p->Create(L"Minibrowser2",
                    WS_OVERLAPPEDWINDOW,
                    /*style_ex*/0,
                    CW_USEDEFAULT, 0,
                    486, 300,
                    /*parent*/nullptr,
                    MAKEINTRESOURCE(IDR_MAINMENU))) {
        ShowWindow(p->hwnd(), nCmdShow);
        MSG msg = {0};
        while (GetMessage(&msg, nullptr, 0, 0)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
    }
    CoUninitialize();
  }
  return 0;
}