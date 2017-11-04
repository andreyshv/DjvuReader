#include "Wrapper.h"
#include "libdjvu\GString.h"
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

using namespace DJVU;

namespace djvulibre
{
	struct __declspec(uuid("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")) __declspec(novtable) IMemoryBufferByteAccess : ::IUnknown
	{
		virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
	};

	Document^ Document::Open(String^ fileName)
	{
		// DjVuSource::FromFile
		GP<DjVuDocument> pDoc = NULL;
		std::string utf8Name = ConvertCxStringToUTF8(fileName);
		auto gstr = GUTF8String::create(utf8Name);
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

	void RenderPixmap(GPixmap& pm, byte* sourcePixels, int startIndex, int stride)
	{
		int width = pm.columns();
		int height = pm.rows();

		// RGB -> BGRA
		for (int y = 0; y < height; ++y)
		{
			auto line = pm[y];
			int offs = startIndex + y * stride;
			for (int x = 0; x < width; ++x)
			{
				sourcePixels[offs + 4 * x + 0] = line[x].b;
				sourcePixels[offs + 4 * x + 1] = line[x].g;
				sourcePixels[offs + 4 * x + 2] = line[x].r;
				sourcePixels[offs + 4 * x + 3] = 0xff;
			}
		}
	}

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

	void RenderBitmap(GBitmap& bm, byte* sourcePixels, int startIndex, int stride)
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
			int offs = startIndex + y * stride / 4;
			for (int x = 0; x < width; ++x) 
			{
				uint32 value = pBMI[line[x]];
				pixels[offs + x] = value;
			}
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
		
		return NULL;
	}

	SoftwareBitmap^ Document::DrawImage()
	{
		GP<DjVuImage> pImage = _doc->get_page(0);
		pImage->set_rotate(0);

		const int width = pImage->get_width();
		const int height = pImage->get_height();

		GRect rect(0, 0, width, height); // 2550 3300

		// high level
		//GP<GPixmap> pGPixmap = pImage->get_pixmap(rect, rect);
		// low level
		GP<GPixmap> pGPixmap = pImage->get_pixmap(rect, 1, 0, GPixel::WHITE);
		if (pGPixmap != NULL)
		{
			return CreateSoftwareBitmap(pGPixmap);
		} 
		
		// high level
		//GP<GBitmap> pGBitmap = pImage->get_bitmap(rect, rect, 4);
		// low level (s - scale)
		GP<GBitmap> pGBitmap = pImage->get_bitmap(rect, 1, 4);

		// This method might run faster if you use AMP or the Parallel Patterns Library to parallelize the operation.
		//return concurrency::create_async([this, width, height, pGBitmap]
		//{
		return CreateSoftwareBitmap(pGBitmap);
		//});
		

		//return nullptr;
	}

	// add link to Windowscodecs.lib
	// T = GBitmap | GPixmap
	template<typename T>
	SoftwareBitmap^ Document::CreateSoftwareBitmap(GP<T> pBmp)
	{
		UINT width;
		UINT height;

		//if (pGBitmap)
		//{
		//	width = pGBitmap->columns();
		//	height = pGBitmap->rows();
		//}
		//else
		//{
		//	width = pGPixmap->columns();
		//	height = pGPixmap->rows();
		//}
		width = pBmp->columns();
		height = pBmp->rows();

		// create bitmap

		auto softwareBitmap = ref new SoftwareBitmap(BitmapPixelFormat::Bgra8, width, height, BitmapAlphaMode::Premultiplied);

		// get raw data pointer

		BitmapBuffer^ buffer = softwareBitmap->LockBuffer(BitmapBufferAccessMode::Write);
		IMemoryBufferReference^ reference = buffer->CreateReference();

		ComPtr<IMemoryBufferByteAccess> byteAccess;
		winrt::check_hresult(reinterpret_cast<::IUnknown*>(reference)->QueryInterface(__uuidof(IMemoryBufferByteAccess), &byteAccess));

		byte* dataInBytes;
		uint32_t capacity;
		winrt::check_hresult(byteAccess->GetBuffer(&dataInBytes, &capacity));

		// fill bitmap

		BitmapPlaneDescription bufferLayout = buffer->GetPlaneDescription(0);
		//if (pGBitmap)
		//	RenderBitmap(*pGBitmap, dataInBytes, bufferLayout.StartIndex, bufferLayout.Stride);
		//else
		//	RenderPixmap(*pGPixmap, dataInBytes, bufferLayout.StartIndex, bufferLayout.Stride);

		pBmp->convertToBgra(dataInBytes + bufferLayout.StartIndex, bufferLayout.Stride);

		return softwareBitmap;
	}

