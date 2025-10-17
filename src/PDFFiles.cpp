#include "PDFFiles.h"
#include "StringConverter.h"
#include "GdiplusManager.h"
#include "VariantUtils.h"
#include "Logger.h"
#include "FileSystemUtils.h"
#include "PdfSplitManager.h"

PdfFiles::PdfFiles() {
    Logger::Debug("=== PdfFiles component initialized ===");

    AddProperty(L"Version", L"Версия", [&]() {
        return std::make_shared<variant_t>(std::string(Version));
        });

    AddProperty(L"EnableLogging", L"ВключитьЛогирование",
        [&]() {
            return std::make_shared<variant_t>(Logger::IsEnabled());
        },
        [&](const variant_t& val) {
            bool enabled = VariantUtils::GetBool(val);
            Logger::SetEnabled(enabled);

            if (enabled) {
                Logger::Debug("=== Logging enabled ===");

                std::wstring logPathW = Logger::GetLogFilePathW();
                std::string logPathUtf8 = StringConverter::WideToUtf8(logPathW);

                Logger::Debug("Log file path: " + logPathUtf8);

                std::string message = "Log file location: " + logPathUtf8;

                AddError(ADDIN_E_INFO, "EnableLogging", message, false);
            }
            else {
                Logger::Debug("=== Logging disabled ===");
            }
        });

    // ========================================================================
    // СВОЙСТВО: НеУдалятьИсходныеФайлы
    // ========================================================================
    AddProperty(L"KeepSourceFiles", L"НеУдалятьИсходныеФайлы",
        [&]() {
            return std::make_shared<variant_t>(m_keepSourceFiles);
        },
        [&](const variant_t& val) {
            m_keepSourceFiles = VariantUtils::GetBool(val);

            if (m_keepSourceFiles) {
                Logger::Debug("=== Keep source files: ENABLED ===");
                std::string message = "Source files will NOT be deleted after merge";
                AddError(ADDIN_E_INFO, "KeepSourceFiles", message, false);
            }
            else {
                Logger::Debug("=== Keep source files: DISABLED ===");
                std::string message = "Source files WILL be deleted after merge";
                AddError(ADDIN_E_INFO, "KeepSourceFiles", message, false);
            }
        });

    AddMethod(L"MergePDFFiles", L"ОбъединитьPDFФайлы", this, &PdfFiles::MergePDFFiles);
    AddMethod(L"MergePDFFilesWithSplit", L"ОбъединитьPDFФайлыСРазделением",
        this, &PdfFiles::MergePDFFilesWithSplit);
}

std::string PdfFiles::extensionName() {
    return "PdfFiles";
}

void ADDIN_API PdfFiles::Done()
{
    // Освобождаем GDI+ ресурсы
    GdiplusManager::Instance().Shutdown();
}

bool PdfFiles::MergePDFFiles(const variant_t& sourceFolderPath, const variant_t& outputFileName) {
    return PdfFiles::MergePDFFilesWithSplit(sourceFolderPath, outputFileName, 0);
}

bool PdfFiles::DeleteOldOutputFiles(const std::string& folderPath, const std::string& outputFileName) {
    Logger::Debug("Checking for old output files to delete...");

    size_t dotPos = outputFileName.find_last_of('.');
    std::string baseFileName = (dotPos != std::string::npos)
        ? outputFileName.substr(0, dotPos)
        : outputFileName;

    Logger::Debug("Base file name: " + baseFileName);

    std::vector<std::string> allFiles;
    if (!FileSystemUtils::GetFilesFromDirectory(folderPath, allFiles)) {
        Logger::Debug("Failed to read directory for cleanup");
        return false;
    }

    size_t deletedCount = 0;

    std::string mainFilePath = folderPath;
    if (mainFilePath.back() != '\\') {
        mainFilePath += '\\';
    }
    mainFilePath += outputFileName;

    if (FileSystemUtils::FileExists(mainFilePath)) {
        if (FileSystemUtils::DelFile(mainFilePath)) {
            Logger::Debug("Deleted old main file: " + mainFilePath);
            deletedCount++;
        }
    }

    for (const auto& file : allFiles) {
        std::string fileName = FileSystemUtils::GetFileName(file);

        if (fileName.find(baseFileName) == 0 && fileName.find("_part") != std::string::npos) {
            if (FileSystemUtils::DelFile(file)) {
                Logger::Debug("Deleted old part file: " + file);
                deletedCount++;
            }
        }
    }

    if (deletedCount > 0) {
        Logger::Debug("Deleted " + std::to_string(deletedCount) + " old output file(s)");
    }
    else {
        Logger::Debug("No old output files found");
    }

    return true;
}

