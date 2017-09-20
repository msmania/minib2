#include <windows.h>
#include <atlbase.h>
#include <ExDispid.h>
#include "resource.h"
#include "eventsink.h"

void Log(LPCWSTR format, ...);

EventSink::EventSink(HWND container)
  : ref_(1), container_(container)
{}

EventSink::~EventSink() {
  Log(L"%s\n", __FUNCTIONW__);
}

STDMETHODIMP EventSink::QueryInterface(REFIID riid, void **ppvObject) {
  const QITAB QITable[] = {
    QITABENT(EventSink, IDispatch),
    { 0 },
  };
  return QISearch(this, QITable, riid, ppvObject);
}

STDMETHODIMP_(ULONG) EventSink::AddRef() {
  return InterlockedIncrement(&ref_);
}

STDMETHODIMP_(ULONG) EventSink::Release() {
  auto cref = InterlockedDecrement(&ref_);
  if (cref == 0) {
    delete this;
  }
  return cref;
}

STDMETHODIMP EventSink::GetTypeInfoCount(__RPC__out UINT *pctinfo) {
  return E_NOTIMPL;
}

STDMETHODIMP EventSink::GetTypeInfo(UINT iTInfo,
                                    LCID lcid,
                                    __RPC__deref_out_opt ITypeInfo **ppTInfo) {
  return E_NOTIMPL;
}

STDMETHODIMP EventSink::GetIDsOfNames(__RPC__in REFIID riid,
                                      __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
                                      __RPC__in_range(0, 16384) UINT cNames,
                                      LCID lcid,
                                      __RPC__out_ecount_full(cNames) DISPID *rgDispId) {
  return E_NOTIMPL;
}

STDMETHODIMP EventSink::Invoke(_In_  DISPID dispIdMember,
                               _In_  REFIID riid,
                               _In_  LCID lcid,
                               _In_  WORD wFlags,
                               _In_  DISPPARAMS *pDispParams,
                               _Out_opt_  VARIANT *pVarResult,
                               _Out_opt_  EXCEPINFO *pExcepInfo,
                               _Out_opt_  UINT *puArgErr) {
  HRESULT hr = S_OK;
  switch (dispIdMember) {
  case DISPID_DOCUMENTCOMPLETE:
    PostMessage(GetParent(container_),
                WM_COMMAND,
                MAKELONG(ID_DEBUG_SCREENSHOT_EVENT, 0),
                0);
    break;
  default:
    hr = E_NOTIMPL;
    break;
  }
  return hr;
}
