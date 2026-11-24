#define NOMINMAX
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace Gdiplus;
using Pixel = std::uint32_t;
static const CLSID PngEncoder = {0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};

static int ink(Pixel pixel) {
    return 255 - std::min({static_cast<int>((pixel >> 16) & 255),
                           static_cast<int>((pixel >> 8) & 255), static_cast<int>(pixel & 255)});
}

static int writeCutout(const wchar_t* source, const wchar_t* target) {
    Bitmap screenshot(source);
    const Rect crop(254, 552, 776, 1240);
    if (screenshot.GetLastStatus() != Ok || screenshot.GetWidth() != 1280 || screenshot.GetHeight() != 2772) return 2;
    const int width = crop.Width;
    const int height = crop.Height;
    BitmapData bits{};
    if (screenshot.LockBits(&crop, ImageLockModeRead, PixelFormat32bppARGB, &bits) != Ok) return 3;
    std::vector<Pixel> sourcePixels(width * height);
    for (int y = 0; y < height; ++y) {
        std::memcpy(sourcePixels.data() + y * width, static_cast<const BYTE*>(bits.Scan0) + y * bits.Stride,
                    width * sizeof(Pixel));
    }
    screenshot.UnlockBits(&bits);

    // Remove only the white around the character, leaving white highlights and eye whites intact.
    std::vector<unsigned char> backdrop(width * height, 0);
    std::vector<int> queue;
    auto enqueue = [&](int i) {
        if (backdrop[i] || ink(sourcePixels[i]) > 10) return;
        backdrop[i] = 1;
        queue.push_back(i);
    };
    for (int x = 0; x < width; ++x) {
        enqueue(x);
        enqueue((height - 1) * width + x);
    }
    for (int y = 1; y < height - 1; ++y) {
        enqueue(y * width);
        enqueue(y * width + width - 1);
    }
    for (size_t head = 0; head < queue.size(); ++head) {
        int i = queue[head];
        if (i % width) enqueue(i - 1);
        if (i % width < width - 1) enqueue(i + 1);
        if (i >= width) enqueue(i - width);
        if (i < width * (height - 1)) enqueue(i + width);
    }
    if (queue.size() < 100000 || queue.size() > 450000) return 4;

    std::vector<Pixel> result(width * height, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            if (backdrop[i]) continue;
            Pixel sourcePixel = sourcePixels[i];
            int strength = ink(sourcePixel);
            bool onContour = false;
            if (strength < 190 && x > 1 && x < width - 2 && y > 1 && y < height - 2) {
                for (int sy = -2; sy <= 2 && !onContour; ++sy) {
                    for (int sx = -2; sx <= 2; ++sx) {
                        if (backdrop[i + sy * width + sx]) { onContour = true; break; }
                    }
                }
            }
            if (!onContour) {
                result[i] = sourcePixel | 0xff000000u;
                continue;
            }

            // Undo the white matte on antialiased edge pixels so they also look clean on dark desktops.
            int local = strength;
            for (int sy = std::max(0, y - 4); sy <= std::min(height - 1, y + 4); ++sy) {
                for (int sx = std::max(0, x - 4); sx <= std::min(width - 1, x + 4); ++sx) {
                    local = std::max(local, ink(sourcePixels[sy * width + sx]));
                }
            }
            int alpha = std::clamp((strength * 255 + local / 2) / std::max(local, 1), 1, 255);
            Pixel output = static_cast<Pixel>(alpha) << 24;
            for (int shift : {0, 8, 16}) {
                int channel = static_cast<int>((sourcePixel >> shift) & 255);
                int clean = std::max(0, 255 - ((255 - channel) * 255 + alpha / 2) / alpha);
                output |= static_cast<Pixel>(clean) << shift;
            }
            result[i] = output;
        }
    }
    Bitmap cutout(width, height, PixelFormat32bppARGB);
    Rect outputBounds(0, 0, width, height);
    BitmapData output{};
    if (cutout.LockBits(&outputBounds, ImageLockModeWrite, PixelFormat32bppARGB, &output) != Ok) return 8;
    for (int y = 0; y < height; ++y) {
        std::memcpy(static_cast<BYTE*>(output.Scan0) + y * output.Stride, result.data() + y * width, width * sizeof(Pixel));
    }
    cutout.UnlockBits(&output);
    Status saved = cutout.Save(target, &PngEncoder, nullptr);
    if (saved == Ok) return 0;
    std::fwprintf(stderr, L"Saving %ls failed with GDI+ status %d\n", target, saved);
    return 5;
}

static float smooth(float v) { return v * v * (3.0f - 2.0f * v); }

static std::array<int, 3> rowColor(Bitmap& image, int midX, int y) {
    std::array<int, 3> color{};
    for (int x = midX - 15; x < midX + 16; ++x) {
        Color pixel;
        image.GetPixel(x, y, &pixel);
        color[0] += pixel.GetR();
        color[1] += pixel.GetG();
        color[2] += pixel.GetB();
    }
    for (int& channel : color) channel /= 31;
    return color;
}

