#include <vector>
#include <map>
#include <string>
#include <windows.h>
#include <atlbase.h>

void Log(LPCWSTR format, ...);

static constexpr DISPID FirstMethodId = 100;

class CustomFactory : public IClassFactory {
  ULONG ref_;

public:
  CustomFactory() : ref_(1) {
    //Log(L"%s: %p\n", __FUNCTIONW__, this);
  }

  virtual ~CustomFactory() {
    //Log(L"%s: %p\n", __FUNCTIONW__, this);
  }

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) {
  const QITAB QITable[] = {
    QITABENT(CustomFactory, IClassFactory),
    { 0 },
  };
  return QISearch(this, QITable, riid, ppvObject);
  }
  STDMETHOD_(ULONG, AddRef)() {
    Log(L"%s: %d -> %d\n", __FUNCTIONW__, ref_, ref_ + 1);
    return InterlockedIncrement(&ref_);
  }
  STDMETHOD_(ULONG, Release)() {
    Log(L"%s: %d -> %d\n", __FUNCTIONW__, ref_, ref_ - 1);
    auto cref = InterlockedDecrement(&ref_);
    if (cref == 0) {
      delete this;
    }
    return cref;
  }

  // IClassFactory
  STDMETHOD(CreateInstance)(
    _Inout_opt_ LPUNKNOWN pUnkOuter,
    _In_ REFIID riid,
    _COM_Outptr_ void** ppvObj)
  {
    if (!ppvObj)
      return E_POINTER;

    *ppvObj = nullptr;

    return E_NOTIMPL;
  }

  STDMETHOD(LockServer)(_In_ BOOL fLock) {
    Log(L"%s: %c\n", __FUNCTIONW__, fLock ? '+' : '-');
    return S_OK;
  }
};

CustomFactory g_factory;

void RegisterProtocol() {
  CComPtr<IInternetSession> session;
  HRESULT hr = CoInternetGetSession(0, &session, 0);
  if (SUCCEEDED(hr)) {
    hr = session->RegisterNameSpace(&g_factory,
                                    CLSID_AboutProtocol,
                                    L"about",
                                    /*cPatterns*/0,
                                    /*ppwzPatterns*/nullptr,
                                    /*dwReserved*/0);
    hr = session->UnregisterNameSpace(&g_factory, L"about");
  }
}

class ExternalSink : public IDispatch {
private:
  struct MethodContext {
    DISPPARAMS *params;
    VARIANT *result;
    EXCEPINFO *exception;
    UINT *argument_error;
  };

  ULONG ref_;
  std::map<std::wstring, DISPID> mappings_;
  std::vector<HRESULT(ExternalSink::*)(MethodContext&)> methods_;

  HRESULT Log(MethodContext &context) {
    ::Log(L"%s\n", __FUNCTIONW__);
    return S_OK;
  }

  HRESULT Hello(MethodContext &context) {
    HRESULT hr = E_INVALIDARG;
    if (context.params->cArgs == 1
        && context.params->rgvarg[0].vt == VT_BSTR) {
      ::Log(L"%s\n", V_BSTR(&context.params->rgvarg[0]));
      RegisterProtocol();
      hr = S_OK;
    }
    return hr;
  }

  void InitDispatchTable() {
    constexpr struct {
      LPCWSTR name;
      HRESULT (ExternalSink::*method)(MethodContext&);
    } static_map[] = {
      {L"log", &ExternalSink::Log},
      {L"hello", &ExternalSink::Hello},
    };
    DISPID id = FirstMethodId;
    for (auto &it : static_map) {
      mappings_[it.name] = id++;
      methods_.push_back(it.method);
    }
  }

public:
  ExternalSink() : ref_(1) {
    InitDispatchTable();
  }

  ~ExternalSink() {
    ::Log(L"%s\n", __FUNCTIONW__);
  }

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) {
    const QITAB QITable[] = {
      QITABENT(ExternalSink, IDispatch),
      { 0 },
    };
    return QISearch(this, QITable, riid, ppvObject);
  }

  STDMETHOD_(ULONG, AddRef)() {
    return InterlockedIncrement(&ref_);
  }

  STDMETHOD_(ULONG, Release)() {
    auto cref = InterlockedDecrement(&ref_);
    if (cref == 0) {
      delete this;
    }
    return cref;
  }

  // IDispatch
  IFACEMETHODIMP GetTypeInfoCount(__RPC__out UINT *pctinfo) {
    return E_NOTIMPL;
  }

  IFACEMETHODIMP GetTypeInfo(UINT iTInfo,
                             LCID lcid,
                             __RPC__deref_out_opt ITypeInfo **ppTInfo) {
    return E_NOTIMPL;
  }

  IFACEMETHODIMP GetIDsOfNames(__RPC__in REFIID riid,
                               __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
                               __RPC__in_range(0, 16384) UINT cNames,
                               LCID lcid,
                               __RPC__out_ecount_full(cNames) DISPID *rgDispId) {
    for (UINT i = 0; i < cNames; ++i) {
      auto it = mappings_.find(rgszNames[i]);
      if (it == mappings_.end())
        return DISP_E_UNKNOWNNAME;
      rgDispId[i] = it->second;
    }
    return S_OK;
  }

  IFACEMETHODIMP Invoke(_In_ DISPID dispIdMember,
                        _In_ REFIID riid,
                        _In_ LCID lcid,
                        _In_ WORD wFlags,
                        _In_ DISPPARAMS *pDispParams,
                        _Out_opt_ VARIANT *pVarResult,
                        _Out_opt_ EXCEPINFO *pExcepInfo,
                        _Out_opt_ UINT *puArgErr) {
    HRESULT hr = E_INVALIDARG;
    dispIdMember -= FirstMethodId;
    if (dispIdMember >= 0
        && dispIdMember < static_cast<DISPID>(methods_.size())) {
      hr = S_OK;
      auto &method = methods_[dispIdMember];
      if (wFlags & DISPATCH_METHOD) {
        MethodContext context{pDispParams, pVarResult, pExcepInfo, puArgErr};
        hr = (this->*method)(context);
      }
      else if (wFlags & DISPATCH_PROPERTYGET && pVarResult) {
        *pVarResult = CComVariant(true);
      }
    }
    return hr;
  }
};

IDispatch *CreateExternalSink() {
  return new ExternalSink();
}
