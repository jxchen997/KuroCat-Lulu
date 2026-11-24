#include <filesystem>
#include <cwchar>
#include "src/CatRenderer.hpp"

int wmain(int argc, wchar_t** argv) {
    using namespace Gdiplus;
    if (argc < 2 || argc > 4) return 1;

    GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &startup, nullptr) != Ok) return 2;
    int status = 0;
    {
        std::filesystem::path frames = argv[1];
        CatSettings settings;
        bool lulu = argc >= 3;
        bool isolateCharacter = argc == 4;
        if (lulu && !LuluReference::available()) status = 5;
        if (lulu) settings.showKeyBadges = false;
        if (!lulu) {
            settings.coatTheme = CatCoatTheme::Daisy;
            settings.keyboardTheme = KeyboardTheme::DaisyPink;
        }
        CatPhysics physics;
        InputEngine input;
        const CLSID pngEncoder = {0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
        bool leftContact = false;
        bool rightContact = false;
        bool lookedLeft = false;
        bool lookedRight = false;
        bool fullyClosed = false;
        bool keyContactWhileTurned = false;
        bool renderedLeftContact = false;
        bool renderedRightContact = false;
        int shot = 0;
        bool chart = argc == 3 && std::wcscmp(argv[2], L"chart") == 0;
        if (chart) {
            const float angles[] = {-1.5707963f, 0.0f, 3.14159265f};
            const wchar_t* names[] = {L"side.png", L"front.png", L"back.png"};
            for (int i = 0; i < 3; ++i) {
                Bitmap bitmap(322, 443, PixelFormat32bppPARGB);
                Graphics graphics(&bitmap);
                graphics.Clear(Color(0, 0, 0, 0));
                graphics.SetSmoothingMode(SmoothingModeAntiAlias);
                graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
                graphics.ScaleTransform(2.0f, 2.0f);
                graphics.TranslateTransform(-85.0f, -33.0f);
                LuluReference::drawStanding(graphics, angles[i]);
                if (bitmap.Save((frames / names[i]).c_str(), &pngEncoder, nullptr) != Ok) status = 3;
            }
        }
        for (int tick = 0; !chart && tick < (lulu ? 348 : 186); ++tick) {
            switch (tick) {
                case 31: physics.triggerKeyDown(PhysicalKey::J, settings); break;
                case 50: physics.triggerKeyDown(PhysicalKey::F, settings); break;
                case 61: physics.triggerKeyDown(PhysicalKey::H, settings); break;
                case 70: physics.triggerKeyDown(PhysicalKey::W, settings); break;
                case 80: physics.triggerKeyDown(PhysicalKey::L, settings); break;
                case 182: if (lulu) physics.triggerKeyDown(PhysicalKey::F, settings); break;
            }
            if (lulu && tick >= 235 && tick < 274 && tick % 5 < 2) {
                physics.registerPetStroke({169.0f + 14.0f * std::sin((tick - 235) * 0.14f), 97.0f},
                                          11.0f, true, settings);
            }
            GetCursorPos(&input.lastCursorPos);
            input.lastMouseMoveTime = nowSeconds() - 4.0;
            RECT bounds = {0, 0, 360, 320};
            if (lulu) {
                long dx = tick >= 111 && tick < 155 ? -400 : tick >= 155 && tick < 208 ? 400 : 0;
                long dy = dx != 0 ? -125 : 0;
                bounds.left = input.lastCursorPos.x - 164 - dx;
                bounds.top = input.lastCursorPos.y - 94 - dy;
                bounds.right = bounds.left + 360;
                bounds.bottom = bounds.top + 320;
            }
            physics.step(1.0f / 60.0f, bounds, input, settings);
            if (lulu) {
                LuluReference::SeatedPose pose(physics);
                lookedLeft |= physics.eyeGaze.x.value < -0.63f && physics.eyeGaze.y.value < -0.25f
                    && pose.shoulders < -0.61f && pose.hips < -0.49f;
                lookedRight |= physics.eyeGaze.x.value > 0.63f && physics.eyeGaze.y.value < -0.25f
                    && pose.shoulders > 0.61f && pose.hips > 0.49f;
                if (std::abs(pose.head - pose.shoulders) > 0.301f) status = 9;
                fullyClosed |= physics.luluContent.value > 0.82f && physics.luluEyeClosure() > 0.96f;
                if (tick == 335 && (physics.luluContent.value > 0.13f || physics.luluEyeClosure() > 0.15f)) status = 6;
                if (LuluReference::hitTest({164.0f, 106.0f}, physics) != CatHitZone::Head) status = 7;
            }

            auto touching = [&](const PetKeystroke& strike) {
                if (strike.key == PhysicalKey::None) return false;
                if (lulu ? std::abs(physics.typingSeat.value) > 0.01f : physics.typingSeat.value < 0.76f) return false;
                auto glow = physics.keyGlow.find(strike.key);
                Pt deck = KeyboardLayout::canvasPointForKey(strike.key);
                if (lulu) deck = KeyboardLayout::onLuluDesk(deck);
                return std::hypot(strike.keyCenter.x - deck.x, strike.keyCenter.y - deck.y) < 0.01f
                    && std::abs(strike.tip.x - strike.keyCenter.x) < 0.01f
                    && std::abs(strike.tip.y - strike.keyCenter.y - PetKeystroke::ContactOffset) < 0.01f
                    && strike.keyTravel(strike.key) > 0.99f
                    && glow != physics.keyGlow.end() && glow->second > 0.88f;
            };
            leftContact = leftContact || touching(physics.leftStrike);
            rightContact = rightContact || touching(physics.rightStrike);
            keyContactWhileTurned |= physics.luluSwivel.value > 0.55f && touching(physics.rightStrike);

            if (tick % 3 != 0) continue;
            Bitmap bitmap(462, 518, PixelFormat32bppPARGB);
            Graphics graphics(&bitmap);
            graphics.Clear(isolateCharacter ? Color(0, 0, 0, 0) :
                           lulu ? Color(255, 246, 240, 230) : Color(255, 250, 239, 235));
            graphics.ScaleTransform(1.58f, 1.58f);
            graphics.TranslateTransform(-30.0f, 3.0f);
            if (isolateCharacter) {
                graphics.SetSmoothingMode(SmoothingModeAntiAlias);
                graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
                LuluReference::drawBody(graphics, physics);
                LuluReference::drawHands(graphics, physics);
            } else {
                CatRenderer::draw(graphics, physics, settings);
            }
            if (lulu) {
                const auto& hands = LuluReference::renderedPose().hands;
                auto paintedContact = [&](const PetKeystroke& strike) {
                    if (!touching(strike)) return false;
                    for (float y : {-1.2f, -0.6f, 0.0f}) {
                        for (float x : {-0.6f, 0.0f, 0.6f}) {
                            if (hands.at(strike.tip.x + x, strike.tip.y + y) >> 24) return true;
                        }
                    }
                    return false;
                };
                renderedLeftContact |= paintedContact(physics.leftStrike);
                renderedRightContact |= paintedContact(physics.rightStrike);
            }
            wchar_t filename[24];
            swprintf(filename, sizeof(filename) / sizeof(filename[0]), L"frame-%03d.png", shot++);
            if (bitmap.Save((frames / filename).c_str(), &pngEncoder, nullptr) != Ok) {
                status = 3;
                break;
            }
        }
        if (!chart && (!leftContact || !rightContact || (lulu && !(lookedLeft && lookedRight && fullyClosed && keyContactWhileTurned))
            || std::abs(physics.typingSeat.value) > 0.05f)) status = 4;
        if (lulu && !chart && !(renderedLeftContact && renderedRightContact)) status = 10;
        if (lulu) {
            LuluOffice::ChairView right(0.70f);
            LuluOffice::ChairView left(-0.70f);
            auto pivot = right.project(0.0f, 0.0f, 0.0f);
            auto rightFront = right.project(0.0f, -44.0f, -36.0f);
            auto rightBack = right.project(0.0f, -44.0f, -56.0f);
            auto leftFront = left.project(0.0f, -44.0f, -36.0f);
            auto leftBack = left.project(0.0f, -44.0f, -56.0f);
            if (pivot.X != 165.0f || pivot.Y != 240.0f || rightFront.X - rightBack.X < 8.0f
                || leftBack.X - leftFront.X < 8.0f) status = 8;
        }
    }
    GdiplusShutdown(token);
    return status;
}
