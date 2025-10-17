#ifndef __FILESYSTEMUTILS_H__
#define __FILESYSTEMUTILS_H__

#include <string>
#include <vector>

class FileSystemUtils {
public:
    static bool DirectoryExists(const std::string& path);
    static bool GetFilesFromDirectory(const std::string& folderPath, std::vector<std::string>& files);
    static size_t GetFileSize(const std::string& filePath);
    static std::string GetFileExtension(const std::string& filePath);
    static std::string GetFileName(const std::string& filePath);
    static std::string GetFileNameWithoutExtension(const std::string& filePath);
    static std::string GetFileDirectory(const std::string& filePath);
    static std::string GeneratePartFileName(const std::string& basePath, int partNumber);
    static bool IsSupportedExtension(const std::string& extension);
    static std::vector<std::string> FilterFilesByExtension(const std::vector<std::string>& files);
    static void SortFilesByName(std::vector<std::string>& files);
    static bool ReadFileToBuffer(const std::string& filePath, std::vector<char>& buffer);
    static bool WriteBufferToFile(const std::string& filePath, const char* data, size_t size);
    static bool DelFile(const std::string& filePath);
    static bool FileExists(const std::string& filePath);
};

#endif // __FILESYSTEMUTILS_H__
