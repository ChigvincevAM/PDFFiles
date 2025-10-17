#include "Logger.h"
#include "PdfProcessor.h"
#include "FileSystemUtils.h"
#include "ImageProcessor.h"
#include "podofo/main/PdfError.h"
#include "podofo/main/PdfPainter.h"

bool PdfProcessor::AppendPdfFile(PoDoFo::PdfMemDocument& outputDoc, const std::string& filePath) {

    Logger::Debug("AppendPdfFile: " + filePath);
    try {
        std::vector<char> buffer;
        if (!FileSystemUtils::ReadFileToBuffer(filePath, buffer)) {
            return false;
        }

        PoDoFo::PdfMemDocument inputDoc;
        inputDoc.LoadFromBuffer(PoDoFo::bufferview(buffer.data(), buffer.size()));
        outputDoc.GetPages().AppendDocumentPages(inputDoc);

        Logger::Debug("PDF appended successfully");
        return true;
    }
    catch (const PoDoFo::PdfError& e) {
        Logger::Error("PdfError code: " + std::to_string(static_cast<int>(e.GetCode())));
        return false;
    }
    catch (const std::exception& e) {
        Logger::Error("Exception: " + std::string(e.what()));
        return false;
    }
}

bool PdfProcessor::AppendImageFile(PoDoFo::PdfMemDocument& outputDoc, const std::string& filePath) {
    
    Logger::Debug("AppendImageFile: " + filePath);
    try {
        std::vector<unsigned char> jpegData;
        unsigned int imgW = 0, imgH = 0;

        if (!ImageProcessor::LoadAndConvertToJpeg(filePath, jpegData, imgW, imgH)) {
            Logger::Error("Failed to load/convert image");
            return false;
        }

        PoDoFo::PdfPage& page = outputDoc.GetPages().CreatePage(
            PoDoFo::Rect(0.0, 0.0, A4_PAGE_WIDTH, A4_PAGE_HEIGHT)
        );

        PoDoFo::bufferview buffer(reinterpret_cast<const char*>(jpegData.data()), jpegData.size());
        auto imagePtr = outputDoc.CreateImage();
        imagePtr->LoadFromBuffer(buffer);

        double podofoWidth = imagePtr->GetWidth();
        double podofoHeight = imagePtr->GetHeight();
        Logger::Debug("PoDoFo dimensions: " + std::to_string(podofoWidth) + "x" + std::to_string(podofoHeight));

        double scaleX = A4_PAGE_WIDTH / podofoWidth;
        double scaleY = A4_PAGE_HEIGHT / podofoHeight;
        double scale = min(scaleX, scaleY);

        double finalWidth = imgW * scale;
        double finalHeight = imgH * scale;
        double x = A4_PAGE_WIDTH - finalWidth;
        double y = A4_PAGE_HEIGHT - finalHeight;

        Logger::Debug("Final position: x=" + std::to_string(x) + ", y=" + std::to_string(y));
        Logger::Debug("Final size: " + std::to_string(finalWidth) + "x" + std::to_string(finalHeight));

        double drawScaleX = finalWidth / podofoWidth;
        double drawScaleY = finalHeight / podofoHeight;
        Logger::Debug("DrawImage scale multipliers: scaleX=" + std::to_string(drawScaleX) + ", scaleY=" + std::to_string(drawScaleY));

        PoDoFo::PdfPainter painter;
        painter.SetCanvas(page);
        painter.DrawImage(*imagePtr, x, y, drawScaleX, drawScaleY);
        painter.FinishDrawing();

        Logger::Debug("Image appended successfully");

        jpegData.clear();
        jpegData.shrink_to_fit();

        return true;
    }
    catch (const PoDoFo::PdfError& e) {
        Logger::Error("PdfError code: " + std::to_string(static_cast<int>(e.GetCode())));
        return false;
    }
    catch (const std::exception& e) {
        Logger::Error("Exception: " + std::string(e.what()));
        return false;
    }
}

bool PdfProcessor::ProcessFile(PoDoFo::PdfMemDocument& outputDoc, const std::string& filePath) {
    std::string ext = FileSystemUtils::GetFileExtension(filePath);
    Logger::Debug("Processing file: " + filePath + " (" + ext + ")");

    if (ext == ".pdf") {
        return AppendPdfFile(outputDoc, filePath);
    }
    else if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
        return AppendImageFile(outputDoc, filePath);
    }

    Logger::Error("Unsupported file extension: " + ext);
    return false;
}