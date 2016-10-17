#include <ny/backend/winapi/com.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/util.hpp>
#include <ny/base/eventDispatcher.hpp>
#include <ny/base/data.hpp>
#include <ny/base/log.hpp>
#include <ny/base/imageData.hpp>

#include <nytl/utf.hpp>
#include <nytl/time.hpp>
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

	//this function checks if the given fmt represents the given cf and tymed, if so, type
	//is added to types_ and fmt associated with it in typeFormats_
	//if type is already associated with a FORMATETC it will be overriden, therefore
	//order the checkAdd function calls from less-wanted to more-wanted for the same dataTypes
	auto checkAdd = [&](const FORMATETC& fmt, CLIPFORMAT cf, DWORD tymed, unsigned int type)
	{
		if(fmt.cfFormat == cf && (fmt.tymed && tymed))
		{
			types_.add(type); //the add call checks for duplicates
			typeFormats_[type] = fmt;
		}
	};

	auto customFormat = ::RegisterClipboardFormat("ny::customDataFormat");

	//enumerate all formats and check for the ones that can be understood
	//msdn states that the enumerator allocates the memory for formats but since we only pass
	//a pointer this does not make any sense. We obviously pass with formats an array of at
	//least celt (first param) objects.
	//msdn is pretty bad...
	constexpr auto formatsSize = 100;
	FORMATETC formats[formatsSize];
	ULONG count;

	while(true)
	{
		auto ret = enumerator->Next(formatsSize, formats, &count);
		for(auto i = 0u; i < count; ++i)
		{
			auto& format = formats[i];
			checkAdd(format, CF_TEXT, TYMED_HGLOBAL, dataType::text);
			checkAdd(format, CF_UNICODETEXT, TYMED_HGLOBAL, dataType::text);
			checkAdd(format, CF_DIBV5, TYMED_HGLOBAL, dataType::image);
			checkAdd(format, CF_HDROP, TYMED_HGLOBAL, dataType::filePaths);
			checkAdd(format, customFormat, TYMED_HGLOBAL, dataType::custom);
		}

		if(ret == S_FALSE || count < formatsSize) break;
		count = 0;
	}

	enumerator->Release();
}

CbConn DataOfferImpl::data(unsigned int format, const DataFunction& func)
{
	HRESULT res = 0;
	STGMEDIUM med {};
	std::any any;

	//always call the given function (empty any on failure)
	auto callGuard = nytl::makeScopeGuard([&]{
		func(any, *this, format);
	});

	auto it = typeFormats_.find(format);
	if(it == typeFormats_.end())
	{
		warning("ny::winapi::DataOfferImpl::data failed: format not supported");
		return {};
	}

	auto& formatetc = it->second;
	if((res = data_->GetData(&formatetc, &med)))
	{
		warning(errorMessage(res, "ny::winapi::DataOfferImpl::data failed: GetData"));
		return {};
	}

	//always release the medium
	auto releaseGuard = nytl::makeScopeGuard([&]{
		::ReleaseStgMedium(&med);
	});

	void* ptr = nullptr;
	if(med.tymed == TYMED_HGLOBAL) ptr = med.hGlobal;
	if(med.tymed == TYMED_GDI) ptr = med.hBitmap;

	if(!ptr)
	{
		warning("ny::winapi::DataOfferImpl::data failed: Cannot parse returned STGMEDIUM");
		return {};
	}

	unsigned int resFormat;
	std::unique_ptr<std::uint8_t> buffer;
	any = comToData(formatetc.cfFormat, ptr, resFormat, buffer);
	if(buffer) buffers_.push_back(std::move(buffer));

	if(resFormat != format) warning("ny::winapi::DataOfferImpl::data failed: formats do not match");
	return {};
}

