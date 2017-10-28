#pragma once

#include "libdjvu\DjVuDocument.h"
#include "libdjvu\GBitmap.h"

#include "winrt/Windows.Foundation.h"
#include <string>

typedef uint8 byte;

namespace djvulibre
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class Document sealed
	{
	public:
		static Document^ Open(Platform::String^ fileName);
		static Windows::Graphics::Imaging::SoftwareBitmap^ test();

		int GetPageCount();
		int GetPageView(int pageNum);

		Windows::Graphics::Imaging::SoftwareBitmap^ Document::DrawImage();
	private:
		Document(GP<DjVuDocument> pDoc);

		GP<DjVuDocument> _doc;

		static std::string ConvertCxStringToUTF8(Platform::String^ stringToConvert);
		Windows::Graphics::Imaging::SoftwareBitmap^ Document::CreateSoftwareBitmap(GP<GBitmap> pGBitmap);
	};

}