static bool eyePatch(Bitmap& front, Bitmap& closed, Bitmap& overlay, RectF sample, Rect area) {
    Bitmap patch(area.Width, area.Height, PixelFormat32bppARGB);
    {
        Graphics g(&patch);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        if (g.DrawImage(&closed, Rect(0, 0, area.Width, area.Height), sample.X, sample.Y, sample.Width, sample.Height, UnitPixel) != Ok) return false;
    }

    auto lowReference = rowColor(front, area.X + area.Width / 2, area.Y + 9);
    auto highReference = rowColor(front, area.X + area.Width / 2, area.Y + area.Height - 10);
    auto lowPatch = rowColor(patch, area.Width / 2, 9);
    auto highPatch = rowColor(patch, area.Width / 2, area.Height - 10);
    for (int y = 0; y < area.Height; ++y) {
        float along = static_cast<float>(y) / (area.Height - 1);
        for (int x = 0; x < area.Width; ++x) {
            float across = static_cast<float>(x) / (area.Width - 1);
            float distance = std::max(std::abs(across * 2.0f - 1.0f), std::abs(along * 2.0f - 1.0f));
            float feather = 1.0f - smooth(std::clamp((distance - 0.72f) / 0.28f, 0.0f, 1.0f));
            Color value;
            patch.GetPixel(x, y, &value);
            Color base;
            front.GetPixel(x + area.X, y + area.Y, &base);
            auto correct = [&](int c, BYTE channel) {
                float a = static_cast<float>(lowReference[c] - lowPatch[c]);
                float b = static_cast<float>(highReference[c] - highPatch[c]);
                return static_cast<BYTE>(std::clamp(static_cast<float>(channel) + a + (b - a) * along, 0.0f, 255.0f));
            };
            BYTE opacity = static_cast<BYTE>(std::min(value.GetA(), base.GetA()) * feather);
            if (opacity > 0) overlay.SetPixel(x + area.X, y + area.Y, Color(opacity, correct(0, value.GetR()), correct(1, value.GetG()), correct(2, value.GetB())));
        }
    }
    return true;
}

static bool writeClosedEyes(const wchar_t* idle, const wchar_t* edited, const wchar_t* destination) {
    Bitmap front(idle), closed(edited);
    if (front.GetWidth() != 776 || front.GetHeight() != 1240 || closed.GetWidth() != 1448 || closed.GetHeight() != 1086) return false;
    Bitmap overlay(776, 1240, PixelFormat32bppARGB);
    if (!eyePatch(front, closed, overlay, {362, 317, 289, 184}, {151, 211, 155, 133})) return false;
    if (!eyePatch(front, closed, overlay, {794, 320, 289, 184}, {459, 197, 157, 135})) return false;
    return overlay.Save(destination, &PngEncoder, nullptr) == Ok;
}

static bool writeSeat(const wchar_t* asset, const wchar_t* destination) {
    Bitmap edited(asset);
    if (edited.GetWidth() != 1082 || edited.GetHeight() != 1454) return false;
    Bitmap cropped(776, 520, PixelFormat32bppARGB);
    {
        Graphics g(&cropped);
        g.SetCompositingMode(CompositingModeSourceCopy);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        if (g.DrawImage(&edited, Rect(0, 0, 776, 520), 149.0f, 904.0f, 776.0f, 472.0f, UnitPixel) != Ok) return false;
    }
    Bitmap joined(776, 520, PixelFormat32bppARGB);
    {
        Graphics g(&joined);
        g.SetCompositingMode(CompositingModeSourceCopy);
        g.SetInterpolationMode(InterpolationModeHighQualityBilinear);
        g.DrawImage(&cropped, Rect(0, 0, 776, 520), 0, 0, 776, 520, UnitPixel);
        // Match the standing artwork's neck width where the two original-pose layers meet.
        for (int y = 0; y < 72; ++y) {
            float neck = 1.0f - smooth(y / 72.0f);
            RectF row(-17.0f * neck, static_cast<float>(y), 776.0f + 62.0f * neck, 1.0f);
            g.DrawImage(&cropped, row, 0.0f, static_cast<float>(y), 776.0f, 1.0f, UnitPixel);
        }
    }
    for (int y = 0; y < 72; ++y) {
        float neck = 1.0f - smooth(y / 72.0f);
        for (int x = 0; x < 776; ++x) {
            Color pixel;
            joined.GetPixel(x, y, &pixel);
            if (!pixel.GetA()) continue;
            auto channel = [neck](BYTE value, float delta) {
                return static_cast<BYTE>(std::clamp(static_cast<float>(value) + neck * delta, 0.0f, 255.0f));
            };
            joined.SetPixel(x, y, Color(pixel.GetA(), channel(pixel.GetR(), -5.0f), channel(pixel.GetG(), 3.0f), channel(pixel.GetB(), 35.0f)));
        }
    }
    return joined.Save(destination, &PngEncoder, nullptr) == Ok;
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3 && argc != 5 && argc != 4) return 1;
    GdiplusStartupInput settings;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &settings, nullptr) != Ok) return 6;
    int result = 0;
    if (argc == 3) result = writeCutout(argv[1], argv[2]);
    else if (argc == 4) result = writeSeat(argv[2], argv[3]) ? 0 : 9;
    else result = writeClosedEyes(argv[2], argv[3], argv[4]) ? 0 : 10;
    GdiplusShutdown(token);
    return result;
}