namespace com
{

//DropTargetImpl
HRESULT DropTargetImpl::DragEnter(IDataObject* data, DWORD keyState, POINTL pos, DWORD* effect)
{
	if(!supported(*data)) currentEffect_ = DROPEFFECT_NONE;
	else currentEffect_ = DROPEFFECT_COPY;

	*effect = currentEffect_;
	return S_OK;
}

HRESULT DropTargetImpl::DragOver(DWORD keyState, POINTL pos, DWORD* effect)
{
	//TODO: possibilty for different window areas to accept/not accept/move drops -> differ effect

	*effect = currentEffect_;
	return S_OK;
}

HRESULT DropTargetImpl::DragLeave()
{
	return S_OK;
}

HRESULT DropTargetImpl::Drop(IDataObject* data, DWORD keyState, POINTL pos, DWORD* effect)
{
	//TODO
	*effect = DROPEFFECT_COPY;

	auto& ac = windowContext_->appContext();
	if(!windowContext_->eventHandler()) return E_UNEXPECTED;

	auto offer = std::make_unique<DataOfferImpl>(*data);
	DataOfferEvent ev(windowContext_->eventHandler(), std::move(offer));
	ac.dispatch(std::move(ev));

	return S_OK;
}

bool DropTargetImpl::supported(IDataObject& data)
{
	IEnumFORMATETC* enumerator;
	data.EnumFormatEtc(DATADIR_GET, &enumerator);
	auto guard = nytl::makeScopeGuard([&]{ enumerator->Release(); });

	constexpr auto formatsSize = 100;
	FORMATETC formats[formatsSize];
	ULONG count;

	while(true)
	{
		auto ret = enumerator->Next(formatsSize, formats, &count);
		for(auto i = 0u; i < count; ++i)
		{
			auto& format = formats[i];
			if(dataTypes.contains(clipboardFormatToDataType(format.cfFormat)))
				return true;
		}

		if(ret == S_FALSE || count < formatsSize) break;
		count = 0;
	}

	return false;
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
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

//DataObjectImpl
DataObjectImpl::DataObjectImpl(std::unique_ptr<DataSource> source) : source_(std::move(source))
{
	formats_.reserve(source_->types().types.size());
	formats_.emplace_back();
	for(auto t : source_->types().types) if(format(t, formats_.back())) formats_.emplace_back();
	formats_.erase(formats_.end() - 1);

}

HRESULT DataObjectImpl::GetData(FORMATETC* format, STGMEDIUM* stgmed)
{
	if(!format) return DV_E_FORMATETC;
	if(!stgmed) return E_UNEXPECTED;

	auto dataType = lookupFormat(*format);
	if(dataType == dataType::none) return DV_E_FORMATETC;

	this->medium(dataType, *stgmed);
	return S_OK;
}
HRESULT DataObjectImpl::GetDataHere(FORMATETC* format, STGMEDIUM* medium)
{
	return DATA_E_FORMATETC;
}
HRESULT DataObjectImpl::QueryGetData(FORMATETC* format)
{
	if(!format || lookupFormat(*format) == -1) return DV_E_FORMATETC;
	return S_OK;
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
	else return SHCreateStdEnumFmtEtc(formats_.size(), formats_.data(), formatOut);
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

unsigned int DataObjectImpl::lookupFormat(const FORMATETC& fmt) const
{
	for(auto type : source_->types().types)
	{
		auto f = format(type);
		if(f.cfFormat == 0 && f.tymed == TYMED_NULL) continue; //invalid
		if((f.tymed & fmt.tymed) && f.cfFormat == fmt.cfFormat && f.dwAspect == fmt.dwAspect)
			return type;
	}

	return dataType::none;
}

FORMATETC DataObjectImpl::format(unsigned int dataType) const
{
	FORMATETC ret {};
	format(dataType, ret);
	return ret;
}

bool DataObjectImpl::format(unsigned int dataType, FORMATETC& format) const
{
	format.dwAspect = DVASPECT_CONTENT;
	format.ptd = nullptr;
	format.lindex = -1;

	unsigned int medium;
	format.cfFormat = dataTypeToClipboardFormat(dataType, medium);
	format.tymed = medium;

	return true;
}

STGMEDIUM DataObjectImpl::medium(unsigned int dataType) const
{
	STGMEDIUM ret {};
	medium(dataType, ret);
	return ret;
}

bool DataObjectImpl::medium(unsigned int dataType, STGMEDIUM& med) const
{
	auto fmt = format(dataType);

	unsigned int cfFormat;
	unsigned int medium;
	auto ptr = dataToCom(dataType, source_->data(dataType), cfFormat, medium);

	if(!ptr || medium != fmt.tymed || cfFormat != fmt.cfFormat)
	{
		warning("ny::winapi::DataObjectImpl::medium: failed to convert data to com object.");
		return false;
	}

	med.hGlobal = ptr; //TODO: correct tymed<->enum setting
	med.tymed = fmt.tymed;
	med.pUnkForRelease = nullptr;
	return true;
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

unsigned int dataTypeToClipboardFormat(unsigned int dataType, unsigned int &medium)
{
	medium = TYMED_HGLOBAL;
	switch(dataType)
	{
		case dataType::text: return CF_UNICODETEXT;
		case dataType::filePaths: return CF_HDROP;
		case dataType::image: medium = TYMED_GDI; return CF_BITMAP;
		default: return ::RegisterClipboardFormat("ny::customDataType");
	}
}
unsigned int clipboardFormatToDataType(unsigned int cfFormat)
{
	switch(cfFormat)
	{
		case CF_OEMTEXT:
		case CF_UNICODETEXT:
		case CF_TEXT: return dataType::text;
		case CF_DIB:
		case CF_DIBV5:
		case CF_BITMAP: return dataType::image;
		case CF_HDROP: return dataType::filePaths;
		default: break;
	}

	if(cfFormat == ::RegisterClipboardFormat("ny::customDataType")) return dataType::custom;
	return 0;
}

std::any comToData(unsigned int cfFormat, void* data, unsigned int& dataType,
	std::unique_ptr<std::uint8_t>& buffer)
{
	switch(cfFormat)
	{
		case CF_TEXT:
		{
			dataType = dataType::text;

			auto str = globalToString(data);
			replaceCRLF(str);
			return (str.empty()) ? std::any{} : std::any{str};
		}
		case CF_UNICODETEXT:
		{
			dataType = dataType::text;

			auto str = toUtf8(globalToStringUnicode(data));
			replaceCRLF(str);
			return (str.empty()) ? std::any{} : std::any{str};
		}
		case CF_HDROP:
		{
			dataType = dataType::filePaths;

			auto ptr = ::GlobalLock(data);
			if(!ptr) return {};

			auto hdrop = reinterpret_cast<HDROP>(ptr);
			auto count = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0); //query count

			std::vector<std::string> paths;
			for(auto i = 0u; i < count; ++i)
			{
				auto size = DragQueryFile(hdrop, i, nullptr, 0); //query buffer size
				char buffer[size];
				if(DragQueryFile(hdrop, i, buffer, size)) paths.push_back(buffer);
			}

			::GlobalUnlock(data);
			return {paths};
		}
		case CF_DIBV5:
		{

			return {};
		}
		case CF_BITMAP:
		{
			dataType = dataType::image;

			auto hbitmap = reinterpret_cast<HBITMAP>(data);
			auto hdc = ::GetDC(nullptr);

			::BITMAPINFO bminfo = {0};
			bminfo.bmiHeader.biSize = sizeof(bminfo.bmiHeader);

			if(::GetDIBits(hdc, hbitmap, 0, 0, nullptr, &bminfo, DIB_RGB_COLORS) == 0)
			{
				//TODO: error handling
			}

			unsigned int width = bminfo.bmiHeader.biWidth;
			unsigned int height = std::abs(bminfo.bmiHeader.biHeight);
			unsigned int stride = width * 4;

			buffer = std::make_unique<std::uint8_t>(height * width * 4);

			bminfo.bmiHeader.biBitCount = 32;
			bminfo.bmiHeader.biCompression = BI_RGB;
			bminfo.bmiHeader.biHeight = height;

			if(::GetDIBits(hdc, hbitmap, 0, height, buffer.get(), &bminfo, DIB_RGB_COLORS) == 0)
			{
				//TODO: error handling
			}

			auto ret = ImageData{buffer.get(), {width, height}, ImageDataFormat::rgba8888, stride};
			return {ret};
		}
		default:
		{
			if(cfFormat == ::RegisterClipboardFormat("ny::customDataFormat"))
			{
				auto buffer = globalToBuffer(data);
				if(buffer.size() < 4) return {};

				dataType = reinterpret_cast<std::uint32_t&>(buffer[0]);

				//XXX: here are custom dataTypes that are specified by ny (such as time)
				//converted back
				if(dataType == dataType::timePoint)
				{
					auto rep = reinterpret_cast<std::int64_t&>(buffer[4]);
					return {nytl::TimePoint(nytl::Nanoseconds(rep))};
				}
				else if(dataType == dataType::timeDuration)
				{
					auto rep = reinterpret_cast<std::int64_t&>(buffer[4]);
					return {nytl::duration_cast<nytl::TimeDuration>(nytl::Nanoseconds(rep))};
				}

				buffer.erase(buffer.begin(), buffer.begin() + 4);
				return {buffer};
			}
			else
			{
				dataType = dataType::none;
				return {};
			}
		}
	}
}
void* dataToCom(unsigned int format, const std::any& data, unsigned int& cfFormat,
	unsigned int& medium)
{
	switch(format)
	{
		case dataType::text:
		{
			cfFormat = CF_UNICODETEXT;
			medium = TYMED_HGLOBAL;

			auto str = std::any_cast<const std::string&>(data);
			replaceLF(str);
			auto str16 = toUtf16(str);
			return stringToGlobalUnicode(str16);
		}
		case dataType::image:
		{
			cfFormat = CF_BITMAP;
			medium = TYMED_GDI;

			static constexpr auto reqFormat = ImageDataFormat::bgra8888;

			const auto& img = std::any_cast<const ImageData&>(data);
			auto data = convertFormat(img, reqFormat);
			const auto& size = img.size;
			return ::CreateBitmap(size.x, size.y, 1, 32, data.get());
		}
		case dataType::filePaths:
		{
			cfFormat = CF_HDROP;
			medium = TYMED_HGLOBAL;

			//https://msdn.microsoft.com/en-us/library/windows/desktop/bb776902(v=vs.85).aspx
			std::u16string filename;
			auto filenames = std::any_cast<std::vector<std::string>>(data);
			for(auto& name : filenames) filename.append(toUtf16(name + "\0"));
			filename.append('\0'); //double null terminated

			DROPFILES dropfiles {};
			dropfiles.pFiles = sizeof(dropfiles);
			dropfiles.fWide = true;

			const auto size = sizeof(dropfiles) + filename.size() * 2;
			auto buffer = std::make_unique<std::uint8_t[]>(size);
			std::memcpy(buffer.get(), &dropfiles, sizeof(dropfiles));
			std::memcpy(buffer.get() + sizeof(dropfiles), filename.data(), filename.size() * 2);
			return bufferToGlobal({buffer.get(), size});
		}
		case dataType::timePoint:
		{
			cfFormat = ::RegisterClipboardFormat("ny::customDataFormat");
			medium = TYMED_HGLOBAL;

			const auto& timepoint = std::any_cast<const nytl::TimePoint&>(data);
			auto ns = nytl::duration_cast<nytl::Nanoseconds>(timepoint.time_since_epoch());
			std::int64_t count = ns.count();
			std::uint32_t format32 = format;

			std::uint8_t buffer[sizeof(count) + 4];
			std::memcpy(buffer, &format32, 4);
			std::memcpy(buffer + 4, &count, sizeof(count));

			return bufferToGlobal({buffer, sizeof(count) + 4});
		}
		case dataType::timeDuration:
		{
			cfFormat = ::RegisterClipboardFormat("ny::customDataFormat");
			medium = TYMED_HGLOBAL;

			const auto& ns = std::any_cast<const nytl::TimeDuration&>(data);
			std::int64_t count = nytl::duration_cast<nytl::Nanoseconds>(ns).count();
			std::uint32_t format32 = format;

			std::uint8_t buffer[sizeof(count) + 4];
			std::memcpy(buffer, &format32, 4);
			std::memcpy(buffer + 4, &count, sizeof(count));

			return bufferToGlobal({buffer, sizeof(count) + 4});
		}
		default:
		{
			cfFormat = ::RegisterClipboardFormat("ny::customDataFormat");
			medium = TYMED_HGLOBAL;

			auto rawData = std::any_cast<std::vector<std::uint8_t>>(data);
			std::uint32_t format32 = format;
			auto ptr = reinterpret_cast<std::uint8_t*>(&format32);
			rawData.insert(rawData.begin(), ptr, ptr + 4);

			return bufferToGlobal({rawData.data(), rawData.size()});
		}
	}
}

} //namespace winapi

} //namespace ny


/*
//This snippet can be later used to provide raw bmp files instead of decoded
//images

dataType = dataType::bmp;

auto bitmapBytes = static_cast<const std::uint8_t*>(::GlobalLock(data));
auto buffLen = ::GlobalSize(data);

//here follows an ugly hack to load the bitmap data as raw image
//stbi has a built-in bmp decoder
//we just have to add some header vars to the buffer
//and yeah, before you ask this is the easiest way... windows...
//but we can still use bitmaps from 1995
//so we got this one going for us which is nice
//https://en.wikipedia.org/wiki/BMP_file_format
//the header is 14 bytes long
auto buffer = std::make_unique<std::uint8_t[]>(buffLen + 14);

//windows format
//bytes 3 - 10 are ignored by stbi
buffer[0] = 'B';
buffer[1] = 'M';
reinterpret_cast<std::uint32_t&>(buffer[10]) = sizeof(BITMAPV5HEADER);
std::memcpy(buffer.get() + 14, bitmapBytes, buffLen);
::GlobalUnlock(data);

evg::Image img;
img.loadFromMemory({buffer.get(), buffLen + 14});
return {img};
*/
