#pragma once

#include <objidl.h>
#include <gdiplus.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

struct EmbeddedRaster {
    using Pixel = std::uint32_t;
    int width = 0;
    int height = 0;
    std::vector<Pixel> pixels;

    static EmbeddedRaster load(int id, int width, int height) {
        using namespace Gdiplus;
        HMODULE module = GetModuleHandleW(nullptr);
        HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(10));
        if (!resource) return {};
        DWORD size = SizeofResource(module, resource);
        auto data = LockResource(LoadResource(module, resource));
        if (!data || size == 0) return {};

        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!memory) return {};
        auto buffer = GlobalLock(memory);
        if (!buffer) {
            GlobalFree(memory);
            return {};
        }
        std::memcpy(buffer, data, size);
        GlobalUnlock(memory);

        IStream* raw = nullptr;
        if (FAILED(CreateStreamOnHGlobal(memory, TRUE, &raw))) {
            GlobalFree(memory);
            return {};
        }
        auto release = [](IStream* stream) { stream->Release(); };
        std::unique_ptr<IStream, decltype(release)> stream(raw, release);
        Bitmap bitmap(stream.get());
        if (bitmap.GetWidth() != static_cast<UINT>(width) || bitmap.GetHeight() != static_cast<UINT>(height)) return {};

        Rect rect(0, 0, width, height);
        BitmapData view{};
        EmbeddedRaster raster{width, height, std::vector<Pixel>(width * height)};
        if (bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppPARGB, &view) != Ok) return {};
        for (int y = 0; y < height; ++y) {
            const auto* row = static_cast<const BYTE*>(view.Scan0) + y * view.Stride;
            std::memcpy(raster.pixels.data() + y * width, row, width * sizeof(Pixel));
        }
        bitmap.UnlockBits(&view);
        return raster;
    }
};
