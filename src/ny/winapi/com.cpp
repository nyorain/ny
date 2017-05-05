// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/com.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/winapi/util.hpp>
#include <ny/asyncRequest.hpp>
#include <ny/log.hpp>
#include <ny/image.hpp>

#include <nytl/utf.hpp>
#include <nytl/scope.hpp>
#include <nytl/span.hpp>

#include <Shlobj.h>
#include <Shlwapi.h>

#include <unordered_map>
#include <cstring>
#include <cmath>

// the few useful sources:
// https://chromium.googlesource.com/chromium/chromium/+/master/ui/base/dragdrop/
// https://msdn.microsoft.com/en-us/library/windows/desktop/bb762034(v=vs.85).aspx
// https://github.com/WebKit/webkit/blob/master/Tools/DumpRenderTree/win/DRTDataObject.cpp

namespace ny {

WinapiDataOffer::WinapiDataOffer(IDataObject& object) : data_(object)
{
	IEnumFORMATETC* enumerator;
	data_->EnumFormatEtc(DATADIR_GET, &enumerator);

	// other formats with non-standard mime types
	// windows can automatically convert between certain formats
	static struct {
		unsigned int cf;
		DataFormat format;
		unsigned int tymed {TYMED_HGLOBAL};
	} mappings [] {
		{CF_BITMAP, DataFormat::image, TYMED_GDI},
		{CF_DIBV5, {"image/bmp", {"image/x-windows-bmp"}}},
		{CF_DIF, {"video/x-dv"}},
		{CF_ENHMETAFILE, {"image/x-emf"}},
		{CF_HDROP, DataFormat::uriList},
		{CF_RIFF, {"audio/wav", {"riff", "audio/wave", "audio/x-wav", "audio/vnd.wave"}}},
		{CF_WAVE, {"audio/wav", {"wave", "audio/wave", "audio/x-wav", "audio/vnd.wave"}}},
		{CF_TIFF, {"image/tiff"}},
		{CF_UNICODETEXT, DataFormat::text}
	};

	// enumerate all formats and check for the ones that can be understood
	// msdn states that the enumerator allocates the memory for formats but since we only pass
	// a pointer this does not make any sense. We obviously pass with formats an array of at
	// least celt (first param) objects.
	constexpr auto formatsSize = 100;
	FORMATETC formats[formatsSize];
	ULONG count;

	// enumerate formats until there are no more available
	while(true) {
		auto ret = enumerator->Next(formatsSize, formats, &count);

		// handle every returned format
		for(auto format : nytl::Span<FORMATETC>(*formats, count)) {
			// check mappings for standard formats
			// if we don't know how to handle the medium, ignore it
			bool found {};
			for(auto& mapping : mappings) {
				if(mapping.cf == format.cfFormat && (mapping.tymed & mapping.tymed)) {
					formats_[mapping.format] = format;
					found = true;
					break;
				}
			}

			// check for custom value and valid format
			// TODO: handle other medium types such as files and streams
			if(!found && format.cfFormat >= 0xC000 && format.tymed == TYMED_HGLOBAL) {
				wchar_t buffer[256] {};
				auto bytes = ::GetClipboardFormatName(format.cfFormat, buffer, 255);
				if(!bytes) continue;
				buffer[bytes] = '\0';
				auto name = narrow(buffer);

				// check for uri-names of standard formats (etc. handle "text/plain" as text)
				// otherwise just insert the name as data format
				using DF = DataFormat;
				if(match(DF::text, name)) formats_[DF::text] = format;
				else if(match(DF::raw, name)) formats_[DF::raw] = format;
				else if(match(DF::uriList, name)) formats_[DF::uriList] = format;
				else if(match(DF::image, name)) formats_[DF::image] = format;
				else formats_[{name}] = format;
			}
		}

		// TODO: error handling?
		if(ret == S_FALSE || count < formatsSize) break;
		count = 0;
	}

	enumerator->Release();
}

DataOffer::FormatsRequest WinapiDataOffer::formats()
{
	std::vector<DataFormat> formats;
	formats.reserve(formats_.size());
	for(auto& f : formats_) formats.push_back(f.first);

	using RequestImpl = DefaultAsyncRequest<std::vector<DataFormat>>;
	return std::make_unique<RequestImpl>(std::move(formats));
}

DataOffer::DataRequest WinapiDataOffer::data(const DataFormat& format)
{
	HRESULT res = 0;
	STGMEDIUM medium {};

	auto it = formats_.find(format);
	if(it == formats_.end()) {
		warning("ny::winapi::DataOfferImpl::data failed: format not supported");
		return {};
	}

	auto& formatetc = it->second;
	if((res = data_->GetData(&formatetc, &medium))) {
		warning(winapi::errorMessage(res, "ny::winapi::DataOfferImpl::data: GetData failed"));
		return {};
	}

	// always release the medium, no matter what
	auto releaseGuard = nytl::makeScopeGuard([&]{ ::ReleaseStgMedium(&medium); });

	using RequestImpl = DefaultAsyncRequest<std::any>;
	return std::make_unique<RequestImpl>(winapi::com::fromStgMedium(formatetc, format, medium));
}

namespace winapi::com {

IDragSourceHelper* DataObjectImpl::helper_ {};
IDropTargetHelper* DropTargetImpl::helper_ {};

// DropTargetImpl
DropTargetImpl::DropTargetImpl(WinapiWindowContext& ctx) : windowContext_(&ctx)
{
	if(!helper_) {
		auto ret = ::CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER,
			IID_IDropTargetHelper, reinterpret_cast<void**>(&helper_));

		if(!helper_ || ret != S_OK) {
			std::string msg = "ny::winapi::com::DataObjectImpl: CoCreateInstance: ";
			msg += std::to_string(ret);
			msg += errorMessage(ret);
			throw std::runtime_error(msg);
		}
	}

