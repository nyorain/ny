#include <ny/backend/winapi/com.hpp>
#include <ny/app/data.hpp>

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
	virtual std::any data(std::uint8_t format) const override;

protected:
	DataTypes types_;
	DataComObject data_;
};

DataOfferImpl::DataOfferImpl(IDataObject& object) : data_(object)
{
	IEnumFORMATETC* enumerator;
	data_->EnumFormatEtc(DATADIR_GET, &enumerator);

	CLIPBOARDFORMAT cf;
	DWORD tymed;

	using dt = namespace dataType;
	auto checkAdd = [&](CLIPBOARDFORMAT checkcf, DWORD checktymed, unsigned int type) {
			if(cf == checkcf && (tymed && checktymed)) types_.add(type);
		};

	FORMATETC* formats;
	ULONG count;
	do
	{
		enumerator->Next(10, formats, &count);
		for(auto i = 0u; i < count; ++i)
		{
			cf = formats[i].cfFormat;
			tymed = formats[i].tymed;

			checkAdd(CF_TEXT, TYMED_HGLOBAL, dt::text::plain);
			checkAdd(CF_TEXT, TYMED_HGLOBAL, dt::text::utf8);
			checkAdd(CF_TEXT, TYMED_HGLOBAL, dt::text::utf16);
			checkAdd(CF_TEXT, TYMED_HGLOBAL, dt::text::utf32);
			checkAdd(CF_UNICODETEXT, TYMED_HGLOBAL, dt::text::plain);
			checkAdd(CF_UNICODETEXT, TYMED_HGLOBAL, dt::text::utf8);
			checkAdd(CF_UNICODETEXT, TYMED_HGLOBAL, dt::text::utf16);
			checkAdd(CF_UNICODETEXT, TYMED_HGLOBAL, dt::text::utf32);
			checkAdd(CF_BITMAP, TYMED_GDI, dt::image::raw);
		}
	}
	while(count == 10);

	enumerator->Release();
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
	if(!windowContext_->eventHandler() || !ac.eventDispatcher) return E_UNEXPECTED;

	auto offer = std::make_unique<DataOfferImpl>(*data);
	DataOfferEvent ev(windowContext_->eventHandler(), std::move(offer));
	ac.eventDispatcher()->dispatch(ev);

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
HRESULT DataObjectImpl::GetData(FORMATETC* format, STGMEDIUM* medium)
{
	if(!format) return DV_E_FORMATETC;
	if(!medium) return E_UNEXPECTED;

	auto id = lookupFormat(*format);
	if(id == -1) return DV_E_FORMATETC;

	medium->tymed = formats(id).tymed;
	medium->pUnkForRelease = nullptr;

	//Code styles are like assholes. Everyone has one but only few people like the ones of
	//other people.
	//TODO: implement other cases e.g. bitmap or sth like this
	switch(m_pFormatEtc[idx].tymed)
    {
    	case TYMED_HGLOBAL:
		{
			auto cpy = duplicateGlobal(mediums()[id].hGlobal);
			if(!cpy) return E_UNEXPECTED;
	        medium->hGlobal = cpy;
	        break;
		}

    	default: return DV_E_FORMATETC;
    }

	return S_OK;
}
HRESULT DataObjectImpl::GetDataHere(FORMATETC* format, STGMEDIUM* medium)
{
	return DATA_E_FORMATETC;
}
HRESULT DataObjectImpl::QueryGetData(FORMATETC*)
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

int DataObjectImpl::lookupFormat(const FORMATETC& fmt)
{
	for(auto i = 0u; i < source_->types().types.size(); ++i)
	{
		auto f = format(i);
		if(f.cfFormat == 0 && f.tymed == TYMED_NULL) continue; //invalid
		if((f.tymed & fmt.tymed) && f.cfFormat == fmt.cfFormat && f.dwAspect == fmt.dwAspect)
			return ret;
	}

	return -1;
}

FORMATETC DataObjectImpl::format(unsigned int id) const
{
	if(id > source_->types().types.size())
		throw std::out_of_bounds("ny::winapi::com::DataObjectImpl::medium");

	using dt = dataType;
	auto type = source_->types().types[id];

	CLIPBOARDFORMAT cf = 0;
	DWORD tymed = TYMED_NULL;
	if(type >= dt::image::png && type <= dt::image::bmp) //image
	{
		 cf = CF_BITMAP;
		 tymed = TYMED_GDI;
	}
	else if(type == dt::text::plain)
	{
		 cf = CF_TEXT;
		 tymed = TYMED_HGLOBAL;
	}
	else if(type == dt::text::utf8 || type == dt::text::utf16 || type == dt::text::utf32)
	{
		cf = CF_UNICODETEXT;
		tymed = TYMED_HGLOBAL;
	}
	else if(type == dt::filePaths)
	{
		cf = CF_HDROP;
		tymed = TYMED_HGLOBAL;
	}

	return {cf, 0, DVASPECT_CONTENT, -1, tymed};
}

STGMEDIUM DataObjectImpl::medium(unsigned int format) const
{
	if(id > source_->types().types.size())
		throw std::out_of_bounds("ny::winapi::com::DataObjectImpl::medium");

	STGMEDIUM ret;
	if(type >= dt::image::png && type <= dt::image::bmp) //image
	{
		//copy the image to a bitmap handle
		HBITMAP bitmap;
		ret.hBitmap = bitmap;
	}
	else if(type == dt::text::plain)
	{
		auto txt = std::any_cast<std::string>(source_->data(type));
		//replaceLF(txt);
		ret.hGlobal = stringToGlobal(txt);
	}
	else if(type == dt::text::utf8)
	{
		auto txt = std::any_cast<std::string>(source_->data(type));
		//replaceLF(txt);
		ret.hGlobal = stringToGlobal(toUTF16(txt));
	}
	else if(type == dt::text::utf16)
	{
		auto txt = std::any_cast<std::u16string>(source_->data(type));
		//replaceLF(txt);
		ret.hGlobal = stringToGlobal(txt);
	}
	else if(type == dt::text::utf32)
	{
		auto txt = std::any_cast<std::u32string>(source_->data(type));
		//replaceLF(txt);
		ret.hGlobal = stringToGlobal(toUTF16(txt));

	}
	else if(type == dt::filePath)
	{
		DROPFILES files;
		auto filename = std::any_cast<std::string>(source_->data(type));
		filename.append('\0'); //double null terminated
		ret.hGlobal = stringToGlobal(filename);
	}

	return ret;
}

//free functions impl
HGLOBAL duplicateGlobal(HGLOBAL global)
{
	auto len = ::GlobalSize(global);
	auto ptr = ::GlobalLock(global);
	if(!ptr) return nullptr;

	auto ret = ::GlobalAlloc(GMEM_FIXED, global);
	if(!ptr) return nullptr;

	std::memcpy(dest, ptr, len);
	::GlobalUnlock(global);
	return dest;
}

}

}

}
