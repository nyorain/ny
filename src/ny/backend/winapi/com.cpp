#include <ny/backend/winapi/com.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/util.hpp>
#include <ny/base/eventDispatcher.hpp>
#include <ny/base/data.hpp>
#include <ny/base/log.hpp>

#include <nytl/utf.hpp>

#include <Shlobj.h>

#include <unordered_map>
#include <cstring>
#include <cmath>

namespace ny
{

namespace winapi
{

///Winapi data offer implementation
class DataOfferImpl : public DataOffer
{
public:
	DataOfferImpl(IDataObject& object);

	virtual DataTypes types() const override { return types_; }
	virtual nytl::CbConn data(std::uint8_t fmt, const DataFunction& func) override;

protected:
	DataTypes types_;
	com::DataComObject data_;
	std::unordered_map<unsigned int, FORMATETC> typeFormats_;
};

DataOfferImpl::DataOfferImpl(IDataObject& object) : data_(object)
{
	IEnumFORMATETC* enumerator;
	data_->EnumFormatEtc(DATADIR_GET, &enumerator);

	FORMATETC* curr = nullptr;
	auto checkAdd = [&](CLIPFORMAT checkcf, DWORD checktymed, unsigned int type) {
			if(curr->cfFormat == checkcf && (curr->tymed && checktymed))
			{
				types_.add(type);
				typeFormats_[type] = *curr;
			}
		};

	ULONG count;
	FORMATETC format;
	auto ret2 = enumerator->Next(1, &format, &count);
	debug(errorMessage(ret2, "begin"));

	while(enumerator->Next(1, &format, &count) == S_OK)
	{
		curr = &format;
		debug("Format: ", curr->cfFormat);
		checkAdd(CF_TEXT, TYMED_HGLOBAL, dataType::text);
		checkAdd(CF_UNICODETEXT, TYMED_HGLOBAL, dataType::text);
		checkAdd(CF_BITMAP, TYMED_GDI, dataType::image);
		checkAdd(CF_HDROP, TYMED_HGLOBAL, dataType::filePaths);

		curr = nullptr;
	}

	//format = {CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	//auto ret = data_->QueryGetData(&format);
	//if(ret == S_OK) debug("Ayy s_ok");
	//else debug(errorMessage(ret, "not so ayy"));

	enumerator->Release();
}

CbConn DataOfferImpl::data(std::uint8_t format, const DataFunction& func)
{
	HRESULT res = 0;
	if(!types_.contains(format)) goto failure;

	if(format == dataType::text)
	{
		debug("TEXT");
		STGMEDIUM med {};
		res = data_->GetData(&typeFormats_.at(format), &med);
		debug(res == DV_E_LINDEX);
		debug(res == DV_E_FORMATETC);
		debug(res == DV_E_TYMED);
		debug(res == DV_E_DVASPECT);
		debug(res == OLE_E_NOTRUNNING);
		debug(res == E_UNEXPECTED);
		debug(res == E_INVALIDARG);
		debug(res == E_OUTOFMEMORY);
		debug(med.hGlobal);
		if(res != S_OK) goto failure;
		debug("yee");
		auto txt = toUtf8(globalToStringUnicode(med.hGlobal));
		debug(txt);
		replaceCRLF(txt);
		debug(txt);
		ReleaseStgMedium(&med);
		debug(txt);
		std::any a(std::string("heio"));
		debug("empty: ", a.empty());
		debug("ptr: ", &a);
		debug("typename: ", typeid(std::string).name());
		debug("txt: ", std::any_cast<std::string>(a));
		debug("txt: ", std::any_cast<std::string>(a));

		func(*this, format, a);
	}
	else if(format == dataType::image)
	{
		 //convert the HBitmap to a ny image
		 //support for compression/decompression?
		func(*this, format, {});
	}
	else if(format == dataType::filePaths)
	{
		//something lockglobal bs
		STGMEDIUM med;
		if(data_->GetData(&typeFormats_[format], &med) != S_OK) goto failure;
		auto hdrop = reinterpret_cast<HDROP>(med.hGlobal);
		auto count = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0); //query count

		std::vector<std::string> paths;
		for(auto i = 0u; i < count; ++i)
		{
			auto size = DragQueryFile(hdrop, i, nullptr, 0); //query buffer size
			char buffer[size];
			if(DragQueryFile(hdrop, i, buffer, size)) paths.push_back(buffer);
		}

		ReleaseStgMedium(&med);
		func(*this, format, std::move(paths));
	}
	else goto failure;

