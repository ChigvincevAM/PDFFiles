#include <sstream>
#include <iomanip>
#include <algorithm>
#include <windows.h>

#include "FileSystemUtils.h"
#include "Logger.h"
#include "StringConverter.h"

bool FileSystemUtils::DirectoryExists(const std::string& path) {
    if (path.empty()) {
        Logger::Error("DirectoryExists: empty path provided");
        return false;
    }

    try {
        std::wstring widePath = StringConverter::Utf8ToWide(path);
        if (widePath.empty()) {
            Logger::Error("DirectoryExists: failed to convert path to wide");
            return false;
        }

        Logger::Debug("DirectoryExists: calling GetFileAttributesW...");
        DWORD attributes = GetFileAttributesW(widePath.c_str());

        if (attributes == INVALID_FILE_ATTRIBUTES) {
            DWORD error = GetLastError();
            Logger::Error("DirectoryExists: GetFileAttributesW failed");
            Logger::Error("  Path length: " + std::to_string(widePath.length()));
            Logger::Error("  Error code: " + std::to_string(error));

            switch (error) {
            case ERROR_FILE_NOT_FOUND:
                Logger::Error("  Reason: File/directory not found");
                break;
            case ERROR_PATH_NOT_FOUND:
                Logger::Error("  Reason: Path not found - check if network is accessible");
                break;
            case ERROR_ACCESS_DENIED:
                Logger::Error("  Reason: Access denied - check user permissions");
                break;
            case ERROR_NETWORK_UNREACHABLE:
                Logger::Error("  Reason: Network unreachable");
                break;
            case ERROR_BAD_NETPATH:
                Logger::Error("  Reason: Bad network path");
                break;
            default:
                Logger::Error("  Reason: Unknown error");
                break;
            }
            return false;
        }

        bool isDirectory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (!isDirectory) {
            Logger::Error("DirectoryExists: path exists but is not a directory");
            return false;
        }

        Logger::Debug("DirectoryExists: directory found and accessible");
        return true;
    }
    catch (const std::exception& e) {
        Logger::Error("DirectoryExists: exception - " + std::string(e.what()));
        return false;
    }
}

bool FileSystemUtils::GetFilesFromDirectory(const std::string& folderPath, std::vector<std::string>& files) {
    std::wstring widePath = StringConverter::Utf8ToWide(folderPath);
    if (widePath.empty()) return false;

    if (widePath.back() != L'\\') widePath += L'\\';
    widePath += L'*';

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(widePath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;

        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring fullPath = StringConverter::Utf8ToWide(folderPath);
            if (fullPath.back() != L'\\') fullPath += L'\\';
            fullPath += findData.cFileName;

            std::string utf8Path = StringConverter::WideToUtf8(fullPath);
            if (!utf8Path.empty()) {
                files.push_back(utf8Path);
            }
        }
    } while (FindNextFileW(hFind, &findData) != 0);

    FindClose(hFind);
    return true;
}

size_t FileSystemUtils::GetFileSize(const std::string& filePath) {
    std::wstring widePath = StringConverter::Utf8ToWide(filePath);
    if (widePath.empty()) return 0;

    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExW(widePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        LARGE_INTEGER size;
        size.HighPart = fileInfo.nFileSizeHigh;
        size.LowPart = fileInfo.nFileSizeLow;
        return static_cast<size_t>(size.QuadPart);
    }
    return 0;
}

bool FileSystemUtils::DelFile(const std::string& filePath) {
    if (filePath.empty()) {
        Logger::Debug("Cannot delete file: empty path");
        return false;
    }

    try {
        std::wstring widePath = StringConverter::Utf8ToWide(filePath);

        DWORD attributes = GetFileAttributesW(widePath.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            Logger::Debug("File does not exist: " + filePath);
            return true;
        }

        if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
            Logger::Error("Path is a directory, not a file: " + filePath);
            return false;
        }

        if (DeleteFileW(widePath.c_str())) {
            Logger::Debug("File deleted: " + filePath);
            return true;
        }
        else {
            DWORD error = GetLastError();
            Logger::Error("Failed to delete file: " + filePath +
                " (WinAPI error: " + std::to_string(error) + ")");
            return false;
        }
    }
    catch (const std::exception& e) {
        Logger::Error("Exception while deleting file: " + std::string(e.what()));
        return false;
    }
}

std::string FileSystemUtils::GetFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos)
        return "";
    return StringConverter::ToLowercase(filePath.substr(dotPos));
}

std::string FileSystemUtils::GetFileName(const std::string& filePath) {
    size_t pos = filePath.find_last_of("\\/");
    return (pos != std::string::npos) ? filePath.substr(pos + 1) : filePath;
}

std::string FileSystemUtils::GetFileNameWithoutExtension(const std::string& filePath) {
    std::string fileName = GetFileName(filePath);
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos != std::string::npos)
        return fileName.substr(0, dotPos);
    return fileName;
}

std::string FileSystemUtils::GetFileDirectory(const std::string& filePath) {
    size_t pos = filePath.find_last_of("\\/");
    if (pos != std::string::npos)
        return filePath.substr(0, pos);
    return "";
}

std::string FileSystemUtils::GeneratePartFileName(const std::string& basePath, int partNumber) {
    std::string dir = GetFileDirectory(basePath);
    std::string nameWithoutExt = GetFileNameWithoutExtension(basePath);

    std::ostringstream oss;
    oss << nameWithoutExt << "_part" << std::setfill('0') << std::setw(3) << partNumber << ".pdf";

    if (!dir.empty())
        return dir + "\\" + oss.str();

    return oss.str();
}

