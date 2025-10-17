#include <podofo/podofo.h>
#include "Logger.h"
#include "PdfSplitManager.h"
#include "PdfProcessor.h"

PdfSplitManager::PdfSplitManager(const std::string& basePath, double maxSizeMB)
    : basePath_(basePath)
    , currentPart_(1)
    , accumulatedSize_(0)
    , currentDoc_(std::make_unique<PoDoFo::PdfMemDocument>()) {

    maxSizeBytes_ = (maxSizeMB > 0)
        ? static_cast<size_t>(maxSizeMB * BYTES_IN_MEGABYTE)
        : 0;

    if (maxSizeBytes_ > 0) {
        Logger::Debug("Split mode enabled: max size = " + std::to_string(maxSizeMB) + " MB (" +
            std::to_string(maxSizeBytes_) + " bytes)");
    }
    else {
        Logger::Debug("Split mode disabled");
    }
}

PdfSplitManager::~PdfSplitManager() {
    Logger::Debug("PdfSplitManager destructor BEGIN");
    try {
        if (currentDoc_) {
            size_t pages = currentDoc_->GetPages().GetCount();
            Logger::Debug("Destroying currentDoc_ with " + std::to_string(pages) + " pages");
            currentDoc_->CollectGarbage();
            currentDoc_->Reset();
            currentDoc_.reset();
        }
        Logger::Debug("PdfSplitManager destructor END");
    }
    catch (const std::exception& e) {
        Logger::Debug(std::string("Exception in destructor: ") + e.what());
    }
}

bool PdfSplitManager::ShouldStartNewPart(size_t additionalSize) const {
    if (maxSizeBytes_ == 0) {
        return false;
    }

    size_t estimatedSize = accumulatedSize_ + additionalSize;
    bool shouldSplit = estimatedSize >= maxSizeBytes_;

    if (shouldSplit) {
        Logger::Debug("Size threshold reached: " + std::to_string(estimatedSize) +
            " >= " + std::to_string(maxSizeBytes_));
    }

    return shouldSplit;
}

bool PdfSplitManager::SaveCurrentDocument(const std::string& outputPath) {
    if (!currentDoc_ || currentDoc_->GetPages().GetCount() == 0) {
        return true;
    }

    std::string path = outputPath.empty() ? basePath_ : outputPath;
    bool isSplit = !outputPath.empty();

    if (isSplit) {
        Logger::Debug("Saving part " + std::to_string(currentPart_) + ": " + path);
    }
    else {
        Logger::Debug("Saving single file: " + path);
    }

    try {
        PoDoFo::charbuff buffer;
        {
            PoDoFo::BufferStreamDevice device(buffer);
            currentDoc_->Save(device);
        }

        Logger::Debug("Document saved to buffer, size: " + std::to_string(buffer.size()));

        if (buffer.empty()) {
            Logger::Error("Buffer is empty after save");
            return false;
        }

        if (!FileSystemUtils::WriteBufferToFile(path, buffer.data(), buffer.size())) {
            Logger::Error("Failed to write buffer to file: " + path);
            FileSystemUtils::DelFile(path);
            return false;
        }

        size_t fileSize = FileSystemUtils::GetFileSize(path);
        Logger::Debug("Successfully wrote " + std::to_string(fileSize) + " bytes to: " + path);
        Logger::Debug("Saved file size: " + std::to_string(fileSize) + " bytes (" +
            std::to_string(fileSize / BYTES_IN_MEGABYTE) + " MB)");

        if (isSplit) {
            savedFiles_.push_back(path);
        }

        return true;
    }
    catch (const PoDoFo::PdfError& e) {
        Logger::Error("Failed to save document: PdfError code " +
            std::to_string(static_cast<int>(e.GetCode())));
        return false;
    }
    catch (const std::exception& e) {
        Logger::Error("Exception: " + std::string(e.what()));
        return false;
    }
}

bool PdfSplitManager::AddFile(const std::string& filePath) {
    Logger::Debug("Processing file: " + filePath + " (.pdf)");

    size_t fileSize = FileSystemUtils::GetFileSize(filePath);

    if (ShouldStartNewPart(fileSize) && currentDoc_->GetPages().GetCount() > 0) {
        std::string partPath = FileSystemUtils::GeneratePartFileName(basePath_, currentPart_);
        if (!SaveCurrentDocument(partPath)) {
            return false;
        }

        currentPart_++;
        accumulatedSize_ = 0;
        currentDoc_ = std::make_unique<PoDoFo::PdfMemDocument>();

        Logger::Debug("Started new document part #" + std::to_string(currentPart_));
    }

    Logger::Debug("AppendPdfFile: " + filePath);
    bool result = PdfProcessor::ProcessFile(*currentDoc_, filePath);

    if (result) {
        accumulatedSize_ += fileSize;
        Logger::Debug("PDF appended successfully");
    }
    else {
        Logger::Error("Failed to append PDF file: " + filePath);
    }

    return result;
}

bool PdfSplitManager::Finalize() {
    Logger::Debug("Finalizing PDF document(s)...");

    if (maxSizeBytes_ == 0) {
        Logger::Debug("Saving single file: " + basePath_);
        if (!currentDoc_) {
            Logger::Error("Current document is null during finalization");
            return false;
        }
        return SaveCurrentDocument();
    }
    else {
        if (currentDoc_ && currentDoc_->GetPages().GetCount() > 0) {
            std::string finalPath = FileSystemUtils::GeneratePartFileName(basePath_, currentPart_);
            if (!SaveCurrentDocument(finalPath)) {
                return false;
            }
        }

        Logger::Debug("Created " + std::to_string(savedFiles_.size()) + " file(s):");
        for (const auto& file : savedFiles_) {
            Logger::Debug("  - " + file);
        }
        Logger::Debug("Total parts created: " + std::to_string(savedFiles_.size()));

        return true;
    }
}

const std::vector<std::string>& PdfSplitManager::GetSavedFiles() const {
    return savedFiles_;
}