	helper_->Show(true);
}

DropTargetImpl::~DropTargetImpl()
{
	if(helper_) helper_->Release();
}

HRESULT DropTargetImpl::DragEnter(IDataObject* data, DWORD keyState, POINTL screenPos,
	DWORD* effect)
{
	nytl::unused(keyState);

	POINT windowPos;
	windowPos.x = screenPos.x;
	windowPos.y = screenPos.y;

	helper_->DragEnter(windowContext().handle(), data, &windowPos, *effect);
	offer_ = {*data};

	::ScreenToClient(windowContext().handle(), &windowPos);
	auto position = nytl::Vec2i{windowPos.x, windowPos.y};

	DndEnterEvent dee;
	dee.eventData = nullptr;
	dee.position = position;
	dee.offer = &offer_;
	windowContext().listener().dndEnter(dee);

	DndMoveEvent dme;
	dme.eventData = nullptr;
	dme.position = position;
	dme.offer = &offer_;
	auto format = windowContext().listener().dndMove(dme);

	if(format != DataFormat::none) *effect = DROPEFFECT_COPY;
	else *effect = DROPEFFECT_NONE;

	return S_OK;
}

HRESULT DropTargetImpl::DragOver(DWORD keyState, POINTL screenPos, DWORD* effect)
{
	nytl::unused(keyState);

	POINT windowPos;
	windowPos.x = screenPos.x;
	windowPos.y = screenPos.y;

	helper_->DragOver(&windowPos, *effect);

	if(!offer_.dataObject()) {
		warning("ny::winapi::DropTargetImpl::DragOver: no current drag data object");
		return E_UNEXPECTED;
	}

	::ScreenToClient(windowContext().handle(), &windowPos);

	DndMoveEvent dme;
	dme.eventData = nullptr;
	dme.position = nytl::Vec2i{windowPos.x, windowPos.y};
	dme.offer = &offer_;
	auto format = windowContext().listener().dndMove(dme);

	if(format != DataFormat::none) *effect = DROPEFFECT_COPY;
	else *effect = DROPEFFECT_NONE;

	return S_OK;
}

HRESULT DropTargetImpl::DragLeave()
{
	helper_->DragLeave();

	if(!offer_.dataObject()) {
		warning("ny::winapi::DropTargetImpl::DragLeave: no current drag data object");
		return E_UNEXPECTED;
	}

	DndLeaveEvent dle;
	dle.eventData = nullptr;
	dle.offer = &offer_;
	windowContext().listener().dndLeave(dle);

	offer_ = {}; // reset the current offer
	return S_OK;
}

HRESULT DropTargetImpl::Drop(IDataObject* data, DWORD keyState, POINTL screenPos, DWORD* effect)
{
	nytl::unused(keyState);

	POINT windowPos;
	windowPos.x = screenPos.x;
	windowPos.y = screenPos.y;

	helper_->Drop(data, &windowPos, *effect);

	if(!data || offer_.dataObject().get() != data) {
		warning("ny::winapi::DropTargetImpl::Drop: current drop data object inconsistency");
		return E_UNEXPECTED;
	}

	::ScreenToClient(windowContext().handle(), &windowPos);
	auto position = nytl::Vec2i{windowPos.x, windowPos.y};

	DndMoveEvent dme;
	dme.eventData = nullptr;
	dme.position = position;
	dme.offer = &offer_;
	auto format = windowContext().listener().dndMove(dme);

	if(format == DataFormat::none) {
		*effect = DROPEFFECT_NONE;
		return S_OK;
	}

	DndDropEvent dde;
	dde.eventData = nullptr;
	dde.offer = std::make_unique<WinapiDataOffer>(std::move(offer_));
	dde.position = position;
	windowContext().listener().dndDrop(dde);

	return S_OK;
}

// DropSourceImpl
HRESULT DropSourceImpl::QueryContinueDrag(BOOL fEscapePressed, DWORD keyState)
{
	if(fEscapePressed) return DRAGDROP_S_CANCEL;
	if(!(keyState & MK_LBUTTON)) return DRAGDROP_S_DROP;
	return S_OK;
}
HRESULT DropSourceImpl::GiveFeedback(DWORD dwEffect)
{
	nytl::unused(dwEffect);
	return DRAGDROP_S_USEDEFAULTCURSORS; // or use return S_OK?
}

// DataObjectImpl
DataObjectImpl::DataObjectImpl(std::unique_ptr<DataSource> source) : source_(std::move(source))
{
	formats_.reserve(source_->formats().size());
	formats_.emplace_back();

	for(auto format : source_->formats()) addFormat(format);

	auto img = source_->image();
	if(img.data) {
		if(!helper_) {
			auto ret = ::CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER,
				IID_IDragSourceHelper, reinterpret_cast<void**>(&helper_));

			if(!helper_ || ret != S_OK) {
				std::string msg = "ny::winapi::com::DataObjectImpl: CoCreateInstance: ";
				msg += std::to_string(ret);
				msg += winapi::errorMessage(ret);
				throw std::runtime_error(msg);
			}
		}

		auto bitmap = winapi::toBitmap(img);

		SHDRAGIMAGE sdi {};
		sdi.sizeDragImage.cx = img.size[0];
		sdi.sizeDragImage.cy = img.size[1];
		sdi.hbmpDragImage = bitmap;
		sdi.crColorKey = 0xFFFFFFFF;
		sdi.ptOffset = {};

		helper_->InitializeFromBitmap(&sdi, this);
	}
}