bool FileSystemUtils::IsSupportedExtension(const std::string& extension) {
    return extension == ".pdf" || extension == ".jpg" ||
        extension == ".jpeg" || extension == ".png";
}

std::vector<std::string> FileSystemUtils::FilterFilesByExtension(const std::vector<std::string>& files) {
    std::vector<std::string> filtered;
    for (const auto& file : files) {
        if (IsSupportedExtension(GetFileExtension(file))) {
            filtered.push_back(file);
        }
    }
    return filtered;
}

void FileSystemUtils::SortFilesByName(std::vector<std::string>& files) {
    std::sort(files.begin(), files.end(),
        [](const std::string& a, const std::string& b) {
            return GetFileName(a) < GetFileName(b);
        });
}

bool FileSystemUtils::ReadFileToBuffer(const std::string& filePath, std::vector<char>& buffer) {
    if (filePath.empty()) {
        Logger::Error("ReadFileToBuffer: empty file path");
        return false;
    }

    std::wstring widePath = StringConverter::Utf8ToWide(filePath);
    if (widePath.empty()) {
        Logger::Error("ReadFileToBuffer: invalid file path (empty after conversion): " + filePath);
        return false;
    }

    try {
        Logger::Debug("ReadFileToBuffer: widePath length: " + std::to_string(widePath.length()));

        HANDLE hFile = CreateFileW(
            widePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            Logger::Error("ReadFileToBuffer: cannot open file");
            Logger::Error("  Original path length: " + std::to_string(filePath.length()));
            Logger::Error("  Wide path length: " + std::to_string(widePath.length()));
            Logger::Error("  WinAPI error code: " + std::to_string(error));

            switch (error) {
            case ERROR_FILE_NOT_FOUND:
                Logger::Error("  Reason: File not found");
                break;
            case ERROR_PATH_NOT_FOUND:
                Logger::Error("  Reason: Path not found");
                break;
            case ERROR_ACCESS_DENIED:
                Logger::Error("  Reason: Access denied - check file permissions");
                break;
            case ERROR_SHARING_VIOLATION:
                Logger::Error("  Reason: File is locked by another process");
                break;
            case ERROR_NETWORK_ACCESS_DENIED:
                Logger::Error("  Reason: Network access denied");
                break;
            default:
                Logger::Error("  Reason: Unknown error");
                break;
            }
            return false;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize)) {
            DWORD error = GetLastError();
            Logger::Error("ReadFileToBuffer: cannot get file size: " + filePath);
            Logger::Error("  WinAPI error code: " + std::to_string(error));
            CloseHandle(hFile);
            return false;
        }

        if (fileSize.QuadPart <= 0) {
            Logger::Error("ReadFileToBuffer: empty or invalid file: " + filePath);
            CloseHandle(hFile);
            return false;
        }

        if (fileSize.QuadPart > 500 * 1024 * 1024) {
            Logger::Error("ReadFileToBuffer: file too large: " + filePath +
                " (" + std::to_string(fileSize.QuadPart / (1024 * 1024)) + " MB)");
            CloseHandle(hFile);
            return false;
        }

        size_t fileDataSize = static_cast<size_t>(fileSize.QuadPart);
        buffer.clear();
        buffer.resize(fileDataSize);

        DWORD bytesRead = 0;
        if (!ReadFile(hFile, buffer.data(), static_cast<DWORD>(fileDataSize), &bytesRead, NULL)) {
            DWORD error = GetLastError();
            Logger::Error("ReadFileToBuffer: failed to read file: " + filePath);
            Logger::Error("  WinAPI error code: " + std::to_string(error));
            CloseHandle(hFile);
            buffer.clear();
            return false;
        }

        if (bytesRead != fileDataSize) {
            Logger::Error("ReadFileToBuffer: incomplete read: " + filePath);
            Logger::Error("  Expected: " + std::to_string(fileDataSize) +
                " bytes, got: " + std::to_string(bytesRead));
            CloseHandle(hFile);
            buffer.clear();
            return false;
        }

        CloseHandle(hFile);

        Logger::Debug("ReadFileToBuffer: successfully read " + std::to_string(bytesRead) +
            " bytes from: " + filePath);
        return true;
    }
    catch (const std::exception& e) {
        Logger::Error("ReadFileToBuffer: exception - " + std::string(e.what()));
        buffer.clear();
        return false;
    }
}

bool FileSystemUtils::WriteBufferToFile(const std::string& filePath, const char* data, size_t size) {
    std::wstring widePath = StringConverter::Utf8ToWide(filePath);
    if (widePath.empty()) {
        Logger::Error("Invalid file path (empty after conversion): " + filePath);
        return false;
    }

    HANDLE hFile = CreateFileW(
        widePath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        Logger::Error("Cannot create file: " + filePath + " (Error: " + std::to_string(error) + ")");

        if (error == ERROR_ACCESS_DENIED) {
            Logger::Error("Access denied - check permissions on network share");
        }
        else if (error == ERROR_PATH_NOT_FOUND) {
            Logger::Error("Path not found - check directory exists");
        }
        else if (error == ERROR_SHARING_VIOLATION) {
            Logger::Error("File is locked by another process");
        }
        else if (error == ERROR_NETWORK_ACCESS_DENIED) {
            Logger::Error("Network access denied");
        }
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL result = WriteFile(hFile, data, static_cast<DWORD>(size), &bytesWritten, NULL);

    CloseHandle(hFile);

    if (!result || bytesWritten != size) {
        Logger::Error("Failed to write all data to file: " + filePath);
        return false;
    }

    Logger::Debug("Successfully wrote " + std::to_string(bytesWritten) + " bytes to: " + filePath);
    return true;
}
