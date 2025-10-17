#pragma once
#include <string>
#include <vector>
#include <memory>
#include "podofo/podofo.h"
#include "FileSystemUtils.h"

class PdfSplitManager {
public:
    
    PdfSplitManager(const std::string& basePath, double maxSizeMB);
    ~PdfSplitManager();

    bool AddFile(const std::string& filePath);
    bool Finalize();
    const std::vector<std::string>& GetSavedFiles() const;

private:
    static constexpr size_t BYTES_IN_MEGABYTE = 1024 * 1024;

    std::string basePath_;
    size_t accumulatedSize_ = 0;
    size_t maxSizeBytes_;
    int currentPart_;
    std::unique_ptr<PoDoFo::PdfMemDocument> currentDoc_;
    std::vector<std::string> savedFiles_;

    bool SaveCurrentDocument(const std::string& outputPath = "");
    bool ShouldStartNewPart(size_t additionalSize) const;
};