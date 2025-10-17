#ifndef __PDF_PROCESSOR_H__
#define __PDF_PROCESSOR_H__

#include <string>
#include "podofo/main/PdfMemDocument.h"

constexpr double A4_PAGE_WIDTH = 595.0;
constexpr double A4_PAGE_HEIGHT = 842.0;

class PdfProcessor {
public:
    static bool AppendPdfFile(PoDoFo::PdfMemDocument& outputDoc, const std::string& filePath);
    static bool AppendImageFile(PoDoFo::PdfMemDocument& outputDoc, const std::string& filePath);
    static bool ProcessFile(PoDoFo::PdfMemDocument& outputDoc, const std::string& filePath);
};

#endif // __PDF_PROCESSOR_H__