	return {};

failure:
	debug(res, errorMessage(res, " winapi::DataOfferImpl: ::GetData"));
	func(*this, format, {});
	return {};
}


namespace com
{

//DropTargetImpl
HRESULT DropTargetImpl::DragEnter(IDataObject* data, DWORD keyState, POINTL pos, DWORD* effect)
{
	*effect = DROPEFFECT_COPY;
	return S_OK;
}

HRESULT DropTargetImpl::DragOver(DWORD keyState, POINTL pos, DWORD* effect)
{
	///TODO: possibilty for different window areas to accept/not accept/move drops -> differ effect
	*effect = DROPEFFECT_COPY;
	return S_OK;
}

HRESULT DropTargetImpl::DragLeave()
{
	return S_OK;
}

HRESULT DropTargetImpl::Drop(IDataObject* data, DWORD keyState, POINTL pos, DWORD* effect)
{
	//XXX: do something with the data (e.g. send event)
	*effect = DROPEFFECT_COPY;

	auto& ac = windowContext_->appContext();
	if(!windowContext_->eventHandler() || !ac.eventDispatcher()) return E_UNEXPECTED;

	auto offer = std::make_unique<DataOfferImpl>(*data);
	DataOfferEvent ev(windowContext_->eventHandler(), std::move(offer));
	ac.eventDispatcher()->dispatch(std::move(ev));

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
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

//DataObjectImpl
DataObjectImpl::DataObjectImpl(std::unique_ptr<DataSource> source) : source_(std::move(source))
{
	//TODO: parse source formats
	//if source e.g. offsers dataType::text, this DataObject should offer unicode as well
	//as plain text.
}

HRESULT DataObjectImpl::GetData(FORMATETC* format, STGMEDIUM* stgmed)
{
	if(!format) return DV_E_FORMATETC;
	if(!stgmed) return E_UNEXPECTED;

	auto id = lookupFormat(*format);
	if(id == -1) return DV_E_FORMATETC;

	this->medium(id, *stgmed);

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
    else return SHCreateStdEnumFmtEtc(formats().size(), formats().data(), formatOut);
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

int DataObjectImpl::lookupFormat(const FORMATETC& fmt) const
{
	for(auto i = 0u; i < source_->types().types.size(); ++i)
	{
		auto f = format(i);
		if(f.cfFormat == 0 && f.tymed == TYMED_NULL) continue; //invalid
		if((f.tymed & fmt.tymed) && f.cfFormat == fmt.cfFormat && f.dwAspect == fmt.dwAspect)
			return i;
	}

	return -1;
}

FORMATETC DataObjectImpl::format(unsigned int id) const
{
	FORMATETC ret;
	format(id, ret);
	return ret;
}

bool DataObjectImpl::format(unsigned int id, FORMATETC& format) const
{
	if(id > source_->types().types.size())
		throw std::out_of_range("ny::winapi::com::DataObjectImpl::medium");

	auto type = source_->types().types[id];

	format.dwAspect = DVASPECT_CONTENT;
	format.ptd = nullptr;
	format.lindex = -1;

	if(type == dataType::image)
	{
		 format.cfFormat = CF_BITMAP;
		 format.tymed = TYMED_GDI;
	}
	else if(type == dataType::text)
	{
		 format.cfFormat = CF_UNICODETEXT;
		 format.tymed = TYMED_HGLOBAL;
	}
	else if(type == dataType::filePaths)
	{
		format.cfFormat = CF_HDROP;
		format.tymed = TYMED_HGLOBAL;
	}
	else
	{
		format.cfFormat = 0;
		format.tymed = 0;
	}
}

STGMEDIUM DataObjectImpl::medium(unsigned int id) const
{
	STGMEDIUM ret;
	medium(id, ret);
	return ret;
}

bool DataObjectImpl::medium(unsigned int id, STGMEDIUM& med) const
{
	if(id > source_->types().types.size())
		throw std::out_of_range("ny::winapi::com::DataObjectImpl::medium");

	med.tymed = format(id).tymed;
	med.pUnkForRelease = nullptr;
	auto type = source_->types().types[id];

	if(type == dataType::image)
	{
		//copy the image to a bitmap handle
		HBITMAP bitmap;
		med.hBitmap = bitmap;
		return true;
	}
	else if(type == dataType::text)
	{
		auto txt = std::any_cast<std::string>(source_->data(type));
		replaceLF(txt);
		med.hGlobal = stringToGlobalUnicode(toUtf16(txt)); //text is normally UTF16
		return true;
	}
	else if(type == dataType::filePaths)
	{
		DROPFILES files;

		auto filenames = std::any_cast<std::vector<std::string>>(source_->data(type));
		std::string filename;
		for(auto& name : filenames)
			filename.append(name + "\0");

		filename.append('\0'); //double null terminated
		med.hGlobal = stringToGlobal(filename); //encoded as ASCII
		return true;
	}

	return false;
}

std::vector<FORMATETC> DataObjectImpl::formats() const
{
	auto size = source_->types().types.size();
	std::vector<FORMATETC> ret(1);
	ret.reserve(size);

	for(auto i = 0u; i < size; ++i)
		if(format(i, ret.back())) ret.emplace_back();

	ret.erase(ret.end() - 1);
	return ret;
}


} //namespace com

//free functions impl
/*
HGLOBAL duplicateGlobal(HGLOBAL global)
{
	auto len = ::GlobalSize(global);
	auto ptr = ::GlobalLock(global);
	if(!ptr) return nullptr;

	auto ret = ::GlobalAlloc(GMEM_FIXED, len);
	if(!ret) return nullptr;

	std::memcpy(ret, ptr, len);
	::GlobalUnlock(global);
	return dest;
}
*/

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
	auto len = string.size() + 2; //null terminator
	auto ret = ::GlobalAlloc(GMEM_FIXED, len);
	if(!ret) return nullptr;

	std::memcpy(ret, string.c_str(), string.size() + 2);
	return ret;
}

HGLOBAL stringToGlobal(const std::string& string)
{
	auto len = string.size() + 1; //null terminator
	auto ret = ::GlobalAlloc(GMEM_FIXED, len);
	if(!ret) return nullptr;

	std::memcpy(ret, string.c_str(), string.size() + 2);
	return ret;
}

std::u16string globalToStringUnicode(HGLOBAL global)
{
	auto len = ::GlobalSize(global) - 2; //excluding null terminator
	auto ptr = ::GlobalLock(global);
	if(!ptr) return nullptr;

	//usually len should be an even number (since it is encoded using utf16)
	//but to go safe, we round len / 2 up and then later remove the trailing
	//nullterminator
	std::u16string str(std::ceil(len / 2), '\0'); //nullterminator excluded
	std::memcpy(&str[0], ptr, len);
	::GlobalUnlock(global);
	str = str.c_str(); //get rid of extra terminators
	return str;
}

std::string globalToString(HGLOBAL global)
{
	auto len = ::GlobalSize(global) - 1; //excluding null terminator
	auto ptr = ::GlobalLock(global);
	if(!ptr) return nullptr;

	std::string str(len, '\0'); //nullterminator excluded
	std::memcpy(&str[0], ptr, len);
	::GlobalUnlock(global);
	return str;
}

} //namespace winapi

} //namespace ny
