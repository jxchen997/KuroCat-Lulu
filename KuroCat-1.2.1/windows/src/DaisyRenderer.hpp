#pragma once

#include <objidl.h>
#include <gdiplus.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include "AppResources.h"
#include "CatPhysics.hpp"
#include "EmbeddedRaster.hpp"

class DaisyRenderer {
public:
    static bool draw(Gdiplus::Graphics& graphics, const CatPhysics& physics) {
        const auto& art = textures();
        if (art.front.pixels.empty()) return false;

        Pose pose = poseFor(physics, !art.seated.pixels.empty() && !art.forepaw.pixels.empty());
        static std::array<Pixel, FrameWidth * FrameHeight> frame;
        for (int y = 0; y < FrameHeight; ++y) {
            for (int x = 0; x < FrameWidth; ++x) {
                Pt point = {
                    FrameLeft + (x + 0.5f) * FrameArtWidth / FrameWidth,
                    FrameTop + (y + 0.5f) * FrameArtHeight / FrameHeight
                };
                point.y = standingY(point.y, pose.sitting);
                frame[y * FrameWidth + x] = renderPixel(art, sourcePoint(point, pose), pose);
            }
        }

        using namespace Gdiplus;
        Bitmap sprite(FrameWidth, FrameHeight, FrameWidth * sizeof(Pixel), PixelFormat32bppPARGB,
                      reinterpret_cast<BYTE*>(frame.data()));
        RectF dest(ArtLeft + pose.bodyX + FrameLeft * ArtScale,
                   FeetY + (FrameTop - ArtFeet) * ArtScale * pose.bodyScale,
                   FrameArtWidth * ArtScale, FrameArtHeight * ArtScale * pose.bodyScale);
        GraphicsState state = graphics.Save();
        graphics.SetInterpolationMode(InterpolationModeHighQualityBilinear);
        graphics.DrawImage(&sprite, dest, 0, 0, FrameWidth, FrameHeight, UnitPixel);
        if (pose.sitting > 0.63f) drawTypingPaws(graphics, art.forepaw, physics, pose);
        graphics.Restore(state);
        return true;
    }

    static CatHitZone hitTest(Pt point, const CatPhysics& physics) {
        const auto& art = textures();
        if (art.front.pixels.empty()) return CatHitZone::None;
        Pose pose = poseFor(physics, !art.seated.pixels.empty() && !art.forepaw.pixels.empty());
        Pt spritePoint = {(point.x - ArtLeft - pose.bodyX) / ArtScale,
                          ArtFeet + (point.y - FeetY) / (ArtScale * pose.bodyScale)};
        spritePoint.y = standingY(spritePoint.y, pose.sitting);
        Pt source = sourcePoint(spritePoint, pose);
        if ((renderPixel(art, source, pose) >> 24) < 45) return CatHitZone::None;
        return source.y < 585.0f ? CatHitZone::Head : CatHitZone::Body;
    }

private:
    using Pixel = std::uint32_t;

    // Measurements are in the coordinate space of the original design image.
    static constexpr float ArtWidth = 1024.0f;
    static constexpr float ArtHeight = 1536.0f;
    static constexpr float ArtFeet = 1470.0f;
    static constexpr float ArtScale = 0.147f;
    static constexpr float ArtLeft = 101.0f;
    static constexpr float FeetY = 237.0f;
    static constexpr float SeatCollarY = 624.0f;
    static constexpr float SeatedHeadDrop = 380.0f;
    static constexpr float FrameLeft = -100.0f;
    static constexpr float FrameTop = -95.0f;
    static constexpr float FrameArtWidth = 1224.0f;
    static constexpr float FrameArtHeight = 1700.0f;
    static constexpr int FrameWidth = 246;
    static constexpr int FrameHeight = 342;

    struct Pose {
        float bodyX;
        float bodyScale;
        float sitting;
        float headX;
        float headY;
        float headCos;
        float headSin;
        float headScaleX;
        float faceTurn;
        Pt gaze;
        float leftTap;
        float rightTap;
        float footTap;
        float tailY;
        float eyesClosed;
        float smileClosed;
    };

