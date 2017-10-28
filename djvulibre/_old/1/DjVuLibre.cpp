#include "DjVuLibre.h"
#include "libdjvu\GString.h"
#include "libdjvu\GPixmap.h"
#include "libdjvu\DjVuInfo.h"
#include "libdjvu\DjVuImage.h"

//#include <ppltasks.h>
#include <wrl.h>
#include <robuffer.h>

#include <Windows.graphics.imaging.interop.h>

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Media::Imaging;

//using namespace winrt;

using namespace DJVU;

namespace djvulibre
{
	std::string ConvertCxStringToUTF8(String^ stringToConvert);

	struct __declspec(uuid("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")) __declspec(novtable) IMemoryBufferByteAccess : ::IUnknown
	{
		virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
	};

	Document^ Document::Open(String^ fileName)
	{
		// DjVuSource::FromFile
		GP<DjVuDocument> pDoc = NULL;
		std::string utf8Name = ConvertCxStringToUTF8(fileName);
		auto gstr = GUTF8String(utf8Name.c_str(), utf8Name.length());
		GURL url = GURL::Filename::UTF8(gstr);

		pDoc = DjVuDocument::create(url);
		if (pDoc->wait_get_pages_num() == 0)
			throw Exception::CreateException(E_FAIL, "Can't open file");

		return ref new Document(pDoc);
	}

	int Document::GetPageCount()
	{
		return _doc->get_pages_num();
	}

	Document::Document(GP<DjVuDocument> pDoc)
	{
		// ctor DjVuSource
		_doc = pDoc;
	}

	/*
	CDIB* RenderPixmap(GPixmap& pm, const CDisplaySettings& displaySettings)
	{
		return RenderPixmap(pm, CRect(0, 0, pm.columns(), pm.rows()), displaySettings);
	}

	CDIB* RenderPixmap(GPixmap& pm, const CRect& rcClip, const CDisplaySettings& displaySettings)
	{
		ASSERT(0 <= rcClip.left && 0 <= rcClip.top);
		ASSERT((int)pm.columns() >= rcClip.right && (int)pm.rows() >= rcClip.bottom);

		BITMAPINFOHEADER bmih;
		ZeroMemory(&bmih, sizeof(bmih));

		bmih.biSize = sizeof(bmih);
		bmih.biWidth = rcClip.Width();
		bmih.biHeight = rcClip.Height();
		bmih.biBitCount = 24;
		bmih.biCompression = BI_RGB;
		bmih.biClrUsed = 0;

		CDIB* pBitmap = CDIB::CreateDIB((BITMAPINFO*)&bmih);
		if (pBitmap->m_hObject == NULL)
			return pBitmap;

		int nRowLength = rcClip.Width() * 3;
		while (nRowLength % 4 != 0)
			++nRowLength;

		LPBYTE pBits = pBitmap->GetBits();
		for (int y = rcClip.top; y < rcClip.bottom; ++y, pBits += nRowLength)
			memcpy(pBits, pm[y] + rcClip.left, rcClip.Width() * 3);

		int nBrightness = displaySettings.GetBrightness();
		int nContrast = displaySettings.GetContrast();
		double fGamma = displaySettings.GetGamma();
		if (fGamma != 1.0 || nBrightness != 0 || nContrast != 0 || displaySettings.bInvertColors)
		{
			// Adjust gamma
			BYTE table[256];
			for (int i = 0; i < 256; ++i)
				table[i] = i;

			if (displaySettings.bInvertColors)
				BuildInvertTable(table);
			if (nBrightness != 0)
				BuildBrightnessTable(nBrightness, table);
			if (nContrast != 0)
				BuildContrastTable(nContrast, table);
			if (fGamma != 1.0)
				BuildGammaTable(fGamma, table);

			pBits = pBitmap->GetBits();
			for (int y = rcClip.top; y < rcClip.bottom; ++y, pBits += nRowLength)
			{
				LPBYTE pCurBit = pBits;
				LPBYTE pEndBit = pCurBit + rcClip.Width() * 3;
				while (pCurBit != pEndBit)
				{
					*pCurBit = table[*pCurBit];
					++pCurBit;
				}
			}
		}

		return pBitmap;
	}
	
	CDIB* RenderBitmap(GBitmap& bm, const CDisplaySettings& displaySettings)
	{
	return RenderBitmap(bm, CRect(0, 0, bm.columns(), bm.rows()), displaySettings);
	}
	*/

