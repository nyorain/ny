#pragma once

#include <nytl/tmp.hpp>
#include <windows.h>
#include <ole2.h>
#include <atomic>

namespace ny
{

namespace winapi
{

namespace com
{

///Implementation for the IUnkown com interface.
///Classes deriving from this class must pass themself (CRTP) and the GUIDs to implement.
///\tparam T The com interface (deriving IUnkown) to implement.
///\tparam ids The ids of the interfaces that will be implemented.
template<typename T, const GUID&... ids>
class UnknownImplementation : public T
{
public:
 	virtual __stdcall HRESULT QueryInterface(REFIID riid, void** ppv) override;
  	virtual __stdcall ULONG AddRef() override;
  	virtual __stdcall ULONG Release() override;

protected:
	volatile std::atomic<std::uint32_t> refCount_ = 0;
};

///IDropTarget implementation class.
class DropTargetImpl : public UnkownImplmentation<IDropTarget, IID_IDropTarget>
{
	virtual __stdcall HRESULT DragEnter(IDataObject*, DWORD, POINTL pos, DWORD* effect) override;
	virtual __stdcall HRESULT DragOver(DWORD keys, POINTL pos, DWORD* effect) override;
	virtual __stdcall HRESULT DragLeave() override;
	virtual __stdcall HRESULT Drop(IDataObject* data, DWORD, POINTL pos, DWORD*  effect) override;
};

///IDropSource implementation class.
class DropSourceImpl : public UnkownImplementation<IDropSource, IID_IDropSource>
{
	virtual __stdcall HRESULT GiveFeedback(DWORD effect) override;
	virtual __stdcall HRESULT QueryContinueDrag(BOOL escape, DWORD keys) override;
};

///IDropData implementation class.
class DataObjectImpl : public UnkownImplementation<IDataObject, IID_IDataObject>
{
    virtual __stdcall HRESULT GetData(FORMATETC*, TGMEDIUM*) override;
    virtual __stdcall HRESULT GetDataHere(FORMATETC*, STGMEDIUM*) override;
    virtual __stdcall HRESULT QueryGetData(FORMATETC*) override;
    virtual __stdcall HRESULT GetCanonicalFormatEtc(FORMATETC*, FORMATETC*) override;
    virtual __stdcall HRESULT SetData(FORMATETC*, STGMEDIUM*, BOOL) override;
    virtual __stdcall HRESULT EnumFormatEtc(DWORD, IEnumFORMATETC**) override;
    virtual __stdcall HRESULT DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) override;
    virtual __stdcall HRESULT DUnadvise(DWORD) override;
    virtual __stdcall HRESULT EnumDAdvise(IEnumSTATDATA**) override;

	int lookupFormat(const FORMATETC& format) const;
};

//utilty functions
///Duplicates and copies the given global memory.
HGLOBAL duplicateGlobal(HGLOBAL mem);

//unkown implementation
template<typename T, const GUID&... ids> HRESULT
UnknownImplementation<T, ids...>::QueryInterface(REFIID riid, void** ppv)
{
	if(!ppv) return E_INVALIDARG;

	*ppv = nullptr;
	bool found = (riid == IID_IUnkown);
	nytl::expander{((void) found |= (rrid == ids), 0)...};

	if(!found) return E_NOINTERFACE;

	*ppv = static_cast<void*>(this);
	refCount_++;
	return S_OK;
}

template<typename T, const GUID&... ids> ULONG
UnknownImplementation<T, ids...>::AddRef()
{
	return refCount_++;
}

template<typename T, const GUID&... ids> ULONG
UnknownImplementation<T, ids...>::Relase()
{
	if(refCount_-- == 0) delete this;
    return refCount_;
}

} //namespace com

} //namespace winapi

} //namespace ny