    static float smooth(float low, float high, float value) {
        float t = std::clamp((value - low) / (high - low), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    static float influence(Pt point, Pt center, float radiusX, float radiusY) {
        float x = (point.x - center.x) / radiusX;
        float y = (point.y - center.y) / radiusY;
        return 1.0f - smooth(0.37f, 1.0f, x * x + y * y);
    }

    static float pawMotion(const Spring2D& spring, Pt rest) {
        Pt p = spring.point();
        float reach = std::abs(p.x - rest.x) + std::abs(p.y - rest.y);
        return std::clamp((reach - 2.0f) * 0.022f + std::abs(spring.y.velocity) * 0.0014f, 0.0f, 1.0f);
    }

    static Pose poseFor(const CatPhysics& physics, bool hasSeatedArt) {
        float yaw = std::clamp(physics.headYaw.value, -1.0f, 1.0f);
        float pitch = std::clamp(physics.headPitch.value, -1.0f, 1.0f);
        float tilt = std::clamp(physics.headTilt.value, -0.22f, 0.22f);
        float contentment = std::max(physics.petHeadIntensity, physics.petBodyIntensity * 0.75f);
        float eyes = smooth(0.23f, 0.68f, physics.effectiveEyeClosure());
        return {
            physics.bodyShiftX.value * 0.55f,
            1.0f + (physics.bodySquash.value - 1.0f) * 0.33f + std::sin(physics.time * 2.1f) * 0.009f,
            hasSeatedArt ? std::clamp(physics.typingSeat.value, 0.0f, 1.0f) : 0.0f,
            yaw * 46.0f,
            pitch * 38.0f + physics.headOffsetY.value * 1.65f,
            std::cos(tilt),
            std::sin(tilt),
            1.0f - std::abs(yaw) * 0.075f,
            std::clamp(physics.daisyTurn.value, -1.0f, 1.0f),
            physics.eyeGaze.point(),
            pawMotion(physics.leftPaw, CatPhysics::LeftPawRest),
            pawMotion(physics.rightPaw, CatPhysics::RightPawRest),
            std::clamp(physics.hindFootActive.value, 0.0f, 1.0f),
            std::sin(physics.tailPhase) * (23.0f + physics.huntIntensity.value * 18.0f),
            eyes,
            std::max(smooth(0.24f, 0.70f, contentment), smooth(0.42f, 0.82f, physics.sleepiness))
        };
    }

    static float standingY(float y, float sitting) {
        float collar = SeatCollarY + sitting * SeatedHeadDrop;
        if (y <= collar) return y - sitting * SeatedHeadDrop;
        return SeatCollarY + (y - collar) * (ArtFeet - SeatCollarY) / (ArtFeet - collar);
    }

    static Pt sourcePoint(Pt point, const Pose& pose) {
        if (point.y < 624.0f) {
            constexpr Pt pivot {529.0f, 556.0f};
            float weight = 1.0f - smooth(400.0f, 624.0f, point.y);
            float x = point.x - pivot.x - pose.headX;
            float y = point.y - pivot.y - pose.headY;
            Pt turned = {pivot.x + (x * pose.headCos + y * pose.headSin) / pose.headScaleX,
                         pivot.y + y * pose.headCos - x * pose.headSin};
            return {point.x + (turned.x - point.x) * weight, point.y + (turned.y - point.y) * weight};
        }

        float left = pose.leftTap * influence(point, {444.0f, 763.0f}, 142.0f, 124.0f);
        float right = pose.rightTap * influence(point, {668.0f, 755.0f}, 126.0f, 116.0f);
        float stand = 1.0f - smooth(0.12f, 0.75f, pose.sitting);
        left *= stand;
        right *= stand;
        float tailY = 1190.0f - pose.sitting * 110.0f;
        float tail = pose.tailY * influence(point, {166.0f, tailY}, 205.0f, 250.0f);
        float foot = pose.footTap * influence(point, {648.0f, 1432.0f}, 99.0f, 100.0f);
        return {point.x + (left - right) * 12.0f - foot * 18.0f,
                point.y - (left + right) * 42.0f - tail + foot * 46.0f};
    }

    // Blend premultiplied pixels in pairs of channels to preserve the fur's transparent edges.
    static Pixel mix(Pixel a, Pixel b, int fraction) {
        constexpr Pixel mask = 0x00ff00ff;
        Pixel t = static_cast<Pixel>(fraction);
        Pixel inverse = 256 - t;
        Pixel blueRed = ((a & mask) * inverse + (b & mask) * t) >> 8;
        Pixel greenAlpha = (((a >> 8) & mask) * inverse + ((b >> 8) & mask) * t) >> 8;
        return (blueRed & mask) | ((greenAlpha & mask) << 8);
    }

    struct Texture {
        int width = 0;
        int height = 0;
        std::vector<Pixel> pixels;

        Pixel sample(Pt point) const {
            float texX = point.x * width / ArtWidth - 0.5f;
            float texY = point.y * height / ArtHeight - 0.5f;
            if (texX < 0.0f || texY < 0.0f || texX >= width - 1 || texY >= height - 1) return 0;
            int x = static_cast<int>(texX);
            int y = static_cast<int>(texY);
            int tX = static_cast<int>((texX - x) * 256.0f);
            int tY = static_cast<int>((texY - y) * 256.0f);
            const Pixel* row = pixels.data() + y * width + x;
            return mix(mix(row[0], row[1], tX), mix(row[width], row[width + 1], tX), tY);
        }
    };

    struct Textures {
        Texture front;
        Texture blink;
        Texture smile;
        Texture closed;
        Texture seated;
        Texture forepaw;
    };

    static Texture loadTexture(int id, int expectedWidth = 512, int expectedHeight = 768) {
        EmbeddedRaster raster = EmbeddedRaster::load(id, expectedWidth, expectedHeight);
        return {raster.width, raster.height, std::move(raster.pixels)};
    }

    static const Textures& textures() {
        static const Textures art {loadTexture(ID_DAISY_FRONT), loadTexture(ID_DAISY_FRONT_BLINK),
                                   loadTexture(ID_DAISY_SMILE), loadTexture(ID_DAISY_CLOSED),
                                   loadTexture(ID_DAISY_SEATED), loadTexture(ID_DAISY_FOREPAW, 112, 384)};
        return art;
    }

    static Pixel frontPixel(const Textures& art, Pt source, const Pose& pose) {
        if (source.y < 190.0f || source.y > 390.0f || source.x < 300.0f || source.x > 730.0f) {
            return art.front.sample(source);
        }
        float eyeArea = std::max(influence(source, {417.0f, 291.0f}, 119.0f, 93.0f),
                                 influence(source, {634.0f, 291.0f}, 119.0f, 93.0f));
        float gazeArea = std::max(influence(source, {431.0f, 293.0f}, 53.0f, 44.0f),
                                  influence(source, {622.0f, 293.0f}, 53.0f, 44.0f));
        Pt gazePoint = {source.x - pose.gaze.x * 19.0f * gazeArea * (1.0f - pose.eyesClosed),
                        source.y - pose.gaze.y * 13.0f * gazeArea * (1.0f - pose.eyesClosed)};
        Pixel smiling = art.front.sample(gazePoint);
        if (pose.eyesClosed * eyeArea <= 0.005f || art.blink.pixels.empty()) return smiling;
        return mix(smiling, art.blink.sample(source), static_cast<int>(eyeArea * pose.eyesClosed * 256.0f));
    }

    static Pixel sidePixel(const Textures& art, Pt source, const Pose& pose, float direction) {
        if (source.y < 148.0f || source.y > 513.0f || source.x < 360.0f || source.x > 815.0f) {
            return art.smile.sample(source);
        }

        float eyeArea = std::max(influence(source, {492.0f, 273.0f}, 122.0f, 92.0f),
                                 influence(source, {699.0f, 239.0f}, 95.0f, 90.0f));
        float gazeArea = std::max(influence(source, {511.0f, 271.0f}, 51.0f, 41.0f),
                                  influence(source, {699.0f, 233.0f}, 42.0f, 39.0f));
        float mouthArea = influence(source, {615.0f, 389.0f}, 156.0f, 112.0f);
        Pt gazePoint = {source.x - direction * pose.gaze.x * 19.0f * gazeArea * (1.0f - pose.eyesClosed),
                        source.y - pose.gaze.y * 14.0f * gazeArea * (1.0f - pose.eyesClosed)};
        Pixel smiling = art.smile.sample(gazePoint);
        float expression = std::max(eyeArea * pose.eyesClosed, mouthArea * pose.smileClosed);
        if (expression <= 0.005f || art.closed.pixels.empty()) return smiling;
        return mix(smiling, art.closed.sample(source), static_cast<int>(expression * 256.0f));
    }

    static Pixel renderPixel(const Textures& art, Pt source, const Pose& pose) {
        if (pose.sitting > 0.0f && source.y >= SeatCollarY) {
            float blend = smooth(0.25f, 0.65f, pose.sitting) * smooth(SeatCollarY, 700.0f, source.y);
            Pixel seated = art.seated.sample(source);
            if (blend >= 1.0f) return seated;
            return mix(art.front.sample(source), seated, static_cast<int>(blend * 256.0f));
        }

        float turn = smooth(0.05f, 0.92f, std::abs(pose.faceTurn));
        if (turn <= 0.0f || source.y >= 554.0f || art.smile.pixels.empty()) {
            return frontPixel(art, source, pose);
        }

        // Align the face landmarks during the turn. The collar, body and tail use one image.
        float direction = pose.faceTurn < 0.0f ? -1.0f : 1.0f;
        if (turn >= 1.0f && source.y <= 487.0f) {
            return sidePixel(art, {560.0f + direction * (source.x - 529.0f), source.y}, pose, direction);
        }
        Pt facingRight = {529.0f + direction * (source.x - 529.0f), source.y};
        float face = influence(facingRight, {529.0f, 324.0f}, 298.0f, 250.0f);
        float leftEar = influence(facingRight, {277.0f, 195.0f}, 146.0f, 129.0f);
        float rightEar = influence(facingRight, {727.0f, 163.0f}, 155.0f, 125.0f);
        float leftEye = influence(facingRight, {448.0f, 286.0f}, 88.0f, 73.0f);
        float rightEye = influence(facingRight, {639.0f, 270.0f}, 85.0f, 76.0f);
        float nose = influence(facingRight, {556.0f, 340.0f}, 87.0f, 64.0f);
        Pt shift = {
            direction * (62.0f * face + 16.0f * leftEar - 92.0f * rightEar - 16.0f * (leftEye + rightEye) + 23.0f * nose),
            -39.0f * face + 30.0f * leftEar - 30.0f * rightEar + 14.0f * leftEye - 16.0f * rightEye
        };
        Pt front = {source.x - shift.x * turn, source.y - shift.y * turn};
        Pt side = {560.0f + direction * (source.x - 529.0f + shift.x * (1.0f - turn)),
                   source.y + shift.y * (1.0f - turn)};
        float blend = turn * (1.0f - smooth(487.0f, 554.0f, source.y));
        if (turn < 1.0f) {
            // Moving lids and teeth use one expression in the transition, so they never double up.
            float features = influence(facingRight, {545.0f, 327.0f}, 219.0f, 177.0f);
            blend += ((turn < 0.5f ? 0.0f : 1.0f) - blend) * features;
        }
        Pixel turned = sidePixel(art, side, pose, direction);
        if (blend >= 1.0f) return turned;
        return mix(frontPixel(art, front, pose), turned, static_cast<int>(blend * 256.0f));
    }

    static void drawTypingPaws(Gdiplus::Graphics& graphics, const Texture& texture, const CatPhysics& physics,
                               const Pose& pose) {
        using namespace Gdiplus;
        float collar = SeatCollarY + pose.sitting * SeatedHeadDrop;
        float shoulders = collar + (830.0f - SeatCollarY) * (ArtFeet - collar) / (ArtFeet - SeatCollarY);
        float shoulderY = FeetY + (shoulders - ArtFeet) * ArtScale * pose.bodyScale;
        float shoulderX = ArtLeft + pose.bodyX + 529.0f * ArtScale;
        Bitmap paw(texture.width, texture.height, texture.width * sizeof(Pixel), PixelFormat32bppPARGB,
                   reinterpret_cast<BYTE*>(const_cast<Pixel*>(texture.pixels.data())));
        float opacity = smooth(0.63f, 0.86f, pose.sitting);
        drawTypingPaw(graphics, paw, {shoulderX - 18.0f, shoulderY}, physics.leftStrike, opacity);
        drawTypingPaw(graphics, paw, {shoulderX + 18.0f, shoulderY}, physics.rightStrike, opacity);
    }

    static void drawTypingPaw(Gdiplus::Graphics& graphics, Gdiplus::Bitmap& paw, Pt root,
                              const PetKeystroke& stroke, float opacity) {
        using namespace Gdiplus;
        Pt tip = stroke.tip;
        float dx = tip.x - root.x;
        float dy = tip.y - root.y;
        float length = std::max(1.0f, std::hypot(dx, dy));
        float halfWidth = std::clamp(11.5f + (length - 50.0f) * 0.036f, 11.0f, 14.0f);
        Pt normal = {dy * halfWidth / length, -dx * halfWidth / length};
        PointF corners[] = {
            {root.x - normal.x, root.y - normal.y}, {root.x + normal.x, root.y + normal.y},
            {tip.x - normal.x, tip.y - normal.y}
        };
        if (stroke.onKey()) {
            SolidBrush contactShadow(Color(static_cast<BYTE>(64.0f * opacity), 116, 52, 75));
            graphics.FillEllipse(&contactShadow, tip.x - 8.2f, tip.y - 3.0f, 16.4f, 5.0f);
        }
        ImageAttributes attributes;
        ColorMatrix fade = {{
            {1, 0, 0, 0, 0}, {0, 1, 0, 0, 0}, {0, 0, 1, 0, 0},
            {0, 0, 0, opacity, 0}, {0, 0, 0, 0, 1}
        }};
        attributes.SetColorMatrix(&fade);
        graphics.DrawImage(&paw, corners, 3, 0.0f, 0.0f, static_cast<float>(paw.GetWidth()),
                           static_cast<float>(paw.GetHeight()), UnitPixel, &attributes);
    }
};
