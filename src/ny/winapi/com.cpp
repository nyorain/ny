// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/com.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/winapi/util.hpp>
#include <ny/dataExchange.hpp>
#include <ny/log.hpp>
#include <ny/imageData.hpp>

#include <nytl/utf.hpp>
#include <nytl/scope.hpp>
#include <nytl/range.hpp>

#include <Shlobj.h>

#include <unordered_map>
#include <cstring>
#include <cmath>

namespace ny
{

namespace winapi
{

DataOfferImpl::DataOfferImpl(IDataObject& object) : data_(object)
{
	IEnumFORMATETC* enumerator;
	data_->EnumFormatEtc(DATADIR_GET, &enumerator);

	//other formats with non-standard mime types
	//windows can automatically convert between certain formats
	static struct {
		unsigned int cf;
		DataFormat format;
		unsigned int tymed {TYMED_HGLOBAL};
	} mappings [] {
		{CF_BITMAP, DataFormat::imageData, TYMED_GDI},
		{CF_DIBV5, {"image/bmp", {"image/x-windows-bmp"}}},
		{CF_DIF, {"video/x-dv"}},
		{CF_ENHMETAFILE, {"image/x-emf"}},
		{CF_HDROP, DataFormat::uriList},
		{CF_RIFF, {"audio/wav", {"riff", "audio/wave", "audio/x-wav", "audio/vnd.wave"}}},
		{CF_WAVE, {"audio/wav", {"wave", "audio/wave", "audio/x-wav", "audio/vnd.wave"}}},
		{CF_TIFF, {"image/tiff"}},
		{CF_UNICODETEXT, DataFormat::text}
	};

	//enumerate all formats and check for the ones that can be understood
	//msdn states that the enumerator allocates the memory for formats but since we only pass
	//a pointer this does not make any sense. We obviously pass with formats an array of at
	//least celt (first param) objects.
	constexpr auto formatsSize = 100;
	FORMATETC formats[formatsSize];
	ULONG count;

	//enumerate formats until there are no more available
	while(true)
	{
		auto ret = enumerator->Next(formatsSize, formats, &count);

		//handle every returned format
		for(auto format : nytl::Range<FORMATETC>(*formats, count))
		{
			//check mappings for standard formats
			//if we don't know how to handle the medium, ignore it
			bool found {};
			for(auto& mapping : mappings)
			{
				if(mapping.cf == format.cfFormat && (mapping.tymed & mapping.tymed))
				{
					formats_[mapping.format] = format;
					found = true;
					break;
				}
			}

			//check for custom value and valid format
			//XXX: handle other medium types such as files and streams
			if(!found && format.cfFormat >= 0xC000 && format.tymed == TYMED_HGLOBAL)
			{
				wchar_t buffer[256] {};
				auto bytes = ::GetClipboardFormatName(format.cfFormat, buffer, 255);
				if(!bytes) continue;
				buffer[bytes] = '\0';

				auto name = nytl::toUtf8(buffer);

				//check for uri-names of standard formats (etc. handle "text/plain" as text)
				//otherwise just insert the name as data format
				using DF = DataFormat;
				if(match(DF::text, name)) formats_[DF::text] = format;
				else if(match(DF::raw, name)) formats_[DF::raw] = format;
				else if(match(DF::uriList, name)) formats_[DF::uriList] = format;
				else if(match(DF::imageData, name)) formats_[DF::imageData] = format;
				else formats_[{name}] = format;
			}
		}

		//XXX: error handling?
		if(ret == S_FALSE || count < formatsSize) break;
		count = 0;
	}

	enumerator->Release();
}

DataOffer::FormatsRequest DataOfferImpl::formats() const
{
	std::vector<DataFormat> formats;
	formats.reserve(formats_.size());
	for(auto& f : formats_) formats.push_back(f.first);

	using RequestImpl = DefaultAsyncRequest<std::vector<DataFormat>>;
	return std::make_unique<RequestImpl>(std::move(formats));
}

DataOffer::DataRequest DataOfferImpl::data(const DataFormat& format)
{
	HRESULT res = 0;
	STGMEDIUM medium {};

	auto it = formats_.find(format);
	if(it == formats_.end())
	{
		warning("ny::winapi::DataOfferImpl::data failed: format not supported");
		return {};
	}

	auto& formatetc = it->second;
	if((res = data_->GetData(&formatetc, &medium)))
	{
		warning(errorMessage(res, "ny::winapi::DataOfferImpl::data: data_->GetData failed"));
		return {};
	}

	//always release the medium, no matter what
	auto releaseGuard = nytl::makeScopeGuard([&]{ ::ReleaseStgMedium(&medium); });

	using RequestImpl = DefaultAsyncRequest<std::any>;
	return std::make_unique<RequestImpl>(fromStgMedium(formatetc, format, medium));
}

namespace com
{

//DropTargetImpl
HRESULT DropTargetImpl::DragEnter(IDataObject* data, DWORD keyState, POINTL pos, DWORD* effect)
{
	nytl::unused(keyState);

	auto it = offers_.emplace(data, std::make_unique<DataOfferImpl>(*data)).first;
	current_ = it->second.get();

	auto position = nytl::Vec2ui(pos.x, pos.y);
	auto format = windowContext().listener().dndMove(position, *it->second, nullptr);

	if(format != DataFormat::none) *effect = DROPEFFECT_COPY;
	else *effect = DROPEFFECT_NONE;

	return S_OK;
}

HRESULT DropTargetImpl::DragOver(DWORD keyState, POINTL pos, DWORD* effect)
{
	nytl::unused(keyState);

	if(!current_)
	{
		warning("ny::winapi::DropTargetImpl::DragOver: no current drag data object");
		return E_UNEXPECTED;
	}

	auto position = nytl::Vec2ui(pos.x, pos.y);
	auto format = windowContext().listener().dndMove(position, *current_, nullptr);

	if(format != DataFormat::none) *effect = DROPEFFECT_COPY;
	else *effect = DROPEFFECT_NONE;

	return S_OK;
}

HRESULT DropTargetImpl::DragLeave()
{
	if(!current_)
	{
		warning("ny::winapi::DropTargetImpl::DragLeave: no current drag data object");
		return E_UNEXPECTED;
	}

	windowContext().listener().dndLeave(*current_, nullptr);

	//erase it and reset current_
	offers_.erase(offers_.find(current_->dataObject().get()));
	current_ = {};

	return S_OK;
}

HRESULT DropTargetImpl::Drop(IDataObject* data, DWORD keyState, POINTL pos, DWORD* effect)
{
	nytl::unused(keyState);

	if(!current_ || current_->dataObject().get() != data)
	{
		warning("ny::winapi::DropTargetImpl::Drop: current drop data object inconsistency");
		return E_UNEXPECTED;
	}

	auto position = nytl::Vec2ui(pos.x, pos.y);
	auto format = windowContext().listener().dndMove(position, *current_, nullptr);

	if(format == DataFormat::none)
	{
		*effect = DROPEFFECT_NONE;
		return S_OK;
	}

	auto it = offers_.find(data);
	if(it == offers_.end())
	{
		warning("ny::winapi::DropTargetImpl::Drop: invalid current drop data object");
		return E_UNEXPECTED;
	}

	auto offer = std::move(it->second);
	offers_.erase(it);
	windowContext().listener().dndDrop(position, std::move(offer), nullptr);

	return S_OK;
}

//DropSourceImpl
HRESULT DropSourceImpl::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if(fEscapePressed) return DRAGDROP_S_CANCEL;
	if(!(grfKeyState & MK_LBUTTON)) return DRAGDROP_S_DROP;
	return S_OK;
}
HRESULT DropSourceImpl::GiveFeedback(DWORD dwEffect)
{
	nytl::unused(dwEffect);
	return DRAGDROP_S_USEDEFAULTCURSORS;
	// return S_OK;
}

//DataObjectImpl
DataObjectImpl::DataObjectImpl(std::unique_ptr<DataSource> source) : source_(std::move(source))
{
	formats_.reserve(source_->formats().size());
	formats_.emplace_back();

	for(auto format : source_->formats()) addFormat(format);
}

HRESULT DataObjectImpl::GetData(FORMATETC* format, STGMEDIUM* stgmed)
{
	if(!format) return DV_E_FORMATETC;
	if(!stgmed) return E_UNEXPECTED;
	*stgmed = {};

	//lookup the DataFormat for the requested format. Fetch the data from the source,
	//and store it into the medium using winapi::toStgMedium
	//we always use the first matching format, therefore order matters when
	//inserting the formats
	for(auto& f : formats_)
	{
		if(f.first.cfFormat == format->cfFormat && f.first.tymed == format->tymed
			&& f.first.dwAspect == format->dwAspect)
		{
			auto stg = toStgMedium(f.second, f.first, source_->data(f.second));
			if(!stg.tymed) return DV_E_FORMATETC;

			*stgmed = stg;
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

	return DV_E_FORMATETC;
}
HRESULT DataObjectImpl::GetCanonicalFormatEtc(FORMATETC*, FORMATETC* formatOut)
{
	formatOut->ptd = nullptr;
	return E_NOTIMPL;
}
HRESULT DataObjectImpl::SetData(FORMATETC*, STGMEDIUM*, BOOL)
{
	return E_NOTIMPL;
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
		{CF_BITMAP, DataFormat::imageData, TYMED_GDI},
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
			formatetc.cfFormat = ::RegisterClipboardFormat(nytl::toWide(format.name).c_str());
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
	auto cf = ::RegisterClipboardFormat(nytl::toWide(format.name).c_str());
	formatetc.cfFormat = cf;
	formatetc.tymed = TYMED_HGLOBAL;
	formats_.push_back({formatetc, format});
}

} //namespace com

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
	auto cpy = std::u16string(string.c_str()); //remove nullterminator
	auto ptr = reinterpret_cast<const std::uint8_t*>(cpy.data());
	return bufferToGlobal({ptr, (string.size() + 1) * 2});
}

HGLOBAL stringToGlobal(const std::string& string)
{
	auto cpy = std::string(string.c_str()); //remove nullterminator
	auto ptr = reinterpret_cast<const std::uint8_t*>(cpy.data());
	return bufferToGlobal({ptr, string.size() + 1});
}

std::u16string globalToStringUnicode(HGLOBAL global)
{
	auto len = ::GlobalSize(global);
	auto ptr = ::GlobalLock(global);
	if(!ptr) return {};

	//usually len should be an even number (since it is encoded using utf16)
	//but to go safe, we round len / 2 up and then later remove the trailing
	//nullterminator
	std::u16string str(std::ceil(len / 2), '\0');
	std::memcpy(&str[0], ptr, len);
	::GlobalUnlock(global);
	str = str.c_str(); //get rid of terminators

	return str;
}

std::string globalToString(HGLOBAL global)
{
	auto len = ::GlobalSize(global);
	auto ptr = ::GlobalLock(global);
	if(!ptr) return {};

	std::string str(len, '\0');
	std::memcpy(&str[0], ptr, len);
	::GlobalUnlock(global);
	str = str.c_str(); //get rid of terminators

	return str;
}

HGLOBAL bufferToGlobal(const nytl::Range<std::uint8_t>& buffer)
{
	auto ret = ::GlobalAlloc(GMEM_MOVEABLE, buffer.size());
	if(!ret) return nullptr;

	auto ptr = ::GlobalLock(ret);
	if(!ptr)
	{
		::GlobalFree(ret);
		return nullptr;
	}

	std::memcpy(ptr, buffer.data(), buffer.size());
	::GlobalUnlock(ret);
	return ret;
}

std::vector<std::uint8_t> globalToBuffer(HGLOBAL global)
{
	auto len = ::GlobalSize(global); //excluding null terminator
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

	if(to == DataFormat::text && from.cfFormat == CF_UNICODETEXT)
	{
		if(medium.tymed != TYMED_HGLOBAL) return {};

		auto str16 = globalToStringUnicode(medium.hGlobal);
		if(str16.empty()) return {};
		auto str = nytl::toUtf8(str16);
		replaceCRLF(str);
		return str;
	}
	else if(to == DataFormat::uriList && from.cfFormat == CF_HDROP)
	{
		if(medium.tymed != TYMED_HGLOBAL) return {};

		auto ptr = ::GlobalLock(medium.hGlobal);
		if(!ptr) return {};

		auto hdrop = reinterpret_cast<HDROP>(ptr);
		auto count = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0); //query files count

		std::vector<std::string> paths;
		for(auto i = 0u; i < count; ++i)
		{
			auto size = DragQueryFile(hdrop, i, nullptr, 0); //query buffer size
			if(!size) continue;

			std::wstring path;
			path.resize(size);

			if(!DragQueryFile(hdrop, i, &path[0], size)) continue;
			paths.push_back("file://" + nytl::toUtf8(path));
		}

		::GlobalUnlock(medium.hGlobal);
		return {paths};
	}
	else if(to == DataFormat::imageData && from.cfFormat == CF_BITMAP)
	{
		if(medium.tymed != TYMED_GDI) return {};

		auto hbitmap = reinterpret_cast<HBITMAP>(medium.hGlobal);
		auto hdc = ::GetDC(nullptr);

		::BITMAPINFO bminfo {};
		bminfo.bmiHeader.biSize = sizeof(bminfo.bmiHeader);

		if(!::GetDIBits(hdc, hbitmap, 0, 0, nullptr, &bminfo, DIB_RGB_COLORS))
		{
			warning("ny::winapi::fromStgMedium(dataExchange): GetDiBits:1 failed");
			return {};
		}

		unsigned int width = bminfo.bmiHeader.biWidth;
		unsigned int height = std::abs(bminfo.bmiHeader.biHeight);
		unsigned int stride = width * 4;

		auto buffer = std::make_unique<uint8_t[]>(height * width * 4);

		bminfo.bmiHeader.biBitCount = 32;
		bminfo.bmiHeader.biCompression = BI_RGB;
		bminfo.bmiHeader.biHeight = height;

		if(::GetDIBits(hdc, hbitmap, 0, height, buffer.get(), &bminfo, DIB_RGB_COLORS) == 0)
		{
			warning("ny::winapi::fromStgMedium(dataExchange): GetDiBits:2 failed");
			return {};
		}

		OwnedImageData ret;
		ret.data = std::move(buffer);
		ret.size = {width, height};
		ret.format = ImageDataFormat::rgba8888;
		ret.stride = stride;

		return ret;
	}
	else
	{
		if(medium.tymed != TYMED_HGLOBAL) return {};
		return wrap(globalToBuffer(medium.hGlobal), to);
	}
}

STGMEDIUM toStgMedium(const DataFormat& from, const FORMATETC& to, const std::any& data)
{
	if(from == DataFormat::none || !to.cfFormat || !to.tymed || !data.has_value()) return {};

	STGMEDIUM ret {};
	if(from == DataFormat::text && to.cfFormat == CF_UNICODETEXT)
	{
		if(to.tymed != TYMED_HGLOBAL) return {};

		auto str = std::any_cast<const std::string&>(data);
		replaceLF(str);
		auto str16 = toUtf16(str);

		ret.tymed = TYMED_HGLOBAL;
		ret.hGlobal = stringToGlobalUnicode(str16);
	}
	else if(from == DataFormat::imageData && to.cfFormat == CF_BITMAP)
	{
		if(to.tymed != TYMED_GDI) return {};

		//winapi requires the image data to have this format
		static constexpr auto reqFormat = ImageDataFormat::bgra8888;

		const auto& img = std::any_cast<const ImageData&>(data);
		auto data = convertFormat(img, reqFormat);
		const auto& size = img.size;

		ret.tymed = TYMED_HGLOBAL;
		ret.hGlobal = ::CreateBitmap(size.x, size.y, 1, 32, data.get());
	}
	else if(from == DataFormat::uriList && to.cfFormat == CF_HDROP)
	{
		if(to.tymed != TYMED_HGLOBAL) return {};

		//https://msdn.microsoft.com/en-us/library/windows/desktop/bb776902(v=vs.85).aspx
		std::u16string filesstring;
		auto filenames = std::any_cast<const std::vector<std::string>&>(data);
		for(auto& name : filenames)
		{
			if(name.find("file://") != 0) continue;
			name.erase(0, 7);
			std::replace(name.begin(), name.end(), '\\', '/');
			filesstring.append(toUtf16(name + "\0"));
		}

		filesstring.append('\0'); //double null terminated

		DROPFILES dropfiles {};
		dropfiles.pFiles = sizeof(dropfiles);
		dropfiles.fWide = true;

		//allocate a global buffer
		//copy the DROPFILES value into it and the filestring after it
		//note that filestring.size() * 2 is needed since the string is utf16 encoded
		auto size = sizeof(dropfiles) + (filesstring.size() * 2);
		auto globalBuffer = ::GlobalAlloc(GMEM_MOVEABLE, size);
		if(!globalBuffer)
		{
			warning(errorMessage("ny::winapi::toStgMedium(DataExchange): GlobalAlloc"));
			return {};
		}

		auto bufferPtr = reinterpret_cast<uint8_t*>(::GlobalLock(globalBuffer));
		if(!bufferPtr)
		{
			warning(errorMessage("ny::winapi::toStgMedium(DataExchange): GlobalLock"));
			::GlobalFree(globalBuffer);
			return {};
		}

		std::memcpy(bufferPtr, &dropfiles, sizeof(dropfiles));
		std::memcpy(bufferPtr + sizeof(dropfiles), filesstring.data(), filesstring.size() * 2);
		::GlobalUnlock(bufferPtr);

		ret.tymed = TYMED_HGLOBAL;
		ret.hGlobal = globalBuffer;
	}
	else if(to.tymed == TYMED_HGLOBAL)
	{
		auto buffer = unwrap(data, from);
		if(!buffer.empty())
		{
			ret.tymed = TYMED_HGLOBAL;
			ret.hGlobal = bufferToGlobal(buffer);
		}
	}

	return ret;
}

} //namespace winapi

} //namespace ny
