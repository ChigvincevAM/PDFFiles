#include "GdiplusManager.h"
#include "Logger.h"
#include "ImageProcessor.h"
#include "StringConverter.h"


CLSID ImageProcessor::GetEncoderClsid(const WCHAR* format) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return CLSID{ 0 };

    std::vector<BYTE> buffer(size);
    Gdiplus::ImageCodecInfo* pImageCodecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
            return pImageCodecInfo[j].Clsid;
    }
    return CLSID{ 0 };
}

bool ImageProcessor::LoadAndConvertToJpeg(
    const std::string& filePath,
    std::vector<unsigned char>& outData,
    unsigned int& width,
    unsigned int& height
) {
    GdiplusManager::Instance().EnsureInitialized();

    std::wstring widePath = StringConverter::Utf8ToWide(filePath);
    if (widePath.empty()) {
        Logger::Error("Invalid file path (empty after UTF-8 conversion)");
        return false;
    }

    Gdiplus::Bitmap* memoryBitmap = nullptr;

    {
        Gdiplus::Bitmap tempBitmap(widePath.c_str());
        if (tempBitmap.GetLastStatus() != Gdiplus::Ok) {
            Logger::Error("Failed to load image: " + filePath);
            return false;
        }

        width = tempBitmap.GetWidth();
        height = tempBitmap.GetHeight();

        memoryBitmap = tempBitmap.Clone(0, 0, width, height, PixelFormat32bppARGB);
    }

    if (!memoryBitmap || memoryBitmap->GetLastStatus() != Gdiplus::Ok) {
        Logger::Error("Failed to clone bitmap to memory");
        if (memoryBitmap) delete memoryBitmap;
        return false;
    }

    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(nullptr, TRUE, &pStream) != S_OK) {
        Logger::Error("Failed to create memory stream");
        delete memoryBitmap;
        return false;
    }

    CLSID clsidJpeg = GetEncoderClsid(L"image/jpeg");
    if (clsidJpeg.Data1 == 0) {
        Logger::Error("JPEG encoder not found");
        pStream->Release();
        delete memoryBitmap;
        return false;
    }

    if (memoryBitmap->Save(pStream, &clsidJpeg, nullptr) != Gdiplus::Ok) {
        Logger::Error("Failed to save image to JPEG stream");
        pStream->Release();
        delete memoryBitmap;
        return false;
    }

    delete memoryBitmap;
    memoryBitmap = nullptr;

    STATSTG stat;
    if (pStream->Stat(&stat, STATFLAG_NONAME) != S_OK) {
        Logger::Error("Failed to get stream size");
        pStream->Release();
        return false;
    }

    ULONG size = static_cast<ULONG>(stat.cbSize.QuadPart);
    outData.resize(size);

    LARGE_INTEGER liZero = {};
    pStream->Seek(liZero, STREAM_SEEK_SET, nullptr);

    ULONG bytesRead = 0;
    HRESULT hr = pStream->Read(outData.data(), size, &bytesRead);
    pStream->Release();

    if (FAILED(hr) || bytesRead != size) {
        Logger::Error("Failed to read JPEG data from stream");
        return false;
    }

    Logger::Debug("Image loaded and converted to JPEG: " + filePath +
        " (" + std::to_string(width) + "x" + std::to_string(height) + ")");
    return true;
}
