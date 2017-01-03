// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/dataExchange.hpp>
#include <nytl/nonCopyable.hpp>

#include <ole2.h>
#include <Shobjidl.h>

#include <atomic> // std::atomic
#include <memory> // std::unique_ptr
#include <string> // std::string
#include <vector> // std::vector
#include <unordered_map> // std::unordered_map
#include <any> // std::any

// TODO: this header could use some more com-specific documentation
// document exceptions!

namespace ny {
namespace winapi::com {

/// Smart pointer wrapper for shared com objects.
/// Automatically adds a reference on construction and removes it on destruction.
template<typename T>
class ComObject {
public:
	ComObject() = default;
	ComObject(T& obj) : obj_(&obj) { obj_->AddRef(); }
	~ComObject() { if(obj_) obj_->Release(); }

	ComObject(const ComObject& other) : obj_(other.obj_) { if(obj_) obj_->AddRef(); }
	ComObject& operator=(const ComObject& other)
	{
		if(obj_) obj_->Release();
		obj_ = other.obj_;
		if(obj_) obj_->AddRef();
		return *this;
	}

	ComObject(ComObject&& other) noexcept : obj_(other.obj_) { other.obj_ = {}; }
	ComObject& operator=(ComObject&& other) noexcept
	{
		if(obj_) obj_->Release();
		obj_ = other.obj_;
		other.obj_ = {};
		return *this;
	}

	T* get() const { return obj_; };
	T& operator*() const { return get(); }
	T* operator->() const { return obj_; }
	operator bool() const { return (obj_); }

protected:
	T* obj_ = nullptr;
};

using UnknownComObject = ComObject<IUnknown>;
using DataComObject = ComObject<IDataObject>;

} // namespace winapi::com

/// Winapi DataOffer implementation using com.
class WinapiDataOffer: public DataOffer {
public:
	WinapiDataOffer() = default;
	WinapiDataOffer(IDataObject& object);

	WinapiDataOffer(WinapiDataOffer&&) noexcept = default;
	WinapiDataOffer& operator=(WinapiDataOffer&&) noexcept = default;

	FormatsRequest formats() override;
	DataRequest data(const DataFormat& format) override;

	// - winapi specific -
	winapi::com::DataComObject& dataObject() { return data_; }
	const winapi::com::DataComObject& dataObject() const { return data_; }

protected:
	winapi::com::DataComObject data_;
	std::unordered_map<DataFormat, FORMATETC> formats_;
};

namespace winapi::com {

/// RAII guard around a StgMedium object that releases it in its destructor
class StgMediumGuard : public nytl::NonCopyable {
public:
	STGMEDIUM medium {};

public:
	StgMediumGuard() = default;
	~StgMediumGuard() { ::ReleaseStgMedium(&medium); }

	StgMediumGuard(StgMediumGuard&& other) : medium(other.medium) { other.medium = {}; }
	StgMediumGuard& operator=(StgMediumGuard&& other)
	{
		::ReleaseStgMedium(&medium);
		medium = other.medium;
		other.medium = {};
		return *this;
	}
};

/// Implementation for the IUnkown com interface.
/// Classes deriving from this class must pass themself (CRTP) and the GUIDs to implement.
/// \tparam T The com interface (deriving IUnkown) to implement.
/// \tparam ids The ids of the interfaces that will be implemented.
template<typename T, const GUID&... ids>
class UnknownImplementation : public T {
public:
	virtual ~UnknownImplementation() = default;

	__stdcall HRESULT QueryInterface(REFIID riid, void** ppv) override;
	__stdcall ULONG AddRef() override;
	__stdcall ULONG Release() override;

protected:
	volatile std::atomic<unsigned int> refCount_ {0};
};

/// IDropTarget implementation that manages the current dnd DataOffer.
class DropTargetImpl : public UnknownImplementation<IDropTarget, IID_IDropTarget> {
public:
	DropTargetImpl(WinapiWindowContext& ctx);
	~DropTargetImpl();