	void RenderGrayBitmap(GBitmap& bm, LPBYTE pPix, int offs, int stride)
	{
		int nPaletteEntries = bm.get_grays();
		byte* pBMI = new byte[nPaletteEntries];

		int width = bm.columns();
		int height = bm.rows();

		// Create palette for the bitmap
		int color = 0xff0000;
		int decrement = color / (nPaletteEntries - 1);
		for (int i = 0; i < nPaletteEntries; ++i)
		{
			pBMI[i] = color >> 16;
			color -= decrement;
		}

		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
				pPix[offs + y * stride + x] = pBMI[bm[y][x]];

		delete pBMI;
	}

	void RenderBitmap(GBitmap& bm, byte* sourcePixels)
	{
		int nPaletteEntries = bm.get_grays();
		uint32* pBMI = new uint32[nPaletteEntries];

		int width = bm.columns();
		int height = bm.rows();

		// Create palette for the bitmap
		int color = 0xff0000;
		int decrement = color / (nPaletteEntries - 1);
		for (int i = 0; i < nPaletteEntries; ++i)
		{
			uint32 c = (byte)(color >> 16);
			pBMI[i] = c | c << 8 | c << 16 | 0xff000000;
			color -= decrement;
		}

		uint32* pixels = (uint32*)sourcePixels;
		for (int y = 0; y < height; ++y)
		{
			auto line = bm[y];
			for (int x = 0; x < width; ++x, ++pixels)
				*pixels = pBMI[line[x]];
		}

		delete pBMI;
	}

	int Document::GetPageView(int pageNum)
	{
		GP<DjVuImage> pImage = _doc->get_page(pageNum);
		//pImage->set_rotate(0);

		// CRenderThread::Render
		GP<GBitmap> pGBitmap;
		GP<GPixmap> pGPixmap;
		
		GP<DjVuInfo> info = pImage->get_info();
		int nTotalRotate = (info != NULL ? info->orientation : 0) % 4;

		GRect rect(0, 0, pImage->get_width(), pImage->get_height());
		
		pGPixmap = pImage->get_pixmap(rect, rect);
		if (pGPixmap == NULL)
		{
			pGBitmap = pImage->get_bitmap(rect, rect, 4);
		}
		
#if 0
		CDIB* pBitmap = NULL;

		if (pGPixmap != NULL)
		{
			if (nTotalRotate != 0)
				pGPixmap = pGPixmap->rotate(nTotalRotate);

			/*if (bScalePnmFixed)
			{
				if (bScaleSubpix)
					pGPixmap = RescalePnm_subpix(pGPixmap, size.cx, size.cy);
				else
					pGPixmap = RescalePnm(pGPixmap, size.cx, size.cy);
			}*/

			//pBitmap = RenderPixmap(*pGPixmap, displaySettings);
		}/*
		else if (pGBitmap != NULL)
		{
			if (nTotalRotate != 0)
				pGBitmap = pGBitmap->rotate(nTotalRotate);

			if (bScalePnmFixed)
			{
				if (bScaleSubpix)
					pGPixmap = RescalePnm_subpix(pGBitmap, size.cx, size.cy);
				else
					pGBitmap = RescalePnm(pGBitmap, size.cx, size.cy);
			}

			if (pGPixmap)
				pBitmap = RenderPixmap(*pGPixmap, displaySettings);
			else
				pBitmap = RenderBitmap(*pGBitmap, displaySettings);
		}
		else
		{
			pBitmap = RenderEmpty(size, displaySettings);
		}

		if (pBitmap != NULL)
			pBitmap->SetDPI(pImage->get_dpi());*/
#endif
		return NULL;
	}

	SoftwareBitmap^ Document::DrawImage()
	{
		GP<DjVuImage> pImage = _doc->get_page(0);

		GP<GPixmap> pGPixmap;

		const unsigned int width = pImage->get_width();
		const unsigned int height = pImage->get_height();

		GRect rect(0, 0, width, height); // 2550 3300

		pGPixmap = pImage->get_pixmap(rect, rect);
		if (pGPixmap == NULL)
		{
			GP<GBitmap> pGBitmap = pImage->get_bitmap(rect, rect, 4);

			// This method might run faster if you use AMP or the Parallel Patterns Library to parallelize the operation.
			//return concurrency::create_async([this, width, height, pGBitmap]
			//{
			return CreateSoftwareBitmap(width, height, pGBitmap);
			//});
		}
		else
		{
			return nullptr;
		}
	}

	SoftwareBitmap^ Document::CreateSoftwareBitmap(int width, int height, GP<GBitmap> pGBitmap)
	{
		// create bitmap

		ComPtr<IWICImagingFactory> imagingFactory;
		winrt::check_hresult(CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			(LPVOID*)&imagingFactory
		));