	//--------------------------------------------------------------------

	std::string Document::ConvertCxStringToUTF8(String^ stringToConvert)
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

	//-------------------------------------------------
	//
	static void
		fmt_convert_row(const GPixel *p, int w, char *buf)
	{
		//case DDJVU_FORMAT_BGR24:    /* truecolor 24 bits in BGR order */
		memcpy(buf, (const char*)p, 3 * w);
	}

	static void
		fmt_convert_row(unsigned char *p, unsigned char g[256][4], int w,
			char *buf)
	{
		//case DDJVU_FORMAT_BGR24:    /* truecolor 24 bits in BGR order */
			while (--w >= 0) {
				buf[0] = g[*p][0];
				buf[1] = g[*p][1];
				buf[2] = g[*p][2];
				buf += 3; p += 1;
			}
	}


	static void
		fmt_convert(GBitmap *bm, char *buffer, int rowsize)
	{
		int w = bm->columns();
		int h = bm->rows();
		int m = bm->get_grays();
		// Gray levels
		int i;
		unsigned char g[256][4];
		const GPixel &wh = GPixel::WHITE;
		for (i = 0; i<m; i++)
		{
			g[i][0] = wh.b - (i * wh.b + (m - 1) / 2) / (m - 1);
			g[i][1] = wh.g - (i * wh.g + (m - 1) / 2) / (m - 1);
			g[i][2] = wh.r - (i * wh.r + (m - 1) / 2) / (m - 1);
			g[i][3] = (5 * g[i][2] + 9 * g[i][1] + 2 * g[i][0]) >> 4;
		}
		for (i = m; i<256; i++)
			g[i][0] = g[i][1] = g[i][2] = g[i][3] = 0;

		// Loop on rows
		//if (fmt->rtoptobottom)
		for (int r = h - 1; r >= 0; r--, buffer += rowsize)
		{
			fmt_convert_row((*bm)[r], g, w, buffer);
		}
	}

	static void
		fmt_convert(GPixmap *pm, char *buffer, int rowsize)
	{
		int w = pm->columns();
		int h = pm->rows();
		// Loop on rows
		//if (fmt->rtoptobottom)
		for (int r = h - 1; r >= 0; r--, buffer += rowsize)
			fmt_convert_row((*pm)[r], w, buffer);
	}

	void
		render(DjVuDocument *doc, int pageno)
	{
		GP<DjVuImage> page = doc->get_page(pageno);
		
		GRect prect;
		int iw = page->get_width();
		int ih = page->get_height();
		int dpi = page->get_dpi();

		char white = (char)0xFF;
		int rowsize;

		/* Process size specification */
		prect.xmin = 0;
		prect.ymin = 0;
		prect.ymax = iw;
		prect.xmax = ih;

		/* Process mode specification */
		
		//mode = DDJVU_RENDER_COLOR;

		/* Determine output pixel format and compression */
		
		//ddjvu_format_style_t style = DDJVU_FORMAT_RGB24;
		//ddjvu_format_t * fmt = ddjvu_format_create(style, 0, 0);
		//fmt->rtoptobottom = true;

		/* Allocate buffer */
		
		rowsize = iw * 3;
		char *image = (char*)malloc(rowsize * ih);
			
		/* Render */
		
		//ddjvu_page_render(page, mode, &prect, &rrect, fmt, rowsize, image);
		
		GP<GPixmap> pm;
		GP<GBitmap> bm;

		pm = page->get_pixmap(prect,prect, 2.2, GPixel::WHITE);
		if (!pm)
			bm = page->get_bitmap(prect,prect);
			
		if (pm)
		{
			fmt_convert(pm, image, rowsize);
		}
		else if (bm)
		{
			fmt_convert(bm, image, rowsize);
		}
	}
}