bool PdfFiles::MergePDFFilesWithSplit(const variant_t& sourceFolderPath,
    const variant_t& outputFileName,
    const variant_t& maxSizeMB) {

    std::string folderPath = StringConverter::SanitizePath(VariantUtils::GetString(sourceFolderPath));
    std::string outputFileName_str = VariantUtils::GetString(outputFileName);
    double sizeLimitMB = VariantUtils::GetDouble(maxSizeMB);

    Logger::SetLogFolder(folderPath);

    Logger::Debug("=== MergePDFFilesWithSplit START ===");

    try {

        Logger::Debug("Source folder: " + folderPath);
        Logger::Debug("Output file name: " + outputFileName_str);
        Logger::Debug("Max size (MB): " + std::to_string(sizeLimitMB));
        Logger::Debug("Keep source files: " + std::string(m_keepSourceFiles ? "YES" : "NO"));

        if (folderPath.empty() || outputFileName_str.empty()) {
            Logger::Error("Empty paths provided");
            AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit",
                "Empty folder or output file name", false);
            return false;
        }

        if (sizeLimitMB < 0) {
            Logger::Error("Negative size limit");
            AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit",
                "File size cannot be negative", false);
            return false;
        }

        Logger::Debug("Checking directory access...");
        if (!FileSystemUtils::DirectoryExists(folderPath)) {
            std::string errorMsg = "Folder is not accessible or does not exist: " + folderPath;
            Logger::Error(errorMsg);
            AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit", errorMsg, false);
            return false;
        }
        Logger::Debug("Directory access OK");

        // ====================================================================
        // НОВОЕ: Удаление старых выходных файлов перед началом
        // ====================================================================
        Logger::Debug("Cleaning up old output files...");
        if (!DeleteOldOutputFiles(folderPath, outputFileName_str)) {
            Logger::Debug("Warning: Could not clean old output files, continuing anyway");
        }

        Logger::Debug("Reading directory contents...");
        std::vector<std::string> allFiles;
        if (!FileSystemUtils::GetFilesFromDirectory(folderPath, allFiles)) {
            Logger::Error("Failed to read directory contents");
            AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit",
                "Failed to read folder contents", false);
            return false;
        }
        Logger::Debug("Found " + std::to_string(allFiles.size()) + " files");

        Logger::Debug("Filtering files by extension...");
        std::vector<std::string> files = FileSystemUtils::FilterFilesByExtension(allFiles);
        Logger::Debug("Filtered to " + std::to_string(files.size()) + " supported files");

        if (files.empty()) {
            Logger::Error("No supported files found");
            AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit",
                "No PDF, JPG or PNG files found in folder", false);
            return false;
        }

        Logger::Debug("Sorting files...");
        FileSystemUtils::SortFilesByName(files);

        // Формируем полный путь выходного файла в каталоге источника
        std::string outputPath = folderPath;
        if (outputPath.back() != '\\') {
            outputPath += '\\';
        }
        outputPath += outputFileName_str;

        Logger::Debug("Creating PDF split manager...");
        PdfSplitManager splitManager(outputPath, sizeLimitMB);

        for (size_t i = 0; i < files.size(); ++i) {
            Logger::Debug("Processing file " + std::to_string(i + 1) + "/" +
                std::to_string(files.size()) + ": " + files[i]);

            if (!splitManager.AddFile(files[i])) {
                std::string errorMsg = "Failed to process file: " +
                    FileSystemUtils::GetFileName(files[i]);
                Logger::Error(errorMsg);
                AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit", errorMsg, false);
                return false;
            }
        }

        Logger::Debug("Finalizing PDF document(s)...");
        if (!splitManager.Finalize()) {
            Logger::Error("Failed to finalize documents");
            AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit",
                "Error saving PDF document", false);
            return false;
        }

        const auto& savedFiles = splitManager.GetSavedFiles();
        if (!savedFiles.empty()) {
            Logger::Debug("Created " + std::to_string(savedFiles.size()) + " file(s):");
            for (const auto& file : savedFiles) {
                Logger::Debug("  - " + file);
            }
        }

        // ====================================================================
        // Удаление исходных файлов согласно флагу
        // ====================================================================
        if (m_keepSourceFiles) {
            Logger::Debug("Source files preservation enabled - skipping deletion");
            Logger::Debug("=== MergePDFFilesWithSplit SUCCESS (files preserved) ===");
        }
        else {
            Logger::Debug("Deleting source files...");
            size_t deletedCount = 0;
            for (const auto& file : files) {
                if (FileSystemUtils::DelFile(file)) {
                    deletedCount++;
                    Logger::Debug("File deleted: " + file);
                }
                else {
                    Logger::Debug("Failed to delete: " + file);
                }
            }
            Logger::Debug("Deleted " + std::to_string(deletedCount) + " source file(s)");
            Logger::Debug("=== MergePDFFilesWithSplit SUCCESS ===");
        }

        return true;
    }
    catch (const PoDoFo::PdfError& e) {
        std::string errorMsg = "PoDoFo error: PdfError code " +
            std::to_string(static_cast<int>(e.GetCode()));
        Logger::Error(errorMsg);
        AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit", errorMsg, false);
        return false;
    }
    catch (const std::exception& e) {
        std::string errorMsg = "Exception: " + std::string(e.what());
        Logger::Error(errorMsg);
        AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit", errorMsg, false);
        return false;
    }
    catch (...) {
        Logger::Error("Unknown exception");
        AddError(ADDIN_E_FAIL, "MergePDFFilesWithSplit",
            "Unknown error while merging files", false);
        return false;
    }
}