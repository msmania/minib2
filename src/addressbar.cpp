#include <windows.h>
#include <commctrl.h>
#include <atlbase.h>
#include "resource.h"
#include "basewindow.h"
#include "addressbar.h"

LRESULT CALLBACK AddressBar::AddressEdit::SubProc(HWND hwnd,
                                                  UINT msg,
                                                  WPARAM wParam,
                                                  LPARAM lParam,
                                                  UINT_PTR uIdSubclass,
                                                  DWORD_PTR dwRefData) {
  if (auto p = reinterpret_cast<AddressEdit*>(dwRefData))
    return p->SubProcInternal(hwnd, msg, wParam, lParam, uIdSubclass);
  else
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK AddressBar::AddressEdit::SubProcInternal(HWND hwnd,
                                                          UINT msg,
                                                          WPARAM w,
                                                          LPARAM l,
                                                          UINT_PTR id) {
  LRESULT ret = 0;
  if (msg == WM_KEYDOWN && w == VK_RETURN) {
    PostMessage(GetParent(hwnd_), WM_COMMAND, MAKELONG(ID_BROWSE, 0), 0);
  }
  else {
    ret = DefSubclassProc(hwnd, msg, w, l);
  }
  return ret;
}

AddressBar::AddressEdit::AddressEdit() : hwnd_(nullptr)
{}

bool AddressBar::AddressEdit::Create(HWND parent,
                                     const RECT &rect,
                                     LPCWSTR caption) {
  hwnd_ = CreateWindow(L"Edit",
                       caption,
                       WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                       rect.left, rect.top,
                       rect.right - rect.left, rect.bottom - rect.top,
                       parent,
                       /*hMenu*/nullptr,
                       GetModuleHandle(nullptr),
                       /*lpParam*/nullptr);
  if (hwnd_) {
    SetWindowSubclass(hwnd_, SubProc, 0, reinterpret_cast<DWORD_PTR>(this));
  }
  return !!hwnd_;
}

AddressBar::AddressEdit::operator HWND() const {
  return hwnd_;
}

bool AddressBar::InitChildControls() {
  RECT clientArea, editArea;
  GetClientRect(hwnd(), &clientArea);
  const int height = clientArea.bottom - clientArea.top;
  const DWORD buttonStyle = WS_VISIBLE | WS_CHILD | BS_FLAT;
  SetRect(&editArea, 0, 0,
          clientArea.right - clientArea.left - BUTTON_WIDTH * 3,
          height);
  edit_.Create(hwnd(), editArea, L"about:blank");
  buttonBack_ = CreateWindow(L"Button",
                             L"<",
                             buttonStyle,
                             editArea.right, 0,
                             BUTTON_WIDTH, height,
                             hwnd(),
                             reinterpret_cast<HMENU>(ID_BROWSE_BACK),
                             GetModuleHandle(nullptr),
                             /*lpParam*/nullptr);
  buttonRefresh_ = CreateWindow(L"Button",
                                L"#",
                                buttonStyle,
                                editArea.right + BUTTON_WIDTH, 0,
                                BUTTON_WIDTH, height,
                                hwnd(),
                                reinterpret_cast<HMENU>(ID_BROWSE_REFRESH),
                                GetModuleHandle(nullptr),
                                /*lpParam*/nullptr);
  buttonForward_ = CreateWindow(L"Button",
                                L">",
                                buttonStyle,
                                editArea.right + BUTTON_WIDTH * 2, 0,
                                BUTTON_WIDTH, height,
                                hwnd(),
                                reinterpret_cast<HMENU>(ID_BROWSE_FORWARD),
                                GetModuleHandle(nullptr),
                                /*lpParam*/nullptr);
  return edit_ && buttonBack_ && buttonRefresh_ && buttonForward_;
}

void AddressBar::Resize() {
  RECT clientArea;
  GetClientRect(hwnd(), &clientArea);
  const int height = clientArea.bottom - clientArea.top;
  MoveWindow(edit_,
              0, 0,
              clientArea.right - clientArea.left - BUTTON_WIDTH * 3, height,
              /*bRepaint*/TRUE);
  MoveWindow(buttonBack_,
              clientArea.right - clientArea.left - BUTTON_WIDTH * 3, 0,
              BUTTON_WIDTH, height,
              /*bRepaint*/TRUE);
  MoveWindow(buttonRefresh_,
             clientArea.right - clientArea.left - BUTTON_WIDTH * 2, 0,
             BUTTON_WIDTH, height,
             /*bRepaint*/TRUE);
  MoveWindow(buttonForward_,
              clientArea.right - clientArea.left - BUTTON_WIDTH, 0,
              BUTTON_WIDTH, height,
              /*bRepaint*/TRUE);
}

AddressBar::AddressBar()
  : buttonBack_(nullptr),
    buttonRefresh_(nullptr),
    buttonForward_(nullptr)
{}

LPCWSTR AddressBar::ClassName() const {
  return L"Minibrowser2 AddressBar";
}

LRESULT AddressBar::HandleMessage(UINT msg, WPARAM w, LPARAM l) {
  LRESULT ret = 0;
  switch (msg) {
  case WM_CREATE:
    if (!InitChildControls()) {
      return -1;
    }
    break;
  case WM_SIZE:
    Resize();
    break;
  case WM_COMMAND:
    switch (LOWORD(w)) {
    case ID_BROWSE:
    case ID_BROWSE_BACK:
    case ID_BROWSE_REFRESH:
    case ID_BROWSE_FORWARD:
      PostMessage(GetParent(hwnd()), WM_COMMAND, w, l);
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

CComBSTR AddressBar::GetUrlText() const {
  CComBSTR text(GetWindowTextLength(edit_) + 1);
  GetWindowText(edit_, text, text.Length());
  return text;
}