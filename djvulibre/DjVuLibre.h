#pragma once

#include "libdjvu\DjVuDocument.h"

using namespace Platform;

namespace djvulibre
{
	public ref class Document sealed
	{
	public:
		static int test1(int a);
		static String^ test2(String^ b);
		static Document^ Open(String^ fileName);

		int GetPageCount();
	private:
		Document(GP<DjVuDocument> pDoc);

		GP<DjVuDocument> _doc;
	};

}