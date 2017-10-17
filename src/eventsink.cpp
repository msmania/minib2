#include <windows.h>
#include <atlbase.h>
#include <exdispid.h>
#include <exdisp.h>
#include <mshtml.h>
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

static bool InjectScriptElement(CComQIPtr<IWebBrowser2> &wb,
                                LPCWSTR scriptText) {
  CComPtr<IDispatch> dispatch;
  if (SUCCEEDED(wb->get_Document(&dispatch))) {
    CComBSTR tagScript(L"script");
    CComBSTR tagText(L"text");
    CComBSTR tagHead(L"head");
    CComQIPtr<IHTMLDOMNode> scriptNode;
    if (CComQIPtr<IHTMLDocument2> doc = dispatch) {
      CComPtr<IHTMLElement> scriptElem;
      if (SUCCEEDED(doc->createElement(tagScript, &scriptElem))) {
        CComVariant text(scriptText);
        if (SUCCEEDED(scriptElem->setAttribute(tagText, text))) {
          scriptNode = scriptElem;
        }
      }
    }
    if (scriptNode) {
      if (CComQIPtr<IHTMLDocument3> doc = dispatch) {
        CComPtr<IHTMLElementCollection> elements;
        long num_elements = 0;
        if (SUCCEEDED(doc->getElementsByTagName(tagHead, &elements))
            && SUCCEEDED(elements->get_length(&num_elements))
            && num_elements > 0) {
          CComPtr<IDispatch> element0;
          CComVariant zero(0);
          if (SUCCEEDED(elements->item(zero, zero, &element0))) {
            if (CComQIPtr<IHTMLDOMNode> node = element0) {
              CComPtr<IHTMLDOMNode> appended;
              if (SUCCEEDED(node->appendChild(scriptNode, &appended))) {
                return true;
              }
            }
          }
        }
      }
    }
  }
  return false;
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
    if (pDispParams->cArgs == 2
        && pDispParams->rgvarg[0].vt == (VT_VARIANT | VT_BYREF)
        && pDispParams->rgvarg[0].pvarVal
        && pDispParams->rgvarg[0].pvarVal->vt == VT_BSTR
        && pDispParams->rgvarg[1].vt == VT_DISPATCH) {
      if (CComQIPtr<IWebBrowser2> wb = pDispParams->rgvarg[1].pdispVal) {
        Log(L"Received DWebBrowserEvents2.DocumentComplete: %s\n",
            pDispParams->rgvarg[0].pvarVal->bstrVal);
        LPCWSTR SuppressAlert = L"window.alert=function(){};"
                                L"window.confirm=function(){};"
                                L"window.open=function(){};"
                                L"window.close=function(){};";
        if (!InjectScriptElement(wb, SuppressAlert)) {
          Log(L"Failed to inject the script code.\n");
        }
        PostMessage(GetParent(container_),
                    WM_COMMAND,
                    MAKELONG(ID_DEBUG_SCREENSHOT_EVENT, 0),
                    0);
      }
    }
    break;
  default:
    hr = E_NOTIMPL;
    break;
  }
  return hr;
}