	__stdcall HRESULT DragEnter(IDataObject*, DWORD, POINTL pos, DWORD* effect) override;
	__stdcall HRESULT DragOver(DWORD keys, POINTL pos, DWORD* effect) override;
	__stdcall HRESULT DragLeave() override;
	__stdcall HRESULT Drop(IDataObject* data, DWORD, POINTL pos, DWORD*  effect) override;

	/// Returns whether one of the formats the DataObject advertises is supported by dataTypes.
	bool supported(IDataObject& data);
	WinapiWindowContext& windowContext() const { return *windowContext_; }

protected:
	static IDropTargetHelper* helper_;

	WinapiWindowContext* windowContext_ {}; // associated WindowContext
	WinapiDataOffer offer_ {}; // the current DataOffer (may be empty)
};

/// IDropSource implementation.
class DropSourceImpl : public UnknownImplementation<IDropSource, IID_IDropSource> {
	__stdcall HRESULT GiveFeedback(DWORD effect) override;
	__stdcall HRESULT QueryContinueDrag(BOOL escape, DWORD keys) override;
};

/// IDropData implementation for a given DataSource implementation.
class DataObjectImpl : public UnknownImplementation<IDataObject, IID_IDataObject> {
public:
	DataObjectImpl(std::unique_ptr<DataSource> src);
	~DataObjectImpl();

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
	/// Translates the given DataFormat to native winapi clipboard formats and inserts
	/// them into the vector of supported formats.
	void addFormat(const DataFormat& format);

protected:
	static IDragSourceHelper* helper_;

	std::unique_ptr<DataSource> source_;
	std::vector<std::pair<FORMATETC, DataFormat>> formats_;

	// The DragSourceHelper allows us to correctly handle dnd images.
	// But since it needs SetData to be correctly implemented we need to store
	// the data retrieved from SetData in additionalData (and return if from GetData)
	std::vector<std::pair<FORMATETC, StgMediumGuard>> additionalData_;
};


// UnkownImplementation implementation
template<typename T, const GUID&... ids>
HRESULT UnknownImplementation<T, ids...>::QueryInterface(REFIID riid, void** ppv)
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

template<typename T, const GUID&... ids>
ULONG UnknownImplementation<T, ids...>::AddRef()
{
	return refCount_++;
}

template<typename T, const GUID&... ids>
ULONG UnknownImplementation<T, ids...>::Release()
{
	if(refCount_-- == 0) delete this;
	return refCount_;
}

/// Replaces all '\n' line ending with '\r\n' line endings as natively used by windows.
void replaceLF(std::string& string);

/// Replaces all '\r\n' line ending with '\n' line endings as natively used by ny.
void replaceCRLF(std::string& string);

/// Creates a global memory object for the given string.
HGLOBAL stringToGlobalUnicode(const std::u16string& string);

/// Copies the data from a global memory object to a string.
std::u16string globalToStringUnicode(HGLOBAL global);

/// Converts between a raw buffer and a global
HGLOBAL bufferToGlobal(nytl::Span<const uint8_t> buffer);
std::vector<std::uint8_t> globalToBuffer(HGLOBAL global);

/// Converts from a native winapi data medium to an any with the specified format.
/// Returns empty any on failure.
std::any fromStgMedium(const FORMATETC& from, const DataFormat& to, const STGMEDIUM& medium);

/// Converts between an any object and its DataFormat to the native winapi data medium.
/// Returns STGMEDIUM with tymed == 0 on failure.
STGMEDIUM toStgMedium(const DataFormat& from, const FORMATETC& to, const std::any& data);

/// Duplicates (i.e. deep copies) a given StgMedium.
/// A StgMedium returned by this must be correctly freed (i.e. using ReleaseStgMedium).
void duplicate(STGMEDIUM& dst, const STGMEDIUM& src, unsigned int cfFormat);

} //namespace winapi::com
} //namespace ny
