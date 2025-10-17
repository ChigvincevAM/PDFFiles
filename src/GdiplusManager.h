#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <atomic>

#pragma comment(lib, "gdiplus.lib")

class GdiplusManager {
private:
    ULONG_PTR gdiplusToken_;
    std::atomic<bool> initialized_;

    GdiplusManager() : gdiplusToken_(0), initialized_(false) {}

public:
    static GdiplusManager& Instance();

    void EnsureInitialized();

    void Shutdown();

    ~GdiplusManager();

    GdiplusManager(const GdiplusManager&) = delete;
    GdiplusManager& operator=(const GdiplusManager&) = delete;
};
