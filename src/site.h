class OleSite : public IOleClientSite,
                public IOleInPlaceSite,
                public IDocHostUIHandler {
private:
  ULONG ref_;
  HWND hwnd_;
  CComPtr<IDispatch> external_;

public:
  OleSite(HWND hwnd, IDispatch *external);
  ~OleSite();

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  // IOleClientSite
  IFACEMETHODIMP SaveObject();
  IFACEMETHODIMP GetMoniker(DWORD dwAssign,
                            DWORD dwWhichMoniker,
                            __RPC__deref_out_opt IMoniker **ppmk);
  IFACEMETHODIMP GetContainer(__RPC__deref_out_opt IOleContainer **ppContainer);
  IFACEMETHODIMP ShowObject();
  IFACEMETHODIMP OnShowWindow(BOOL fShow);
  IFACEMETHODIMP RequestNewObjectLayout();

  // IOleWindow (via IOleInPlaceSite)
  IFACEMETHODIMP GetWindow(__RPC__deref_out_opt HWND *phwnd);
  IFACEMETHODIMP ContextSensitiveHelp(
    BOOL fEnterMode);

  // IOleInPlaceSite
  IFACEMETHODIMP CanInPlaceActivate();
  IFACEMETHODIMP OnInPlaceActivate();
  IFACEMETHODIMP OnUIActivate();
  IFACEMETHODIMP GetWindowContext(__RPC__deref_out_opt IOleInPlaceFrame **ppFrame,
                                  __RPC__deref_out_opt IOleInPlaceUIWindow **ppDoc,
                                  __RPC__out LPRECT lprcPosRect,
                                  __RPC__out LPRECT lprcClipRect,
                                  __RPC__inout LPOLEINPLACEFRAMEINFO lpFrameInfo);
  IFACEMETHODIMP Scroll(SIZE scrollExtant);
  IFACEMETHODIMP OnUIDeactivate(BOOL fUndoable);
  IFACEMETHODIMP OnInPlaceDeactivate();
  IFACEMETHODIMP DiscardUndoState();
  IFACEMETHODIMP DeactivateAndUndo();
  IFACEMETHODIMP OnPosRectChange(__RPC__in LPCRECT lprcPosRect);

  // IDocHostUIHandler
  IFACEMETHODIMP ShowContextMenu(_In_ DWORD dwID,
                                 _In_ POINT *ppt,
                                 _In_ IUnknown *pcmdtReserved,
                                 _In_ IDispatch *pdispReserved);
  IFACEMETHODIMP GetHostInfo(_Inout_ DOCHOSTUIINFO *pInfo);
  IFACEMETHODIMP ShowUI(DWORD dwID,
                        _In_ IOleInPlaceActiveObject *pActiveObject,
                        _In_ IOleCommandTarget *pCommandTarget,
                        _In_ IOleInPlaceFrame *pFrame,
                        _In_ IOleInPlaceUIWindow *pDoc);
  IFACEMETHODIMP HideUI();
  IFACEMETHODIMP UpdateUI();
  IFACEMETHODIMP EnableModeless(BOOL fEnable);
  IFACEMETHODIMP OnDocWindowActivate(BOOL fActivate);
  IFACEMETHODIMP OnFrameWindowActivate(BOOL fActivate);
  IFACEMETHODIMP ResizeBorder(_In_ LPCRECT prcBorder,
                              _In_ IOleInPlaceUIWindow *pUIWindow,
                              _In_ BOOL fRameWindow);
  IFACEMETHODIMP TranslateAccelerator(LPMSG lpMsg,
                                      const GUID *pguidCmdGroup,
                                      DWORD nCmdID);
  IFACEMETHODIMP GetOptionKeyPath(_Out_ LPOLESTR *pchKey,
                                  DWORD dw);
  IFACEMETHODIMP GetDropTarget(_In_ IDropTarget *pDropTarget,
                               _Outptr_ IDropTarget **ppDropTarget);
  IFACEMETHODIMP GetExternal(_Outptr_result_maybenull_ IDispatch **ppDispatch);
  IFACEMETHODIMP TranslateUrl(DWORD dwTranslate,
                              _In_ LPWSTR pchURLIn,
                              _Outptr_ LPWSTR *ppchURLOut);
  IFACEMETHODIMP FilterDataObject(_In_ IDataObject *pDO,
                                  _Outptr_result_maybenull_ IDataObject **ppDORet);
};
