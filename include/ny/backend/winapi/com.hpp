#pragma once

#include <ny/include.hpp>
#include <nytl/tmp.hpp>
#include <windows.h>
#include <ole2.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace ny
{

namespace winapi
{

namespace com
{

///Smart pointer wrapper for shared com objects.
///Automatically adds a reference on construction and removes it on destruction.
template<typename T>
class ComObject
{
public:
	ComObject(T& obj)	: obj_(&obj) { obj_->AddRef(); }
	~ComObject() { if(obj_) obj_->Release(); }

	T& get() { return *obj_; };
	const T& get() const { return *obj_; }

	T& operator*() { return *get(); }
	const T& operator*() const { return *get(); }

	T* operator->() { return get(); }
	const T* operator->() const { return get(); }

	operator bool() const { return (obj_); }

protected:
	T* obj_ = nullptr;
};

using UnknownComObject = ComObject<IUnknown>;
using DataComObject = ComObject<IDataObject>;

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
	volatile std::atomic<unsigned int> refCount_ {0};
};

///IDropTarget implementation class.
class DropTargetImpl : public UnknownImplementation<IDropTarget, IID_IDropTarget>
{
public:
	virtual __stdcall HRESULT DragEnter(IDataObject*, DWORD, POINTL pos, DWORD* effect) override;
	virtual __stdcall HRESULT DragOver(DWORD keys, POINTL pos, DWORD* effect) override;
	virtual __stdcall HRESULT DragLeave() override;
	virtual __stdcall HRESULT Drop(IDataObject* data, DWORD, POINTL pos, DWORD*  effect) override;
};

///IDropSource implementation class.
class DropSourceImpl : public UnknownImplementation<IDropSource, IID_IDropSource>
{
	virtual __stdcall HRESULT GiveFeedback(DWORD effect) override;
	virtual __stdcall HRESULT QueryContinueDrag(BOOL escape, DWORD keys) override;
};

///IDropData implementation class.
class DataObjectImpl : public UnknownImplementation<IDataObject, IID_IDataObject>
{
public:
	DataObjectImpl(std::unique_ptr<DataSource> src);

    virtual __stdcall HRESULT GetData(FORMATETC*, STGMEDIUM*) override;
    virtual __stdcall HRESULT GetDataHere(FORMATETC*, STGMEDIUM*) override;
    virtual __stdcall HRESULT QueryGetData(FORMATETC*) override;
    virtual __stdcall HRESULT GetCanonicalFormatEtc(FORMATETC*, FORMATETC*) override;
    virtual __stdcall HRESULT SetData(FORMATETC*, STGMEDIUM*, BOOL) override;
    virtual __stdcall HRESULT EnumFormatEtc(DWORD, IEnumFORMATETC**) override;
    virtual __stdcall HRESULT DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) override;
    virtual __stdcall HRESULT DUnadvise(DWORD) override;
    virtual __stdcall HRESULT EnumDAdvise(IEnumSTATDATA**) override;

protected:
	///Returns the type id of the supported types that matches the given format, or -1.
	int lookupFormat(const FORMATETC& format) const;

	///Returns a FORMATETC struct for the given supported type id.
	///\exception std::out_of_range When id > supportedTypes.size()
	FORMATETC format(unsigned int id) const;
	bool format(unsigned int id, FORMATETC& format) const;

	///Returns all supported formats in a vector.
	std::vector<FORMATETC> formats() const;

	///Returns a STGMEDIUM struct for the given supported type id holding the data.
	///\exception std::out_of_range When id > supportedTypes.size()
	STGMEDIUM medium(unsigned int id) const;
	bool medium(unsigned int id, STGMEDIUM& med) const;

protected:
	std::unique_ptr<DataSource> source_;
};

//utilty functions
///Changes line endings
void replaceLF(std::string& string);
void replaceCRLF(std::string& string);

///Creates a global memory object for the given string.
HGLOBAL stringToGlobal(const std::string& string);
HGLOBAL stringToGlobalUnicode(const std::u16string& string);

///Copies the data from a global memory object to a string.
std::u16string globalToStringUnicode(HGLOBAL global);
std::string globalToString(HGLOBAL global);

//unkown implementation
template<typename T, const GUID&... ids> HRESULT
UnknownImplementation<T, ids...>::QueryInterface(REFIID riid, void** ppv)
{
	if(!ppv) return E_INVALIDARG;

	*ppv = nullptr;
	bool found = (riid == IID_IUnknown);
	nytl::Expand {(found |= (riid == ids), 0)...};

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
UnknownImplementation<T, ids...>::Release()
{
	if(refCount_-- == 0) delete this;
    return refCount_;
}

} //namespace com

} //namespace winapi

} //namespace ny
