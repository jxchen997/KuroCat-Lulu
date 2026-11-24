#include "src/CatRenderer.hpp"

static bool followsThenReturnsToFront(const CatSettings& settings) {
    InputEngine input;
    CatPhysics physics;
    auto step = [&](int offset, bool moving) {
        for (int frame = 0; frame < 72; ++frame) {
            GetCursorPos(&input.lastCursorPos);
            input.lastMouseMoveTime = nowSeconds() - (moving ? 0.0 : 2.0);
            POINT cursor = input.lastCursorPos;
            RECT rect = {cursor.x - offset, cursor.y - 64, cursor.x - offset + 360, cursor.y + 256};
            physics.step(1.0f / 60.0f, rect, input, settings);
        }
    };
    step(-140, true);
    if (physics.daisyTurn.value > -0.85f) return false;
    step(540, true);
    if (physics.daisyTurn.value < 0.85f) return false;
    step(540, false);
    if (std::abs(physics.daisyTurn.value) > 0.08f) return false;
    physics.registerPetStroke({178.0f, 58.0f}, 13.0f, true, settings);
    physics.registerPetStroke({185.0f, 58.0f}, 13.0f, true, settings);
    step(540, false);
    return physics.daisyTurn.value > 0.85f && physics.isHappyClosedEyes();
}

int wmain(int argc, wchar_t** argv) {
    using namespace Gdiplus;
    if (argc < 2 || argc > 3) return 1;

    GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &startup, nullptr) != Ok) return 2;

    int result = 0;
    {
        CatSettings settings;
        settings.coatTheme = CatCoatTheme::Daisy;
        settings.keyboardTheme = KeyboardTheme::DaisyPink;
        CatPhysics idle;
        CatPhysics tracking;
        tracking.headYaw.value = -0.82f;
        tracking.headPitch.value = -0.48f;
        tracking.headTilt.value = -0.12f;
        tracking.daisyTurn.value = -1.0f;
        tracking.eyeGaze.x.value = -0.88f;
        tracking.eyeGaze.y.value = -0.45f;
        tracking.tailPhase = 1.0f;

        CatPhysics right;
        right.daisyTurn.value = argc == 3 ? 0.50f : 1.0f;
        right.headYaw.value = 0.78f;
        right.headPitch.value = 0.22f;
        right.headTilt.value = 0.08f;
        right.eyeGaze.x.value = 0.8f;
        right.tailPhase = 4.0f;

        CatPhysics petted;
        petted.petHeadIntensity = 1.0f;
        petted.daisyTurn.value = 1.0f;
        petted.headTilt.value = 0.13f;
        petted.headOffsetY.value = -3.0f;
        petted.tailPhase = 3.9f;
        petted.sparkles.push_back({{213.0f, 29.0f}, {0.0f, 0.0f}, 0.4f, 1.0f, false});

        if (settings.coatTheme != CatCoatTheme::Daisy || !followsThenReturnsToFront(settings) ||
            CatRenderer::hitTest({185.0f, 64.0f}, idle, settings.coatTheme) != CatHitZone::Head ||
            CatRenderer::hitTest({181.0f, 174.0f}, idle, settings.coatTheme) != CatHitZone::Body ||
            CatRenderer::hitTest({95.0f, 45.0f}, idle, settings.coatTheme) != CatHitZone::None) {
            result = 3;
        }

        constexpr int scale = 2;
        constexpr int panelWidth = 248;
        constexpr int panelHeight = 327;
        Bitmap bitmap(scale * panelWidth * 4, scale * panelHeight, PixelFormat32bppPARGB);
        Graphics graphics(&bitmap);
        graphics.Clear(Color(255, 244, 235, 225));
        const CatPhysics* poses[] = {&idle, &tracking, &right, &petted};
        const wchar_t* labels[] = {L"\u5F85\u673A\u6B63\u8138", L"\u5411\u5DE6\u8F6C", L"\u5411\u53F3\u8F6C", L"\u6478\u5934\u95ED\u773C"};
        Font font(L"Microsoft YaHei UI", 13.0f, FontStyleRegular, UnitPixel);
        SolidBrush ink(Color(255, 107, 77, 61));
        SolidBrush white(Color(255, 238, 224, 213));
        SolidBrush dark(Color(255, 55, 48, 48));
        StringFormat caption;
        caption.SetAlignment(StringAlignmentCenter);
        for (int i = 0; i < 4; ++i) {
            GraphicsState panel = graphics.Save();
            graphics.ScaleTransform(scale, scale);
            graphics.TranslateTransform(i * static_cast<float>(panelWidth), 0.0f);
            if (i == 3) graphics.FillRectangle(&dark, 0, 0, panelWidth, panelHeight);
            GraphicsState cat = graphics.Save();
            graphics.TranslateTransform(-55.0f, -3.0f);
            CatRenderer::draw(graphics, *poses[i], settings);
            graphics.Restore(cat);
            graphics.DrawString(labels[i], -1, &font, PointF(panelWidth / 2.0f, 305.0f), &caption, i == 3 ? &white : &ink);
            graphics.Restore(panel);
        }

        const CLSID pngEncoder = {0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
        if (bitmap.Save(argv[1], &pngEncoder, nullptr) != Ok) result = 4;
    }
    GdiplusShutdown(token);
    return result;
}