DataObjectImpl::~DataObjectImpl()
{
	if(helper_) helper_->Release();
}

HRESULT DataObjectImpl::GetData(FORMATETC* format, STGMEDIUM* stgmed)
{
	if(!format) return DV_E_FORMATETC;
	if(!stgmed) return E_UNEXPECTED;
	*stgmed = {};

	// lookup the DataFormat for the requested format. Fetch the data from the source,
	// and store it into the medium using winapi::toStgMedium
	// we always use the first matching format, therefore order matters when
	// inserting the formats
	for(auto& f : formats_) {
		if(f.first.cfFormat == format->cfFormat &&
			f.first.tymed == format->tymed &&
			f.first.dwAspect == format->dwAspect) {

			auto stg = toStgMedium(f.second, f.first, source_->data(f.second));
			if(!stg.tymed) return DV_E_FORMATETC;

			*stgmed = stg;
			return S_OK;
		}
	}

	// check private formats add with SetData
	// needed for DragSourceHelper to work correctly
	for(auto& f : additionalData_) {
		if(f.first.cfFormat == format->cfFormat &&
			f.first.tymed == format->tymed &&
			f.first.dwAspect == format->dwAspect) {

			duplicate(*stgmed, f.second.medium, f.first.cfFormat);
			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}
HRESULT DataObjectImpl::GetDataHere(FORMATETC* format, STGMEDIUM* medium)
{
	nytl::unused(format, medium);
	return DATA_E_FORMATETC;
}
HRESULT DataObjectImpl::QueryGetData(FORMATETC* format)
{
	if(!format) return DV_E_FORMATETC;

	for(auto& f : formats_)
	{
		if(f.first.cfFormat == format->cfFormat &&
			f.first.tymed == format->tymed
			&& f.first.dwAspect == format->dwAspect) return S_OK;
	}

	for(auto& f : additionalData_)
	{
		if(f.first.cfFormat == format->cfFormat && f.first.tymed == format->tymed
			&& f.first.dwAspect == format->dwAspect) return S_OK;
	}

	return DV_E_FORMATETC;
}
HRESULT DataObjectImpl::GetCanonicalFormatEtc(FORMATETC*, FORMATETC* formatOut)
{
	formatOut->ptd = nullptr;
	return E_NOTIMPL;
}
HRESULT DataObjectImpl::SetData(FORMATETC* format, STGMEDIUM* medium, BOOL owned)
{
	//needed for DragSourceHelper to work correctly
	additionalData_.emplace_back();
	additionalData_.back().first = *format;

	if(owned) additionalData_.back().second.medium = *medium;
	else duplicate(additionalData_.back().second.medium, *medium, format->cfFormat);

	return S_OK;
}
HRESULT DataObjectImpl::EnumFormatEtc(DWORD direction, IEnumFORMATETC** formatOut)
{
	if(direction != DATADIR_GET) return E_NOTIMPL;

	std::vector<FORMATETC> formatetcs;
	formatetcs.reserve(formats_.size());
	for(auto& f : formats_) formatetcs.push_back(f.first);

	return SHCreateStdEnumFmtEtc(formatetcs.size(), formatetcs.data(), formatOut);
}
HRESULT DataObjectImpl::DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*)
{
	return OLE_E_ADVISENOTSUPPORTED;
}
HRESULT DataObjectImpl::DUnadvise(DWORD)
{
	return OLE_E_ADVISENOTSUPPORTED;
}
HRESULT DataObjectImpl::EnumDAdvise(IEnumSTATDATA**)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

void DataObjectImpl::addFormat(const DataFormat& format)
{
	static struct {
		unsigned int cf;
		DataFormat format;
		unsigned int tymed {TYMED_HGLOBAL};
	} mappings [] {
		{CF_BITMAP, DataFormat::image, TYMED_GDI},
		{CF_DIBV5, {"image/bmp", {"image/x-windows-bmp"}}},
		{CF_DIF, {"video/x-dv"}},
		{CF_ENHMETAFILE, {"image/x-emf"}},
		{CF_RIFF, {"audio/wav", {"audio/wave", "audio/x-wav", "audio/vnd.wave"}}},
		{CF_WAVE, {"audio/wav", {"audio/wave", "audio/x-wav", "audio/vnd.wave"}}},
		{CF_TIFF, {"image/tiff"}},
		{CF_UNICODETEXT, DataFormat::text}
	};

	FORMATETC formatetc {};
	formatetc.dwAspect = DVASPECT_CONTENT;
	formatetc.lindex = -1;

	//NOTE: sort better formats here before worse ones, i.e. first mime types
	//check standard clipboard formats
	//we always also support the custom mime type clipboard formats
	for(auto& mapping : mappings)
	{
		if(match(mapping.format, format.name))
		{
			formatetc.cfFormat = ::RegisterClipboardFormat(widen(format.name).c_str());
			formats_.push_back({formatetc, format});

			formatetc.cfFormat = mapping.cf;
			formatetc.tymed = mapping.tymed;
			formats_.push_back({formatetc, format});

			return;
		}
	}

	// - special cases -
	//for DataFormat::uriList, we have to check whether all uris are file:// uris
	//if so, we can support the HDROP native clipboard format.
	if(format == DataFormat::uriList)
	{
		formatetc.cfFormat = ::RegisterClipboardFormat(L"text/uri-list");
		formatetc.tymed = TYMED_HGLOBAL;
		formats_.push_back({formatetc, format});

		auto any = source_->data(DataFormat::uriList);
		if(any.has_value())
		{
			auto uriList = std::any_cast<const std::vector<std::string>&>(any);
			bool filesOnly = false;
			for(auto& uri : uriList) filesOnly &= (uri.find("file://") == 0);

			if(filesOnly)
			{
				formatetc.cfFormat = CF_HDROP;
				formatetc.tymed = TYMED_HGLOBAL;
				formats_.push_back({formatetc, format});
			}
		}

		return;
	}

	//custom clipboard format
	auto cf = ::RegisterClipboardFormat(widen(format.name).c_str());
	formatetc.cfFormat = cf;
	formatetc.tymed = TYMED_HGLOBAL;
	formats_.push_back({formatetc, format});
}

//free functions impl
void replaceLF(std::string& string)
{
	auto idx = string.find("\n");
	while(idx != std::string::npos)
	{
		string[idx] = '\r';
		string.insert(idx, 1, '\n');
		idx = string.find("\n", idx);
	}
}

void replaceCRLF(std::string& string)
{
	auto idx = string.find("\r\n");
	while(idx != std::string::npos)
	{
		string[idx] = '\n';
		string.erase(idx + 1, 1);
		idx = string.find("\r\n", idx);
	}
}

HGLOBAL stringToGlobalUnicode(const std::u16string& string)
{
	auto cpy = std::u16string(string.c_str()); // remove extra nullterminator
	auto ptr = reinterpret_cast<const uint8_t*>(cpy.data());
	return bufferToGlobal({ptr, (string.size() + 1) * 2});
}

std::u16string globalToStringUnicode(HGLOBAL global)
{
	auto len = ::GlobalSize(global);
	auto ptr = ::GlobalLock(global);
	if(!ptr) return {};

	// usually len should be an even number (since it is encoded using utf16)
	// but to go safe, we round len / 2 up and then later remove the trailing
	// nullterminator
	std::u16string str(std::ceil(len / 2), '\0');
	std::memcpy(&str[0], ptr, len);
	::GlobalUnlock(global);
	str = str.c_str(); //get rid of terminators

	return str;
}

HGLOBAL bufferToGlobal(nytl::Span<const uint8_t> buffer)
{
	auto ret = ::GlobalAlloc(GMEM_MOVEABLE, buffer.size());
	if(!ret) return nullptr;

	auto ptr = ::GlobalLock(ret);
	if(!ptr) {
		::GlobalFree(ret);
		return nullptr;
	}

	std::memcpy(ptr, buffer.data(), buffer.size());
	::GlobalUnlock(ret);
	return ret;
}

std::vector<std::uint8_t> globalToBuffer(HGLOBAL global)
{
	auto len = ::GlobalSize(global); // excluding null terminator
	auto ptr = ::GlobalLock(global);
	if(!ptr) return {};

	std::vector<std::uint8_t> ret(len);
	std::memcpy(ret.data(), ptr, len);
	::GlobalUnlock(global);
	return ret;
}

std::any fromStgMedium(const FORMATETC& from, const DataFormat& to, const STGMEDIUM& medium)
{
	if(to == DataFormat::none || !from.cfFormat || !from.tymed) return {};

	if(to == DataFormat::text && from.cfFormat == CF_UNICODETEXT) {
		if(medium.tymed != TYMED_HGLOBAL) return {};

		auto str16 = globalToStringUnicode(medium.hGlobal);
		if(str16.empty()) return {};
		auto str = nytl::toUtf8(str16);

		replaceCRLF(str);
		return str;
	} else if(to == DataFormat::uriList && from.cfFormat == CF_HDROP) {
		if(medium.tymed != TYMED_HGLOBAL) return {};

		auto ptr = ::GlobalLock(medium.hGlobal);
		if(!ptr) return {};

		auto hdrop = reinterpret_cast<HDROP>(ptr);
		auto count = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0); // query files count

		std::vector<std::string> paths;
		for(auto i = 0u; i < count; ++i) {
			auto size = DragQueryFile(hdrop, i, nullptr, 0); // query buffer size
			if(!size) continue;

			std::wstring path;
			path.resize(size + 1);

			if(!DragQueryFile(hdrop, i, &path[0], size + 1)) continue;

			std::wstring uri;
			DWORD bufferSize = 10 + size * 2;
			uri.resize(bufferSize);
			::UrlCreateFromPath(path.c_str(), &uri[0], &bufferSize, 0);

			paths.push_back(narrow(uri));
		}

		::GlobalUnlock(medium.hGlobal);
		return {paths};

	} else if(to == DataFormat::image && from.cfFormat == CF_BITMAP) {
		if(medium.tymed != TYMED_GDI) return {};

		auto hbitmap = reinterpret_cast<HBITMAP>(medium.hGlobal);
		auto ret = winapi::toImage(hbitmap);
		if(!ret.data) return {};
		return ret;

	} else {
		if(medium.tymed != TYMED_HGLOBAL) return {};
		return wrap(globalToBuffer(medium.hGlobal), to);
	}
}

