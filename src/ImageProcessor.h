#ifndef __IMAGE_PROCESSOR_H__
#define __IMAGE_PROCESSOR_H__

#include <string>
#include <vector>
#include <windows.h>

class ImageProcessor {
public:
    static bool LoadAndConvertToJpeg(
        const std::string& filePath,
        std::vector<unsigned char>& outData,
        unsigned int& width,
        unsigned int& height
    );

private:
    static CLSID GetEncoderClsid(const WCHAR* format);
};

#endif // __IMAGE_PROCESSOR_H__
