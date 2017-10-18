
using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using DjvuWrapper;

namespace djvulibre.test
{
    [TestClass]
    public class UnitTest1
    {
        [TestMethod]
        public void OpenDoc()
        {
            string fileName = @"Data\test1.djvu";
            var doc = Document.Open(fileName);
        }
    }
}