STGMEDIUM toStgMedium(const DataFormat& from, const FORMATETC& to, const std::any& data)
{
	if(from == DataFormat::none || !to.cfFormat || !to.tymed || !data.has_value()) return {};

	STGMEDIUM ret {};
	if(from == DataFormat::text && to.cfFormat == CF_UNICODETEXT) {
		if(to.tymed != TYMED_HGLOBAL) return {};

		auto str = std::any_cast<const std::string&>(data);
		replaceLF(str);
		auto str16 = nytl::toUtf16(str);

		ret.tymed = TYMED_HGLOBAL;
		ret.hGlobal = stringToGlobalUnicode(str16);

	} else if(from == DataFormat::image && to.cfFormat == CF_BITMAP) {
		if(to.tymed != TYMED_GDI) return {};

		const auto& img = std::any_cast<const UniqueImage&>(data);
		ret.tymed = TYMED_HGLOBAL;
		ret.hGlobal = toBitmap(img);

	} else if(from == DataFormat::uriList && to.cfFormat == CF_HDROP) {
		if(to.tymed != TYMED_HGLOBAL) return {};

		// https://msdn.microsoft.com/en-us/library/windows/desktop/bb776902(v=vs.85).aspx
		std::string filesstring;
		auto uriList = std::any_cast<const std::vector<std::string>&>(data);
		for(auto& uri : uriList) {
			auto uriW = widen(uri);
			DWORD bufferSize = MAX_PATH;
			std::wstring path;
			path.resize(bufferSize);
			::PathCreateFromUrl(uriW.c_str(), &path[0], &bufferSize, 0);
			filesstring.append(narrow(path));
		}

		filesstring.append("\0\0"); // must be double null terminated

		DROPFILES dropfiles {};
		dropfiles.pFiles = sizeof(dropfiles);
		dropfiles.fWide = true;

		// allocate a global buffer
		// copy the DROPFILES value into it and the filestring after it
		// note that filestring.size() * 2 is needed since the string is utf16 encoded
		auto size = sizeof(dropfiles) + (filesstring.size() * 2);
		auto globalBuffer = ::GlobalAlloc(GMEM_MOVEABLE, size);
		if(!globalBuffer) {
			warning(winapi::errorMessage("ny::winapi::toStgMedium(DataExchange): GlobalAlloc"));
			return {};
		}

		auto bufferPtr = reinterpret_cast<uint8_t*>(::GlobalLock(globalBuffer));
		if(!bufferPtr) {
			warning(winapi::errorMessage("ny::winapi::toStgMedium(DataExchange): GlobalLock"));
			::GlobalFree(globalBuffer);
			return {};
		}

		auto filesstringW = widen(filesstring);
		std::memcpy(bufferPtr, &dropfiles, sizeof(dropfiles));
		std::memcpy(bufferPtr + sizeof(dropfiles), filesstringW.data(), filesstringW.size() * 2);
		::GlobalUnlock(bufferPtr);

		ret.tymed = TYMED_HGLOBAL;
		ret.hGlobal = globalBuffer;
	} else if(to.tymed == TYMED_HGLOBAL) {
		auto buffer = unwrap(data, from);
		if(!buffer.empty()) {
			ret.tymed = TYMED_HGLOBAL;
			ret.hGlobal = bufferToGlobal(buffer);
		}
	}

	return ret;
}

