using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using djvulibre;
using System.Runtime.InteropServices;
using Windows.Graphics.Imaging;
using Windows.UI.Xaml.Media.Imaging;

namespace DjvuReader
{
    [ComImport]
    [Guid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    unsafe interface IMemoryBufferByteAccess
    {
        void GetBuffer(out byte* buffer, out uint capacity);
    }

    class Source
    {
        public static unsafe SoftwareBitmap GetImage_()
        {
            var softwareBitmap = new SoftwareBitmap(BitmapPixelFormat.Bgra8, 100, 100, BitmapAlphaMode.Premultiplied);

            using (BitmapBuffer buffer = softwareBitmap.LockBuffer(BitmapBufferAccessMode.Write))
            {
                using (var reference = buffer.CreateReference())
                {
                    byte* dataInBytes;
                    uint capacity;
                    ((IMemoryBufferByteAccess)reference).GetBuffer(out dataInBytes, out capacity);
                    
                    // Fill-in the BGRA plane
                    BitmapPlaneDescription bufferLayout = buffer.GetPlaneDescription(0);
                    for (int i = 0; i < bufferLayout.Height; i++)
                    {
                        for (int j = 0; j < bufferLayout.Width; j++)
                        {
                            byte value = (byte)((float)j / bufferLayout.Width * 255);
                            dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 0] = value; // Blue
                            dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 1] = value; // Green
                            dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 2] = value; // Red
                            dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 3] = 255; // Alpha
                        }
                    }
                }
            }

            return softwareBitmap;
        }

        public static unsafe SoftwareBitmap GetImage(Document doc)
        {
            var softwareBitmap = new SoftwareBitmap(BitmapPixelFormat.Gray8, 100, 100);

            using (BitmapBuffer buffer = softwareBitmap.LockBuffer(BitmapBufferAccessMode.Write))
            {
                using (var reference = buffer.CreateReference())
                {
                    // Fill-in the Gray plane
                    BitmapPlaneDescription bufferLayout = buffer.GetPlaneDescription(0);
                    //doc.test(reference, 100, 100, bufferLayout.StartIndex, bufferLayout.Stride);
                }
            }

            return SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
        }
    }
}
