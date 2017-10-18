#include "DjVuLibre.h"
#include "libdjvu\GString.h"
#include <string>

namespace djvulibre
{
	std::string ConvertCxStringToUTF8(String^ stringToConvert);

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

	int Document::test1(int a)
	{
		return a + 2;
	}

	String^ Document::test2(String^ b)
	{
		return "zzz" + b;
	}

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
