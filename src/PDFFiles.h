#ifndef __PDFFILES_H__
#define __PDFFILES_H__

#include "Component.h"
#include <podofo/podofo.h>
#include <string>
#include <vector>

using namespace PoDoFo;

// ============================================================================
// PdfFiles CLASS
// ============================================================================
class PdfFiles final : public Component {

private:
    bool m_keepSourceFiles = false;

public:
    // Component version
    const char* Version = "1.2.0";
    
    PdfFiles();

    std::string extensionName() override;
    void ADDIN_API Done() override;

    bool DeleteOldOutputFiles(const std::string& folderPath, const std::string& outputFileName);
    bool MergePDFFiles(const variant_t& sourceFolderPath, const variant_t& outputFileName);
    bool MergePDFFilesWithSplit(const variant_t& sourceFolderPath, const variant_t& outputFileName, const variant_t& maxSizeMB);
};

#endif // __PDFFILES_H__