void duplicate(STGMEDIUM& dst, const STGMEDIUM& src, unsigned int cfFormat)
{
	dst = {};

	switch (src.tymed) {
		case TYMED_HGLOBAL:
			dst.hGlobal = static_cast<HGLOBAL>(OleDuplicateData(src.hGlobal, cfFormat, 0));
			break;
		case TYMED_FILE:
			dst.lpszFileName = static_cast<LPOLESTR>(OleDuplicateData(src.lpszFileName,
				cfFormat, 0));
			break;
		case TYMED_ISTREAM:
			dst.pstm = src.pstm;
			dst.pstm->AddRef();
			break;
		case TYMED_ISTORAGE:
			dst.pstg = src.pstg;
			dst.pstg->AddRef();
			break;
		case TYMED_GDI:
			dst.hBitmap = static_cast<HBITMAP>(OleDuplicateData(src.hBitmap, cfFormat, 0));
			break;
		case TYMED_MFPICT:
			dst.hMetaFilePict = static_cast<HMETAFILEPICT>(OleDuplicateData(src.hMetaFilePict,
				cfFormat, 0));
			break;
		case TYMED_ENHMF:
			dst.hEnhMetaFile = static_cast<HENHMETAFILE>(OleDuplicateData(src.hEnhMetaFile,
				cfFormat, 0));
			break;
		default:
			break;
	}

	dst.tymed = src.tymed;
	dst.pUnkForRelease = src.pUnkForRelease;

	if(dst.pUnkForRelease) dst.pUnkForRelease->AddRef();
}

} // namespace winapi::com
} // namespace ny