		ComPtr<IWICBitmap> pBitmap;
		winrt::check_hresult(imagingFactory->CreateBitmap(
			width,
			height,
			GUID_WICPixelFormat32bppBGRA, // https://msdn.microsoft.com/en-us/library/windows/desktop/ee719797(v=vs.85).aspx
			WICBitmapCacheOnDemand, // Allocates system memory for the bitmap at initialization
			&pBitmap
		));

		// fill bitmap

		WICRect rcLock = { 0, 0, width, height };
		IWICBitmapLock *pLock = NULL;
		HRESULT hr = pBitmap->Lock(&rcLock, WICBitmapLockWrite, &pLock);
		if (SUCCEEDED(hr))
		{
			UINT cbBufferSize = 0;
			UINT cbStride = 0;
			BYTE *pv = NULL;

			hr = pLock->GetStride(&cbStride);

			if (SUCCEEDED(hr))
			{
				hr = pLock->GetDataPointer(&cbBufferSize, &pv);
			}

			// Clear the image data
			//ZeroMemory(pv, cbBufferSize);
			RenderBitmap(*pGBitmap, pv);

			pLock->Release();
		}

		winrt::check_hresult(hr);

		// convert to SoftwareBitmap

		ComPtr<ISoftwareBitmapNativeFactory> factoryNative;
		winrt::check_hresult(CoCreateInstance(CLSID_SoftwareBitmapNativeFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factoryNative)));

		SoftwareBitmap^ pSoftwareBitmap = nullptr;

		winrt::check_hresult(factoryNative->CreateFromWICBitmap(
			pBitmap.Get(),						// The source WIC bitmap
			false,								// A value indicating whether the created software bitmap is read-only.
			__uuidof(ISoftwareBitmapNative), 
			reinterpret_cast<void**>(&pSoftwareBitmap)));

		return pSoftwareBitmap;
	}

	//void Document::test(Windows::Foundation::IMemoryBufferReference^ ref, int w, int h, int offs, int stride)
	//IAsyncOperation<WriteableBitmap^>^ 
	/*WriteableBitmap^ Document::DrawImage_()
	{
		GP<DjVuImage> pImage = _doc->get_page(0);
		
		GP<GPixmap> pGPixmap;

		const unsigned int width = pImage->get_width();
		const unsigned int height = pImage->get_height();

		GRect rect(0, 0, width, height); // 2550 3300

		pGPixmap = pImage->get_pixmap(rect, rect);
		if (pGPixmap == NULL)
		{
			GP<GBitmap> pGBitmap = pImage->get_bitmap(rect, rect, 4);

			// This method might run faster if you use AMP or the Parallel Patterns Library to parallelize the operation.
			//return concurrency::create_async([this, width, height, pGBitmap]
			//{
				auto bmp = ref new WriteableBitmap(width, height);

				unsigned int length;
				byte* sourcePixels = GetPointerToPixelData(bmp->PixelBuffer, &length);

				RenderBitmap(*pGBitmap, sourcePixels);

				return bmp;
			//});
		}
		else 
		{
			return nullptr;
		}
	}

	byte* Document::GetPointerToPixelData(IBuffer^ pixelBuffer, uint32 *length)
	{
		if (length != nullptr)
		{
			*length = pixelBuffer->Length;
		}
		
		// Query the IBufferByteAccess interface.  
		ComPtr<IBufferByteAccess> bufferByteAccess;
		reinterpret_cast<IInspectable*>(pixelBuffer)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));

		// Retrieve the buffer data.  
		byte* pixels = nullptr;
		bufferByteAccess->Buffer(&pixels);

		return pixels;
	}*/

	//--------------------------------------------------------------------

	std::string ConvertCxStringToUTF8(String^ stringToConvert)
	{
		const wchar_t* data = stringToConvert->Data();

		auto requiredBufferSize = WideCharToMultiByte(
			CP_UTF8,
			WC_ERR_INVALID_CHARS,
			stringToConvert->Data(),
			static_cast<int>(stringToConvert->Length()),
			nullptr,
			0,
			nullptr,
			nullptr
		);

		if (requiredBufferSize == 0)
		{
			auto error = GetLastError();
			throw Exception::CreateException(HRESULT_FROM_WIN32(error));
		}

		requiredBufferSize++;

		std::string buffer(requiredBufferSize, 0);

		auto numBytesWritten = WideCharToMultiByte(
			CP_UTF8,
			WC_ERR_INVALID_CHARS,
			stringToConvert->Data(),
			static_cast<int>(stringToConvert->Length()),
			const_cast<char *>(buffer.data()),
			requiredBufferSize - 1,
			nullptr,
			nullptr
		);

		if (numBytesWritten != (requiredBufferSize - 1))
		{
			throw Exception::CreateException(E_UNEXPECTED, L"WideCharToMultiByte returned an unexpected number of bytes written.");
		}

		return buffer;
	}
}
