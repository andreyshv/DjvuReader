#pragma once

#include "libdjvu\DjVuDocument.h"
#include "libdjvu\GBitmap.h"

//#include <windows.foundation.h>
#include "winrt/Windows.Foundation.h"
#include <string>
//#include <collection.h>

typedef uint8 byte;

namespace djvulibre
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class Document sealed
	{
	public:
		static Document^ Open(Platform::String^ fileName);

		int GetPageCount();
		int GetPageView(int pageNum);

		//void test(Windows::Foundation::IMemoryBufferReference^ ref, int w, int h, int offs, int stride);
		//Windows::Foundation::IAsyncOperation<Windows::UI::Xaml::Media::Imaging::WriteableBitmap^>^ DrawImage();
		//Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ DrawImage_();
		Windows::Graphics::Imaging::SoftwareBitmap^ Document::DrawImage();
	private:
		Document(GP<DjVuDocument> pDoc);

		GP<DjVuDocument> _doc;

		Windows::Graphics::Imaging::SoftwareBitmap^ Document::CreateSoftwareBitmap(int width, int height, GP<GBitmap> pGBitmap);
		//byte* Document::GetPointerToPixelData(Windows::Storage::Streams::IBuffer^ pixelBuffer, uint32 *length);
	};

}