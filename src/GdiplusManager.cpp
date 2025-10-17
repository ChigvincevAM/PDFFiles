#include "GdiplusManager.h"

#pragma comment(lib, "gdiplus.lib")

GdiplusManager& GdiplusManager::Instance() {
    static GdiplusManager instance;
    return instance;
}

void GdiplusManager::EnsureInitialized() {
    if (!initialized_.load()) {
        bool expected = false;
        if (initialized_.compare_exchange_strong(expected, true)) {
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, nullptr);
            if (status != Gdiplus::Ok) {
                initialized_ = false;
            }
        }
    }
}

void GdiplusManager::Shutdown() {
    bool expected = true;
    if (initialized_.compare_exchange_strong(expected, false)) {
        Gdiplus::GdiplusShutdown(gdiplusToken_);
        gdiplusToken_ = 0;
    }
}

GdiplusManager::~GdiplusManager() {
    Shutdown();
}
