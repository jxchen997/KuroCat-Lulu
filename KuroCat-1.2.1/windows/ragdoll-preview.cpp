#include "src/CatRenderer.hpp"

int wmain(int argc, wchar_t** argv) {
    using namespace Gdiplus;
    if (argc != 2) return 1;

    GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &startup, nullptr) != Ok) return 2;

    Status result;
    {
        constexpr int scale = 2;
        constexpr int panelWidth = 248;
        constexpr int panelHeight = 291;
        Bitmap bitmap(panelWidth * 3 * scale, panelHeight * scale, PixelFormat32bppPARGB);
        Graphics graphics(&bitmap);
        graphics.Clear(Color(255, 248, 246, 244));

        CatPhysics idle;
        CatPhysics hunting;
        hunting.huntIntensity.value = 0.87f;
        hunting.headYaw.value = 0.70f;
        hunting.headPitch.value = 0.27f;
        hunting.headTilt.value = -0.10f;
        hunting.headOffsetY.value = 10.0f;
        hunting.bodySquash.value = 0.85f;
        hunting.eyeGaze.x.value = 0.68f;
        hunting.eyeGaze.y.value = 0.29f;
        hunting.leftPawOnMouse = true;
        hunting.leftPaw.x.value = 88.0f;
        hunting.leftPaw.y.value = 248.0f;
        hunting.rightPawTyping = true;
        hunting.rightPaw.x.value = 200.0f;
        hunting.rightPaw.y.value = 244.0f;

        CatPhysics petted;
        petted.headTilt.value = 0.11f;
        petted.earFold.value = 0.55f;
        petted.petHeadIntensity = 0.95f;

        const CatPhysics* poses[] = { &idle, &hunting, &petted };
        const wchar_t* captions[] = { L"\u5F85\u673A", L"\u6355\u730E", L"\u6478\u5934" };
        Font font(L"Microsoft YaHei UI", 13.0f, FontStyleRegular, UnitPixel);
        SolidBrush ink(Color(255, 89, 85, 104));
        StringFormat caption;
        caption.SetAlignment(StringAlignmentCenter);
        CatSettings settings;
        for (int pane = 0; pane < 3; ++pane) {
            GraphicsState state = graphics.Save();
            graphics.ScaleTransform(scale, scale);
            graphics.TranslateTransform(panelWidth * static_cast<float>(pane), 0.0f);
            GraphicsState catState = graphics.Save();
            graphics.TranslateTransform(-56.0f, -46.0f);
            CatRenderer::draw(graphics, *poses[pane], settings);
            graphics.Restore(catState);
            graphics.DrawString(captions[pane], -1, &font, PointF(panelWidth / 2.0f, 266.0f), &caption, &ink);
            graphics.Restore(state);
        }

        const CLSID pngEncoder = {0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
        result = bitmap.Save(argv[1], &pngEncoder, nullptr);
    }
    GdiplusShutdown(token);
    return result == Ok ? 0 : 3;
}
