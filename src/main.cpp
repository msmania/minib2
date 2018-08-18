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
#include "eventsink.h"

IDispatch *CreateExternalSink();

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
  CComPtr<EventSink> events_;
  CComPtr<IDispatch> external_;
  CComPtr<IWebBrowser2> wb_;

  HRESULT ActivateBrowser() {
    external_.Attach(CreateExternalSink());
    site_.Attach(new OleSite(hwnd(), external_));
    if (!site_ || !external_) {
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

  HRESULT ConnectEventSink() {
    events_.Attach(new EventSink(hwnd()));
    if (!events_) {
      return E_POINTER;
    }

    HRESULT hr = E_POINTER;
    if (CComQIPtr<IConnectionPointContainer> cpc = wb_) {
      CComPtr<IConnectionPoint> cp;
      hr = cpc->FindConnectionPoint(DIID_DWebBrowserEvents2, &cp);
      if (SUCCEEDED(hr)) {
        DWORD cookie;
        hr = cp->Advise(events_, &cookie);
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
    Log(L"> %s %p\n", __FUNCTIONW__, this);
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
      if (FAILED(ActivateBrowser())
          || FAILED(ConnectEventSink())) {
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

  std::unique_ptr<BrowserContainer> container_;
  AddressBar addressBar_;
  CComPtr<IFileSaveDialog> savedialog_;

  struct Options {
    bool autoCapture;
    Options() : autoCapture(false) {}
  } options_;

  bool InitChildControls() {
    RECT parentRect;
    GetClientRect(hwnd(), &parentRect);
    addressBar_.Create(L"AddressBar",
                       WS_VISIBLE | WS_CHILD,
                       /*style_ex*/0,
                       0, 0,
                       parentRect.right - parentRect.left,
                       ADDRESSBAR_HEIGHT,
                       hwnd(),
                       /*menu*/nullptr);
    return addressBar_.hwnd();
  }

  bool InitBrowser() {
    if (container_) {
      DestroyWindow(container_->hwnd());
    }
    RECT parentRect;
    GetClientRect(hwnd(), &parentRect);
    container_ = std::make_unique<BrowserContainer>();
    container_->Create(L"Container",
                       WS_VISIBLE | WS_CHILD,
                       /*style_ex*/0,
                       0, ADDRESSBAR_HEIGHT,
                       parentRect.right - parentRect.left,
                       parentRect.bottom - parentRect.top - ADDRESSBAR_HEIGHT,
                       hwnd(),
                       /*menu*/nullptr);
    return container_->hwnd();
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
    if (container_ && container_->hwnd()) {
      MoveWindow(container_->hwnd(),
                 0, ADDRESSBAR_HEIGHT,
                 clientSize.right - clientSize.left,
                 clientSize.bottom - clientSize.top - ADDRESSBAR_HEIGHT,
                 /*bRepaint*/FALSE);
    }
  }

  void DumpInfo() {
    if (!container_) return;
    Log(L"> %s\n", __FUNCTIONW__);
    if (CComPtr<IWebBrowser2> wb = container_->GetBrowser()) {
      HWND browserWindow;
      IUnknown_GetWindow(wb, &browserWindow);
      Log(L"  WebBrowser = %p (HWND=%p)\n",
          static_cast<LPVOID>(wb),
          browserWindow);

      CComPtr<IDispatch> dispatch;
      if (SUCCEEDED(wb->get_Document(&dispatch))) {
        IUnknown_GetWindow(dispatch, &browserWindow);
        Log(L"  Document   = %p (HWND=%p)\n",
            static_cast<LPVOID>(dispatch),
            browserWindow);
      }
    }
  }

  bool ShowSaveDialog(LPCWSTR defaultName,
                      std::wstring &filepath) {
    bool ret = false;
    filepath = L"";
    if (savedialog_ == nullptr) {
      if (SUCCEEDED(savedialog_.CoCreateInstance(CLSID_FileSaveDialog))) {
        static const COMDLG_FILTERSPEC filetypes[] = {
          {L"Bitmap", L"*.bmp;*.dib"},
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

  void OleDraw(LPCWSTR output, WORD bitCount) {
    if (!container_) return;
    if (CComPtr<IWebBrowser2> wb = container_->GetBrowser()) {
      long width, height;
      if (SUCCEEDED(wb->get_Width(&width))
          && width > 0
          && SUCCEEDED(wb->get_Height(&height))
          && height > 0) {
        RECT scrollerRect;
        SetRect(&scrollerRect, 0, 0, width, height);

        if (auto memDC = SafeDC::CreateMemDC(hwnd())) {
          if (auto dib = DIB::CreateNew(memDC,
                                        bitCount,
                                        width,
                                        height,
                                        /*section*/nullptr,
                                        /*initWithGrayscaleTable*/true)) {
            auto oldBitmap = SelectBitmap(memDC, dib);
            HRESULT hr = ::OleDraw(wb, DVASPECT_CONTENT, memDC, &scrollerRect);
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

  // https://msdn.microsoft.com/en-us/library/vs/alm/dd183402(v=vs.85).aspx
  void Capture(LPCWSTR output, WORD bitCount) {
    if (!container_) return;
    if (CComPtr<IWebBrowser2> wb = container_->GetBrowser()) {
      HWND targetWindow;
      long width, height;
      if (SUCCEEDED(IUnknown_GetWindow(wb, &targetWindow))
          && SUCCEEDED(wb->get_Width(&width))
          && width > 0
          && SUCCEEDED(wb->get_Height(&height))
          && height > 0) {
        if (HDC target = GetDC(targetWindow)) {
          HANDLE section = nullptr;
          DWORD uw = width, uh = height;
          DIB dib;
          if (bitCount == 8) {
            dib = DIB::CaptureFromHDC(target, 32, uw, uh, section);
            dib.ConvertToGrayscale(target, section);
          }
          else {
            dib = DIB::CaptureFromHDC(target, bitCount, uw, uh, section);
          }
          if (dib) {
            std::ofstream os(output, std::ios::binary);
            if (os.is_open()) {
              dib.Save(os);
            }
            ReleaseDC(targetWindow, target);
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
    std::wstring output;
    switch (msg) {
    case WM_CREATE:
      if (!InitChildControls() || !InitBrowser()) {
        return -1;
      }
      PostMessage(hwnd(), WM_COMMAND, ID_BROWSE, 0);
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_SIZE:
      Resize();
      break;
    case WM_COMMAND:
      switch (LOWORD(w)) {
      case ID_CONTROL_DESTROY:
        if (container_) {
          DestroyWindow(container_->hwnd());
          container_.reset(nullptr);
        }
        break;
      case ID_CONTROL_CREATE:
        InitBrowser();
        break;
      case ID_BROWSE:
        if (container_) {
          container_->GetBrowser()->Navigate(addressBar_.GetUrlText(),
                                             nullptr,
                                             nullptr,
                                             nullptr,
                                             nullptr);
        }
        break;
      case ID_BROWSE_BACK:
        if (container_) {
          container_->GetBrowser()->GoBack();
        }
        break;
      case ID_BROWSE_FORWARD:
        if (container_) {
          container_->GetBrowser()->GoForward();
        }
        break;
      case ID_BROWSE_REFRESH:
        if (container_) {
          CComVariant level(REFRESH_NORMAL);
          container_->GetBrowser()->Refresh2(&level);
        }
        break;
      case ID_DEBUG_DUMPINFO:
        DumpInfo();
        break;
      case ID_DEBUG_OLEDRAW:
        if (ShowSaveDialog(L"oledraw", output)) {
          OleDraw(output.c_str(), /*bitCount*/8);
        }
        break;
      case ID_DEBUG_CAPTURE:
        if (ShowSaveDialog(L"capture", output)) {
          Capture(output.c_str(), /*bitCount*/8);
        }
        break;
      case ID_DEBUG_SCREENSHOT_EVENT:
        if (options_.autoCapture) {
          if (ShowSaveDialog(L"screenshot", output)) {
            OleDraw(output.c_str(), /*bitCount*/8);
          }
        }
        break;
      case ID_OPTIONS_AUTOCAPTURE:
        if (auto menu = GetMenu(hwnd())) {
          options_.autoCapture = !options_.autoCapture;
          MENUITEMINFO mi = { 0 };
          mi.cbSize = sizeof(mi);
          mi.fMask = MIIM_STATE;
          mi.fState = options_.autoCapture ? MFS_CHECKED : MFS_UNCHECKED;
          SetMenuItemInfo(menu, ID_OPTIONS_AUTOCAPTURE, FALSE, &mi);
        }
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

std::wstring DetermineAwarenessLevel(const std::wstring &cmdline) {
  std::wstring suffix;
  if (cmdline.find(L"--v2_process") != std::string::npos) {
    auto ret = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (ret)
      suffix += L" V2_PROCESS";
    else {
      Log(L"SetProcessDpiAwarenessContext failed - %08x\n", GetLastError());
      suffix += L" V2_PROCESS(fail)";
    }
  }
  if (cmdline.find(L"--v2_thread") != std::string::npos) {
    auto ret = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Log(L"SetThreadDpiAwarenessContext - %p\n", ret);
    suffix += L" V2_THREAD";
  }
  if (cmdline.find(L"--systemdpi_process") != std::string::npos) {
    auto ret = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    if (ret)
      suffix += L" SYSDPI_PROCESS";
    else {
      Log(L"SetProcessDpiAwarenessContext failed - %08x\n", GetLastError());
      suffix += L" SYSDPI_PROCESS(fail)";
    }
  }
  if (cmdline.find(L"--systemdpi_thread") != std::string::npos) {
    auto ret = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    Log(L"SetThreadDpiAwarenessContext - %p\n", ret);
    suffix += L" SYSDPI_THREAD";
  }
  return suffix;
}

int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE,
                    PWSTR pCmdLine,
                    int nCmdShow) {
  std::wstring title(L"Minibrowser2 -");
  title += DetermineAwarenessLevel(pCmdLine);

  const auto flags = COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE;
  if (SUCCEEDED(CoInitializeEx(nullptr, flags))) {
    if (auto p = std::make_unique<MainWindow>()) {
      if (p->Create(title.c_str(),
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