#include <ny/backend/winapi/com.hpp>

namespace ny
{

namespace winapi
{

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
	*effect = DROPEFFECT_COPY;
	return S_OK;
}

HRESULT DropTargetImpl::DragLeave()
{
	return S_OK;
}

HRESULT DropTargetImpl::Drop(IDataObject* data, DWORD keyState, POINTL pos, DWORD*  effect)
{
	//XXX: do something with the data (e.g. send event)
	*effect = DROPEFFECT_COPY;
	log("Got Drop");
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
HRESULT DataObjectImpl::GetData(FORMATETC* format, TGMEDIUM* medium)
{
	if(!format) return DV_E_FORMATETC;

	auto id = lookupFormat(*format);
	if(!id) return DV_E_FORMATETC;

	if(!medium) return E_UNEXPECTED;

	medium->tymed = formats()[id].tymed;
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
	int ret = 0;
	for(auto& fit : formats())
	{
		if((fit.tymed & fmt.tymed) && fit.cfFormat == fmt.cfFormat && fit.dwAspect == fmt.dwAspect)
		{
			return ret;
		}

		ret++;
	}

	return -1;
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
