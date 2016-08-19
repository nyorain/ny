#pragma once

#include <ny/include.hpp>
#include <ny/base/data.hpp>

#include <windows.h>
#include <ole2.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// #include <experimental/any>
// namespace std { using namespace experimental; }
// #include <any>
#include <ny/base/any.hpp>

namespace ny
{

class WinapiWindowContext;
class WinapiAppContext;

namespace winapi
{

//utilty functions
///Changes line endingsg
void replaceLF(std::string& string);
void replaceCRLF(std::string& string);

///Creates a global memory object for the given string.
HGLOBAL stringToGlobal(const std::string& string);
HGLOBAL stringToGlobalUnicode(const std::u16string& string);

///Copies the data from a global memory object to a string.
std::u16string globalToStringUnicode(HGLOBAL global);
std::string globalToString(HGLOBAL global);

///Converts between a raw buffer and a global
HGLOBAL bufferToGlobal(const nytl::Range<std::uint8_t>& buffer);
std::vector<std::uint8_t> globalToBuffer(HGLOBAL global);

///Converts between dataType format and native winapi clipboard format
//returns cfFormat
unsigned int dataTypeToClipboardFormat(unsigned int dataType, unsigned int& medium);

//returns member of dataType
unsigned int clipboardFormatToDataType(unsigned int cfFormat);

///Converts between std::any holding the dataType data and the native winapi representation.
std::any comToData(unsigned int cfFormat, void* data, unsigned int& dataFormat);
void* dataToCom(unsigned int dataFormat, const std::any& data, unsigned int& cfFormat,
	unsigned int& medium);

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

	T& operator*() { return get(); }
	const T& operator*() const { return get(); }

	T* operator->() { return obj_; }
	const T* operator->() const { return obj_; }

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
 	__stdcall HRESULT QueryInterface(REFIID riid, void** ppv) override;
  	__stdcall ULONG AddRef() override;
  	__stdcall ULONG Release() override;

protected:
	volatile std::atomic<unsigned int> refCount_ {0};
};

///IDropTarget implementation class.
class DropTargetImpl : public UnknownImplementation<IDropTarget, IID_IDropTarget>
{
public:
	DropTargetImpl(WinapiWindowContext& ctx) : windowContext_(&ctx) {}

	__stdcall HRESULT DragEnter(IDataObject*, DWORD, POINTL pos, DWORD* effect) override;
	__stdcall HRESULT DragOver(DWORD keys, POINTL pos, DWORD* effect) override;
	__stdcall HRESULT DragLeave() override;
	__stdcall HRESULT Drop(IDataObject* data, DWORD, POINTL pos, DWORD*  effect) override;

protected:
	WinapiWindowContext* windowContext_;
};

///IDropSource implementation class.
class DropSourceImpl : public UnknownImplementation<IDropSource, IID_IDropSource>
{
	__stdcall HRESULT GiveFeedback(DWORD effect) override;
	__stdcall HRESULT QueryContinueDrag(BOOL escape, DWORD keys) override;
};

///IDropData implementation class.
class DataObjectImpl : public UnknownImplementation<IDataObject, IID_IDataObject>
{
public:
	DataObjectImpl(std::unique_ptr<DataSource> src);

    __stdcall HRESULT GetData(FORMATETC*, STGMEDIUM*) override;
    __stdcall HRESULT GetDataHere(FORMATETC*, STGMEDIUM*) override;
    __stdcall HRESULT QueryGetData(FORMATETC*) override;
    __stdcall HRESULT GetCanonicalFormatEtc(FORMATETC*, FORMATETC*) override;
    __stdcall HRESULT SetData(FORMATETC*, STGMEDIUM*, BOOL) override;
    __stdcall HRESULT EnumFormatEtc(DWORD, IEnumFORMATETC**) override;
    __stdcall HRESULT DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) override;
    __stdcall HRESULT DUnadvise(DWORD) override;
    __stdcall HRESULT EnumDAdvise(IEnumSTATDATA**) override;

protected:
	///Returns the dataType id of the supported types that matches the given format.
	///Returns dataType::none if it could not match a known dataType.
	unsigned int lookupFormat(const FORMATETC& format) const;

	///Returns a FORMATETC struct for the given supported dataType id.
	FORMATETC format(unsigned int dataType) const;
	bool format(unsigned int dataType, FORMATETC& format) const;

	///Returns a STGMEDIUM struct for the given dataType id holding the data.
	STGMEDIUM medium(unsigned int id) const;
	bool medium(unsigned int id, STGMEDIUM& med) const;

protected:
	std::unique_ptr<DataSource> source_;
	std::vector<FORMATETC> formats_;
};


//unkown implementation
template<typename T, const GUID&... ids> HRESULT
UnknownImplementation<T, ids...>::QueryInterface(REFIID riid, void** ppv)
{
	using Expand = int[];
	if(!ppv) return E_INVALIDARG;

	*ppv = nullptr;
	bool found = (riid == IID_IUnknown);
	Expand {(found |= (riid == ids), 0)...};

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

///Winapi data offer implementation
class DataOfferImpl : public DataOffer
{
public:
	DataOfferImpl(IDataObject& object);

	DataTypes types() const override { return types_; }
	nytl::CbConn data(unsigned int fmt, const DataFunction& func) override;

protected:
	DataTypes types_;
	com::DataComObject data_;
	std::unordered_map<unsigned int, FORMATETC> typeFormats_;
};

} //namespace winapi

} //namespace ny
