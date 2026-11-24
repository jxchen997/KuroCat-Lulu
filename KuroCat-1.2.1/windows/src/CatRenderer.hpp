#pragma once
#define NOMINMAX
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include "CatPhysics.hpp"
#include "DaisyRenderer.hpp"
#include "LuluDevices.hpp"
#include "LuluOffice.hpp"

class CatRenderer {
public:
    static constexpr float RadToDeg = 57.2957795f;

    static CatHitZone hitTest(Pt p, const CatPhysics& physics, CatCoatTheme coat) {
        if (coat == CatCoatTheme::Lulu) {
            CatHitZone zone = LuluArt::hitTest(p, physics);
            if (zone != CatHitZone::None) return zone;
            if (p.x >= 89.0f && p.x <= 273.0f && p.y >= 168.0f && p.y <= 224.0f) return CatHitZone::Body;
            return CatHitZone::None;
        }
        if (coat == CatCoatTheme::Daisy) {
            CatHitZone zone = DaisyRenderer::hitTest(p, physics);
            if (zone != CatHitZone::None) return zone;
            return deviceHitTest(p);
        }

        float cx = CatPhysics::SeatAnchor.x + physics.bodyShiftX.value;
        float breath = std::sin(physics.time * 2.1f) * 1.6f;
        Pt headCenter = { cx + physics.bodyTilt.value * 36.0f + physics.headYaw.value * 7.5f,
                          126.0f + physics.headOffsetY.value + physics.headPitch.value * 4.8f + breath * 1.1f };

        float headRadius = coat == CatCoatTheme::Ragdoll ? 55.0f : 48.0f;
        float hdx = (p.x - headCenter.x) / headRadius;
        float hdy = (p.y - headCenter.y) / 42.0f;
        if (hdx * hdx + hdy * hdy <= 1.0f) return CatHitZone::Head;

        float bdx = (p.x - cx) / 58.0f;
        float bdy = (p.y - 196.0f) / 56.0f;
        if (bdx * bdx + bdy * bdy <= 1.0f) return CatHitZone::Body;

        return deviceHitTest(p);
    }

    static void draw(Gdiplus::Graphics& g, const CatPhysics& physics, const CatSettings& settings) {
        using namespace Gdiplus;
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

        bool lulu = settings.coatTheme == CatCoatTheme::Lulu;
        bool sourceArt = lulu && LuluReference::available();
        if (lulu) {
            LuluOffice::drawSupport(g);
            LuluOffice::chair(g, physics.luluSwivel.value);
            if (sourceArt) LuluReference::drawBody(g, physics);
            LuluOffice::drawDesktop(g);
        }

        if (settings.keyboardTheme == KeyboardTheme::LuluMacaron) {
            LuluDevices::drawMouse(g, physics, lulu);
            LuluDevices::drawKeyboard(g, physics, lulu);
        } else {
            drawMouse(g, physics, settings.keyboardTheme, lulu);
            drawKeyboard(g, physics, settings.keyboardTheme, settings.eyeTheme, lulu);
        }

        if (sourceArt) {
            LuluReference::drawHands(g, physics);
            LuluArt::drawFlowers(g, physics);
        } else if (lulu) {
            LuluArt::draw(g, physics);
        } else if (settings.coatTheme != CatCoatTheme::Daisy || !DaisyRenderer::draw(g, physics)) {
            CatCoatTheme coat = settings.coatTheme == CatCoatTheme::Daisy ? CatCoatTheme::Ragdoll : settings.coatTheme;
            drawVectorCoat(g, physics, settings.eyeTheme, coat);
        }

        if (settings.showKeyBadges) {
            drawFloatingKeycaps(g, physics, settings.keyboardTheme);
        }
        drawSparkles(g, physics, settings.eyeTheme, settings.coatTheme == CatCoatTheme::Lulu);
    }

private:
    static CatHitZone deviceHitTest(Pt p) {
        return p.x >= 58.0f && p.x <= 294.0f && p.y >= 232.0f && p.y <= 296.0f
            ? CatHitZone::Body : CatHitZone::None;
    }

    static void drawVectorCoat(Gdiplus::Graphics& g, const CatPhysics& physics,
                               EyeColorTheme eyeTheme, CatCoatTheme coat) {
        float breath = std::sin(physics.time * 2.1f) * 1.6f;
        float seatX = CatPhysics::SeatAnchor.x + physics.bodyShiftX.value;
        float seatY = CatPhysics::SeatAnchor.y;
        Pt headCenter = {
            seatX + physics.bodyTilt.value * 36.0f + physics.headYaw.value * 7.5f,
            126.0f + physics.headOffsetY.value + physics.headPitch.value * 4.8f + breath * 1.1f
        };
        drawTail(g, seatX, seatY, physics, coat);
        drawEarsLayer(g, headCenter, physics, coat);
        drawUnifiedSilhouette(g, seatX, seatY, headCenter, breath, physics, coat);
        if (physics.hindFootActive.value > 0.03f) {
            drawHindFoot(g, seatX, seatY, physics, coat);
        }
        drawHeadFeatures(g, headCenter, physics, eyeTheme, coat);
        drawFrontPaws(g, seatX, seatY, breath, physics, coat);
    }

    static Gdiplus::Color catBlack(BYTE a = 255)      { return Gdiplus::Color(a, 22, 22, 24); }
    static Gdiplus::Color contourMint(BYTE a = 194)   { return Gdiplus::Color(a, 209, 235, 227); }
    static Gdiplus::Color whiskerWhite(BYTE a = 174)  { return Gdiplus::Color(a, 224, 230, 235); }
    static Gdiplus::Color siameseCream(BYTE a = 255)  { return Gdiplus::Color(a, 241, 231, 213); }
    static Gdiplus::Color siameseLatte(BYTE a = 255)  { return Gdiplus::Color(a, 221, 203, 177); }
    static Gdiplus::Color siameseMocha(BYTE a = 255)  { return Gdiplus::Color(a, 115, 87, 69); }
    static Gdiplus::Color siamesePoint(BYTE a = 255)  { return Gdiplus::Color(a, 45, 33, 27); }
    static Gdiplus::Color siameseOutline(BYTE a = 224){ return Gdiplus::Color(a, 66, 48, 40); }
    static Gdiplus::Color siameseWhisker(BYTE a = 174){ return Gdiplus::Color(a, 92, 69, 56); }
    static Gdiplus::Color ragdollIvory(BYTE a = 255) { return Gdiplus::Color(a, 255, 247, 231); }
    static Gdiplus::Color ragdollShade(BYTE a = 255) { return Gdiplus::Color(a, 211, 192, 172); }
    static Gdiplus::Color ragdollPoint(BYTE a = 255) { return Gdiplus::Color(a, 104, 81, 70); }
    static Gdiplus::Color ragdollOutline(BYTE a = 208) { return Gdiplus::Color(a, 104, 82, 73); }

    static Gdiplus::Color eyePrimary(EyeColorTheme t, BYTE a = 255) {
        switch (t) {
            case EyeColorTheme::NeonChartreuse: return Gdiplus::Color(a, 201, 242, 64);
            case EyeColorTheme::SiameseBlue:    return Gdiplus::Color(a, 94, 199, 250);
            case EyeColorTheme::WarmAmber:      return Gdiplus::Color(a, 250, 194, 56);
            case EyeColorTheme::IceCyan:        return Gdiplus::Color(a, 115, 235, 245);
        }
        return Gdiplus::Color(a, 201, 242, 64);
    }

    static Gdiplus::Color blendColor(Gdiplus::Color a, Gdiplus::Color b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        auto lerp = [t](BYTE x, BYTE y) -> BYTE { return (BYTE)std::clamp((int)(x + (y - x) * t + 0.5f), 0, 255); };
        return Gdiplus::Color(lerp(a.GetA(), b.GetA()), lerp(a.GetR(), b.GetR()), lerp(a.GetG(), b.GetG()), lerp(a.GetB(), b.GetB()));
    }

    static void addRoundedRect(Gdiplus::GraphicsPath& p, float x, float y, float w, float h, float r) {
        float d = std::min(r * 2.0f, std::min(w, h));
        if (d <= 0.5f) {
            p.AddRectangle(Gdiplus::RectF(x, y, w, h));
            return;
        }
        p.AddArc(x, y, d, d, 180.0f, 90.0f);
        p.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
        p.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
        p.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
        p.CloseFigure();
    }

    static void addQuadPoly(Gdiplus::GraphicsPath& p, Pt tl, Pt tr, Pt br, Pt bl) {
        Gdiplus::PointF pts[4] = { {tl.x, tl.y}, {tr.x, tr.y}, {br.x, br.y}, {bl.x, bl.y} };
        p.AddPolygon(pts, 4);
    }

    static void addQuadBezier(Gdiplus::GraphicsPath& p, Pt p0, Pt ctrl, Pt p2) {
        // Convert quadratic Bezier (p0, ctrl, p2) -> exact cubic Bezier for GDI+
        Gdiplus::PointF c1(p0.x + (2.0f / 3.0f) * (ctrl.x - p0.x), p0.y + (2.0f / 3.0f) * (ctrl.y - p0.y));
        Gdiplus::PointF c2(p2.x + (2.0f / 3.0f) * (ctrl.x - p2.x), p2.y + (2.0f / 3.0f) * (ctrl.y - p2.y));
        p.AddBezier(Gdiplus::PointF(p0.x, p0.y), c1, c2, Gdiplus::PointF(p2.x, p2.y));
    }

    static Pt projectHead3D(float x, float y, float z, float yawRad, float pitchRad) {
        float cy = std::cos(yawRad), sy = std::sin(yawRad);
        float x1 = x * cy + z * sy;
        float z1 = -x * sy + z * cy;
        float cp = std::cos(pitchRad), sp = std::sin(pitchRad);
        float y2 = y * cp + z1 * sp;
        return { x1, y2 };
    }

    static Pt quadPoint(Pt p0, Pt p1, Pt p2, float t) {
        float mt = 1.0f - t;
        return { mt * mt * p0.x + 2.0f * mt * t * p1.x + t * t * p2.x,
                 mt * mt * p0.y + 2.0f * mt * t * p1.y + t * t * p2.y };
    }

    static Pt quadTangent(Pt p0, Pt p1, Pt p2, float t) {
        float mt = 1.0f - t;
        return { 2.0f * mt * (p1.x - p0.x) + 2.0f * t * (p2.x - p1.x),
                 2.0f * mt * (p1.y - p0.y) + 2.0f * t * (p2.y - p1.y) };
    }

    // MARK: - Mini White Mouse (180° facing the cat)

    static void drawMouse(Gdiplus::Graphics& g, const CatPhysics& physics, KeyboardTheme theme, bool onDesk) {
        using namespace Gdiplus;
        Pt center = { CatPhysics::MouseBaseCenter.x + physics.mouseOffset.point().x,
                      CatPhysics::MouseBaseCenter.y + physics.mouseOffset.point().y };
        if (onDesk) center = KeyboardLayout::onLuluDesk(center);
        float pressShift = std::max(physics.mousePressLeft.value, physics.mousePressRight.value) * 1.6f;

        GraphicsState st = g.Save();
        g.TranslateTransform(center.x, center.y + pressShift);
        g.RotateTransform(0.22f * RadToDeg);

        SolidBrush shadowBrush(Color(60, 0, 0, 0));
        g.FillEllipse(&shadowBrush, -13.5f, -9.5f, 27.0f, 31.0f);

        Color bodyCol, borderCol, wheelCol;
        if (theme == KeyboardTheme::MidnightHacker) {
            bodyCol = Color(255, 51, 54, 61); borderCol = Color(255, 20, 20, 26); wheelCol = Color(255, 199, 242, 64);
        } else if (theme == KeyboardTheme::RetroCream) {
            bodyCol = Color(255, 227, 222, 209); borderCol = Color(255, 51, 48, 46); wheelCol = Color(255, 224, 107, 61);
        } else {
            bodyCol = Color(255, 244, 242, 236); borderCol = Color(255, 36, 36, 41); wheelCol = Color(255, 245, 176, 41);
        }

        SolidBrush bodyBrush(bodyCol);
        g.FillEllipse(&bodyBrush, -13.5f, -16.5f, 27.0f, 33.0f);

        if (physics.mousePressLeft.value > 0.05f || physics.mousePressRight.value > 0.05f) {
            GraphicsPath clipPath;
            clipPath.AddEllipse(-13.5f, -16.5f, 27.0f, 33.0f);
            GraphicsState cs = g.Save();
            g.SetClip(&clipPath);
            if (physics.mousePressLeft.value > 0.05f) {
                SolidBrush hb(Color((BYTE)(96 * physics.mousePressLeft.value), 201, 242, 64));
                g.FillRectangle(&hb, 0.0f, 0.0f, 14.0f, 17.0f);
            }
            if (physics.mousePressRight.value > 0.05f) {
                SolidBrush hb(Color((BYTE)(96 * physics.mousePressRight.value), 201, 242, 64));
                g.FillRectangle(&hb, -14.0f, 0.0f, 14.0f, 17.0f);
            }
            g.Restore(cs);
        }

        // Front button split seam pointing away from cat (bottom)
        Pen seamPen(Color(82, borderCol.GetR(), borderCol.GetG(), borderCol.GetB()), 1.05f);
        g.DrawLine(&seamPen, 0.0f, 16.0f, 0.0f, 1.5f);

        // Amber scroll wheel on front (bottom) half
        GraphicsPath wheelPath;
        addRoundedRect(wheelPath, -1.9f, 2.5f, 3.8f, 8.5f, 1.9f);
        SolidBrush wheelBrush(wheelCol);
        g.FillPath(&wheelBrush, &wheelPath);

        Pen borderPen(borderCol, 1.7f);
        g.DrawEllipse(&borderPen, -13.5f, -16.5f, 27.0f, 33.0f);
        g.Restore(st);
    }

    // MARK: - Mini 65% Mechanical Keyboard (180° facing the cat)

    static void drawKeyboard(Gdiplus::Graphics& g, const CatPhysics& physics, KeyboardTheme theme, EyeColorTheme eyeTheme, bool onDesk) {
        using namespace Gdiplus;
        auto project = [onDesk](float u, float v) {
            Pt p = KeyboardLayout::screenQuadPoint(u, v);
            return onDesk ? KeyboardLayout::onLuluDesk(p) : p;
        };
        Pt tl = project(-0.025f, -0.08f);
        Pt tr = project(1.025f, -0.08f);
        Pt bl = project(-0.032f, 1.08f);
        Pt br = project(1.032f, 1.08f);
        constexpr float depth = 6.5f;

        // Shadow
        {
            GraphicsPath sp;
            addQuadPoly(sp, {tl.x, tl.y + depth + 2}, {tr.x, tr.y + depth + 2},
                            {br.x, br.y + depth + 3}, {bl.x, bl.y + depth + 3});
            SolidBrush sb(Color(56, 0, 0, 0));
            g.FillPath(&sb, &sp);
        }

        bool pink = theme == KeyboardTheme::DaisyPink;
        Color baseCol, deckCol, keyCol, borderCol, accentCol;
        if (pink) {
            baseCol = Color(255, 205, 104, 146); deckCol = Color(255, 244, 151, 189);
            keyCol = Color(255, 255, 255, 255); borderCol = Color(255, 223, 168, 192); accentCol = keyCol;
        } else if (theme == KeyboardTheme::MidnightHacker) {
            baseCol = Color(255, 15, 15, 20); deckCol = Color(255, 26, 28, 33);
            keyCol = Color(255, 54, 56, 64); borderCol = Color(242, 28, 28, 33); accentCol = Color(255, 196, 240, 64);
        } else if (theme == KeyboardTheme::RetroCream) {
            baseCol = Color(255, 64, 61, 56); deckCol = Color(255, 97, 92, 84);
            keyCol = Color(255, 230, 224, 209); borderCol = Color(216, 173, 166, 153); accentCol = Color(255, 224, 107, 61);
        } else {
            baseCol = Color(255, 26, 26, 31); deckCol = Color(255, 36, 36, 41);
            keyCol = Color(255, 247, 246, 240); borderCol = Color(190, 199, 196, 189); accentCol = Color(255, 247, 179, 41);
        }

        {
            GraphicsPath lp;
            addQuadPoly(lp, {tl.x, tl.y + depth}, {tr.x, tr.y + depth}, {br.x, br.y + depth}, {bl.x, bl.y + depth});
            SolidBrush bb(baseCol);
            Pen bp(baseCol, 5.0f); bp.SetLineJoin(LineJoinRound);
            g.FillPath(&bb, &lp);
            g.DrawPath(&bp, &lp);
        }
        {
            GraphicsPath tp;
            addQuadPoly(tp, tl, tr, br, bl);
            SolidBrush db(deckCol);
            Pen dp(deckCol, 4.0f); dp.SetLineJoin(LineJoinRound);
            g.FillPath(&db, &tp);
            g.DrawPath(&dp, &tp);
        }
        if (pink) {
            GraphicsPath keywell;
            addQuadPoly(keywell, project(-0.008f, -0.018f), project(1.008f, -0.018f),
                        project(1.008f, 1.018f), project(-0.008f, 1.018f));
            SolidBrush plate(Color(255, 255, 233, 243));
            g.FillPath(&plate, &keywell);
        }

        constexpr float padU = 0.0028f;
        constexpr float padV = 0.014f;
        Color glowCol = pink ? Color(255, 255, 74, 155) : eyePrimary(eyeTheme, 215);
        Pen keyBorderPen(borderCol, 0.7f);

        for (const auto& cap : KeyboardLayout::keycaps()) {
            float glow = 0.0f;
            auto it = physics.keyGlow.find(cap.key);
            if (it != physics.keyGlow.end()) glow = it->second;

            // 180° rotated to face the cat
            float su0 = (1.0f - (cap.xUnits + cap.widthUnits)) + padU;
            float su1 = (1.0f - cap.xUnits) - padU;
            float sv0 = (1.0f - (cap.row + 1) / 5.0f) + padV;
            float sv1 = (1.0f - cap.row / 5.0f) - padV;

            float dy = glow * 1.9f;
            Pt kTL = project(su0, sv0); kTL.y += dy;
            Pt kTR = project(su1, sv0); kTR.y += dy;
            Pt kBR = project(su1, sv1); kBR.y += dy;
            Pt kBL = project(su0, sv1); kBL.y += dy;

            GraphicsPath kp;
            addQuadPoly(kp, kTL, kTR, kBR, kBL);
            Color fill = cap.isAmberAccent ? accentCol : keyCol;
            if (glow > 0.02f) fill = blendColor(fill, glowCol, glow * (pink ? 0.82f : 0.55f));

            SolidBrush kb(fill);
            g.FillPath(&kb, &kp);
            g.DrawPath(&keyBorderPen, &kp);
        }

        // Radiant backlit glow halos over pressed keys
        for (const auto& kv : physics.keyGlow) {
            if (kv.second <= 0.05f) continue;
            Pt c = KeyboardLayout::canvasPointForKey(kv.first);
            if (onDesk) c = KeyboardLayout::onLuluDesk(c);
            float r = 17.0f + kv.second * 6.0f;
            GraphicsPath gp;
            gp.AddEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f);
            PathGradientBrush pgb(&gp);
            pgb.SetCenterColor(Color((BYTE)((pink ? 133 : 92) * kv.second), glowCol.GetR(), glowCol.GetG(), glowCol.GetB()));
            Color surr[1] = { Color(0, glowCol.GetR(), glowCol.GetG(), glowCol.GetB()) };
            INT cnt = 1;
            pgb.SetSurroundColors(surr, &cnt);
            g.FillPath(&pgb, &gp);
        }
    }

    // MARK: - Tail

    static void drawTail(Gdiplus::Graphics& g, float seatX, float seatY, const CatPhysics& physics, CatCoatTheme coat) {
        using namespace Gdiplus;
        float p = physics.tailPhase;
        float hunt = std::clamp(physics.huntIntensity.value, 0.0f, 1.0f);
        float wag1 = std::sin(p) * (8.0f + hunt * 5.5f);
        float wag2 = std::cos(p * 1.3f) * (6.0f + hunt * 4.5f);
        float huntDrop = hunt * 14.0f;

        PointF root(seatX + 34.0f, seatY - 6.0f);
        PointF c1(seatX + 64.0f, seatY + 2.0f + wag1 * 0.35f + huntDrop * 0.4f);
        PointF c2(seatX + 66.0f + wag2 * 0.4f, seatY - 26.0f + wag1 * 0.7f + huntDrop);
        PointF tip(seatX + 91.0f + wag2 * 0.5f, seatY - 30.0f + wag1 * 0.85f + huntDrop);

        if (coat == CatCoatTheme::Ragdoll) {
            float tipX = tip.X + 10.0f;
            float tipY = tip.Y + 10.0f;
            GraphicsPath plume;
            plume.AddBezier(root, PointF(c1.X, seatY - 13.0f), PointF(c2.X - 9.0f, tipY - 12.0f), PointF(tipX - 14.0f, tipY - 12.0f));
            plume.AddBezier(PointF(tipX - 14.0f, tipY - 12.0f), PointF(tipX + 3.0f, tipY - 13.0f), PointF(tipX + 8.0f, tipY - 3.0f), PointF(tipX + 3.0f, tipY + 2.0f));
            plume.AddBezier(PointF(tipX + 3.0f, tipY + 2.0f), PointF(tipX + 1.0f, tipY + 4.0f), PointF(tipX - 4.0f, tipY + 5.0f), PointF(tipX - 9.0f, tipY + 5.5f));
            plume.AddBezier(PointF(tipX - 9.0f, tipY + 5.5f), PointF(c2.X, tipY + 22.0f), PointF(c1.X - 9.0f, seatY + 14.0f), PointF(root.X - 4.0f, root.Y + 14.0f));
            plume.CloseFigure();
            LinearGradientBrush fur(PointF(root.X, tipY - 18.0f), PointF(tipX + 11.0f, seatY + 23.0f),
                                    Color(255, 173, 145, 123), ragdollPoint());
            g.FillPath(&fur, &plume);
            Pen edge(ragdollOutline(174), 1.05f);
            edge.SetLineJoin(LineJoinRound);
            g.DrawPath(&edge, &plume);
            Pen sheen(Color(113, 220, 190, 155), 1.25f);
            sheen.SetStartCap(LineCapRound);
            sheen.SetEndCap(LineCapRound);
            g.DrawBezier(&sheen, PointF(root.X + 12.0f, root.Y - 1.0f), PointF(c1.X + 7.0f, seatY - 6.0f),
                         PointF(c2.X - 2.0f, tipY - 9.0f), PointF(tipX - 10.0f, tipY - 7.0f));
            return;
        }

        Pen tailPen(coat == CatCoatTheme::Siamese ? siamesePoint() : catBlack(), 11.0f);
        tailPen.SetStartCap(LineCapRound);
        tailPen.SetEndCap(LineCapRound);
        tailPen.SetLineJoin(LineJoinRound);
        g.DrawBezier(&tailPen, root, c1, c2, tip);
    }

    // MARK: - 3D-Projected Ears

    static void drawEarsLayer(Gdiplus::Graphics& g, Pt center, const CatPhysics& physics, CatCoatTheme coat) {
        using namespace Gdiplus;
        GraphicsState st = g.Save();
        g.TranslateTransform(center.x, center.y);
        g.RotateTransform(physics.headTilt.value * RadToDeg);

        float yawRad = physics.headYaw.value * 0.50f;
        float pitchRad = physics.headPitch.value * 0.36f;
        float fold = physics.earFold.value;
        float twitch = physics.earTwitch.value * 0.045f;

        drawEar3D(g, true, yawRad, pitchRad, fold, twitch, coat);
        drawEar3D(g, false, yawRad, pitchRad, fold, -twitch, coat);
        g.Restore(st);
    }

    static void drawEar3D(Gdiplus::Graphics& g, bool isLeft, float yawRad, float pitchRad, float fold, float twitch, CatCoatTheme coat) {
        using namespace Gdiplus;
        float sign = isLeft ? -1.0f : 1.0f;
        bool ragdoll = coat == CatCoatTheme::Ragdoll;
        float foldAngle = sign * (0.06f + fold * 0.34f) + twitch;
        float earHeight = ragdoll ? 25.0f : 32.0f;
        float tipDx = std::sin(foldAngle) * earHeight;
        float tipDy = -std::cos(foldAngle) * earHeight;

        Pt innerBase = projectHead3D(sign * (ragdoll ? 10.0f : 9.0f), -16.0f, 0.0f, yawRad, pitchRad);
        Pt outerBase = projectHead3D(sign * (ragdoll ? 44.0f : 35.0f), -5.0f, -2.0f, yawRad, pitchRad);
        Pt tip3D     = projectHead3D(sign * (ragdoll ? 32.0f : 24.5f) + tipDx, -20.0f + tipDy, -3.0f, yawRad, pitchRad);

        Pt tipL = { tip3D.x - sign * 3.8f, tip3D.y + 2.8f };
        Pt tipR = { tip3D.x + sign * 3.8f, tip3D.y + 3.2f };

        GraphicsPath ear;
        addQuadBezier(ear, innerBase, { (innerBase.x + tipL.x) * 0.5f - sign * 1.5f, (innerBase.y + tipL.y) * 0.5f - 2.0f }, tipL);
        addQuadBezier(ear, tipL, { tip3D.x, tip3D.y - 2.2f }, tipR);
        addQuadBezier(ear, tipR, { (outerBase.x + tipR.x) * 0.5f + sign * 2.5f, (outerBase.y + tipR.y) * 0.5f - 2.0f }, outerBase);
        ear.CloseFigure();

        if (ragdoll) {
            LinearGradientBrush fur(PointF(tip3D.x, tip3D.y - 6.0f), PointF(tip3D.x, outerBase.y + 8.0f),
                                    Color(255, 83, 66, 62), Color(255, 170, 138, 115));
            Pen rim(ragdollOutline(190), 1.05f);
            g.FillPath(&fur, &ear);
            g.DrawPath(&rim, &ear);

            GraphicsPath innerEar;
            addQuadBezier(innerEar, {innerBase.x + sign * 5.0f, innerBase.y},
                          {tip3D.x - sign * 3.0f, tip3D.y + 14.0f},
                          {tip3D.x, tip3D.y + 8.0f});
            addQuadBezier(innerEar, {tip3D.x, tip3D.y + 8.0f},
                          {tip3D.x + sign * 5.0f, tip3D.y + 14.0f},
                          {outerBase.x - sign * 4.0f, outerBase.y - 2.0f});
            innerEar.CloseFigure();
            LinearGradientBrush inner(PointF(tip3D.x, tip3D.y + 3.0f), PointF(tip3D.x, outerBase.y + 8.0f),
                                      Color(255, 156, 115, 106), ragdollIvory());
            g.FillPath(&inner, &innerEar);
            return;
        }

        SolidBrush b(coat == CatCoatTheme::Siamese ? siamesePoint() : catBlack());
        g.FillPath(&b, &ear);
    }

    // MARK: - Unified C1-Continuous Cat Silhouette (+ Siamese 2.5D Face Mask)

    static void drawUnifiedSilhouette(Gdiplus::Graphics& g, float seatX, float seatY, Pt headCenter, float breath, const CatPhysics& physics, CatCoatTheme coat) {
        using namespace Gdiplus;
        float yaw = physics.headYaw.value;
        float pitch = physics.headPitch.value;
        float hTilt = physics.headTilt.value;
        float bTilt = physics.bodyTilt.value;

        float cH = std::cos(hTilt), sH = std::sin(hTilt);
        auto H = [&](float lx, float ly) -> PointF {
            return PointF(headCenter.x + lx * cH - ly * sH, headCenter.y + lx * sH + ly * cH);
        };

        float cB = std::cos(bTilt), sB = std::sin(bTilt);
        float sy = physics.bodySquash.value;
        float sx = 1.0f + (1.0f - sy) * 0.55f;
        auto B = [&](float lx, float ly) -> PointF {
            float rx = lx * sx, ry = ly * sy;
            return PointF(seatX + rx * cB - ry * sB, seatY + rx * sB + ry * cB);
        };

        float cheekShift = yaw * 3.0f;
        float crownDrop = pitch * 2.0f;
        bool ragdoll = coat == CatCoatTheme::Ragdoll;
        float cheekWidth = ragdoll ? 49.0f : 42.5f;
        float hipWidth = ragdoll ? 53.0f : 49.0f;
        float leftCheekX = -cheekWidth + cheekShift * 0.4f;
        float rightCheekX = cheekWidth + cheekShift * 0.4f;

        PointF crown  = H(cheekShift * 0.2f, -30.0f + crownDrop);
        PointF rCheek = H(rightCheekX, 5.0f);
        PointF rHip   = B(hipWidth, -14.0f);
        PointF rBot   = B(24.0f, 12.0f);
        PointF lBot   = B(-24.0f, 12.0f);
        PointF lHip   = B(-hipWidth, -14.0f);
        PointF lCheek = H(leftCheekX, 5.0f);

        GraphicsPath path;
        path.AddBezier(crown, H(26.0f + cheekShift * 0.3f, -30.0f + crownDrop), H(rightCheekX + 1.5f, -14.0f), rCheek);
        if (ragdoll) {
            PointF tuft = H(rightCheekX + 6.0f, 12.0f);
            PointF neck = H(rightCheekX - 3.0f, 20.0f);
            PointF shoulder = B(53.0f, -54.0f + breath * 0.5f);
            path.AddBezier(rCheek, H(rightCheekX + 2.0f, 9.0f), H(rightCheekX + 4.0f, 11.0f), tuft);
            path.AddBezier(tuft, H(rightCheekX + 4.0f, 16.0f), H(rightCheekX, 18.0f), neck);
            path.AddBezier(neck, H(rightCheekX + 8.0f, 29.0f), B(47.0f, -58.0f), shoulder);
            path.AddBezier(shoulder, B(48.0f, -52.0f), B(61.5f, -30.0f), rHip);
        } else {
            path.AddBezier(rCheek, H(rightCheekX - 2.5f, 24.0f), B(51.5f, -48.0f + breath * 0.5f), rHip);
        }
        path.AddBezier(rHip, B(47.0f, 5.0f), B(36.0f, 12.0f), rBot);
        path.AddLine(rBot, lBot);
        path.AddBezier(lBot, B(-36.0f, 12.0f), B(-47.0f, 5.0f), lHip);
        if (ragdoll) {
            PointF shoulder = B(-53.0f, -54.0f + breath * 0.5f);
            PointF neck = H(leftCheekX + 3.0f, 20.0f);
            PointF tuft = H(leftCheekX - 6.0f, 12.0f);
            path.AddBezier(lHip, B(-61.5f, -30.0f), B(-48.0f, -52.0f), shoulder);
            path.AddBezier(shoulder, B(-47.0f, -58.0f), H(leftCheekX - 8.0f, 29.0f), neck);
            path.AddBezier(neck, H(leftCheekX, 18.0f), H(leftCheekX - 4.0f, 16.0f), tuft);
            path.AddBezier(tuft, H(leftCheekX - 4.0f, 11.0f), H(leftCheekX - 2.0f, 9.0f), lCheek);
        } else {
            path.AddBezier(lHip, B(-51.5f, -48.0f + breath * 0.5f), H(leftCheekX + 2.5f, 24.0f), lCheek);
        }
        path.AddBezier(lCheek, H(leftCheekX - 1.5f, -14.0f), H(-26.0f + cheekShift * 0.3f, -30.0f + crownDrop), crown);
        path.CloseFigure();

        if (coat == CatCoatTheme::ClassicBlack) {
            SolidBrush b(catBlack());
            g.FillPath(&b, &path);
        } else if (coat == CatCoatTheme::Ragdoll) {
            drawRagdollCoat(g, path, headCenter, seatY, physics);
        } else {
            GraphicsState cs = g.Save();
            g.SetClip(&path);

            LinearGradientBrush bodyGrad(
                PointF(seatX, headCenter.y - 54.0f),
                PointF(seatX, seatY + 38.0f),
                siameseCream(),
                siameseLatte()
            );
            bodyGrad.SetWrapMode(WrapModeTileFlipXY);
            Color bodyCols[4] = { siameseCream(), siameseCream(), siameseLatte(), siameseLatte() };
            REAL bodyPos[4] = { 0.0f, 0.22f, 0.80f, 1.0f };
            bodyGrad.SetInterpolationColors(bodyCols, bodyPos, 4);
            g.FillPath(&bodyGrad, &path);

            // 2.5D Rotating Seal-Point "挖煤" Face Mask!
            float yawRad = yaw * 0.50f;
            float pitchRad = pitch * 0.36f;
            Pt maskC = projectHead3D(0.0f, 2.5f, 27.0f, yawRad, pitchRad);
            float maskSX = std::max(0.76f, std::cos(yawRad * 0.7f)) * 1.18f;
            float maskSY = std::max(0.82f, std::cos(pitchRad * 0.7f)) * 0.88f;

            GraphicsState ms = g.Save();
            g.TranslateTransform(headCenter.x, headCenter.y);
            g.RotateTransform(hTilt * RadToDeg);
            g.TranslateTransform(maskC.x, maskC.y);
            g.ScaleTransform(maskSX, maskSY);

            GraphicsPath maskEllipse;
            maskEllipse.AddEllipse(-31.0f, -31.0f, 62.0f, 62.0f);
            PathGradientBrush maskBrush(&maskEllipse);
            Color cols[4] = { Color(0, 115, 87, 69), Color(178, 115, 87, 69), siamesePoint(245), siamesePoint(255) };
            REAL pos[4] = { 0.0f, 0.26f, 0.56f, 1.0f };
            maskBrush.SetInterpolationColors(cols, pos, 4);
            g.FillPath(&maskBrush, &maskEllipse);

            g.Restore(ms);
            g.Restore(cs);

            // Clean subtle warm-cocoa outline so Siamese pops on white & dark wallpapers
            Pen outPen(siameseOutline(), 1.5f);
            g.DrawPath(&outPen, &path);
        }

        // Tucked hind paws at bottom
        GraphicsState ps = g.Save();
        g.TranslateTransform(seatX, seatY);
        g.RotateTransform(bTilt * RadToDeg);
        g.ScaleTransform(sx, sy);

        Color pawColor = catBlack();
        Color pawRim = contourMint(148);
        if (coat == CatCoatTheme::Siamese) {
            pawColor = siamesePoint();
            pawRim = siameseCream(140);
        } else if (coat == CatCoatTheme::Ragdoll) {
            pawColor = ragdollIvory();
            pawRim = ragdollOutline(149);
        }
        SolidBrush pawBrush(pawColor);
        g.FillEllipse(&pawBrush, -35.0f, 3.0f, 17.0f, 11.0f);
        if (physics.hindFootActive.value < 0.35f) {
            g.FillEllipse(&pawBrush, 18.0f, 3.0f, 17.0f, 11.0f);
        }

        Pen rimPen(pawRim, 1.3f);
        rimPen.SetStartCap(LineCapRound); rimPen.SetEndCap(LineCapRound);
        g.DrawArc(&rimPen, -34.0f, 1.0f, 15.0f, 15.0f, 15.0f, 150.0f);
        if (physics.hindFootActive.value < 0.35f) {
            g.DrawArc(&rimPen, 19.0f, 1.0f, 15.0f, 15.0f, 15.0f, 150.0f);
        }
        g.Restore(ps);
    }

    static void shadeFur(Gdiplus::Graphics& g, Gdiplus::RectF area, Gdiplus::Color color) {
        using namespace Gdiplus;
        GraphicsPath ellipse;
        ellipse.AddEllipse(area);
        PathGradientBrush shade(&ellipse);
        shade.SetCenterColor(color);
        Color edge(0, color.GetR(), color.GetG(), color.GetB());
        INT count = 1;
        shade.SetSurroundColors(&edge, &count);
        g.FillPath(&shade, &ellipse);
    }

    static void drawRagdollCoat(Gdiplus::Graphics& g, Gdiplus::GraphicsPath& silhouette, Pt center,
                                float seatY, const CatPhysics& physics) {
        using namespace Gdiplus;
        LinearGradientBrush coat(PointF(center.x, center.y - 55.0f), PointF(center.x, seatY + 30.0f),
                                 ragdollIvory(), ragdollShade());
        Color coatStops[] = { Color(255, 228, 208, 190), ragdollIvory(), Color(255, 235, 220, 201), ragdollShade() };
        REAL coatPositions[] = { 0.0f, 0.42f, 0.66f, 1.0f };
        coat.SetInterpolationColors(coatStops, coatPositions, 4);
        g.FillPath(&coat, &silhouette);

        GraphicsState state = g.Save();
        g.SetClip(&silhouette);
        shadeFur(g, RectF(center.x + 21.0f, seatY - 69.0f, 54.0f, 97.0f), Color(173, 161, 126, 102));
        shadeFur(g, RectF(center.x - 82.0f, seatY - 54.0f, 65.0f, 76.0f), Color(128, 174, 141, 115));
        g.TranslateTransform(center.x, center.y);
        g.RotateTransform(physics.headTilt.value * RadToDeg);

        // Match the wider white forehead blaze and longer dark patch on the cat's left cheek.
        float yaw = physics.headYaw.value * 0.50f;
        float pitch = physics.headPitch.value * 0.36f;
        LinearGradientBrush point(PointF(0.0f, -58.0f), PointF(0.0f, 35.0f),
                                  Color(255, 170, 142, 119), Color(0, 179, 149, 125));
        Color pointStops[] = { Color(255, 157, 129, 109), Color(255, 164, 137, 115),
                              Color(255, 134, 106, 89), Color(153, 175, 142, 116), Color(0, 179, 149, 125) };
        REAL pointPositions[] = { 0.0f, 0.31f, 0.64f, 0.84f, 1.0f };
        point.SetInterpolationColors(pointStops, pointPositions, 5);
        for (float side : {-1.0f, 1.0f}) {
            bool longerPatch = side > 0.0f;
            auto projected = [&](float x, float y) {
                Pt p = projectHead3D(side * x, y, 27.0f, yaw, pitch);
                return PointF(p.x, p.y);
            };
            GraphicsPath marking;
            PointF crown = projected(longerPatch ? -1.0f : 5.5f, -45.0f);
            PointF brow = projected(longerPatch ? 11.3f : 9.6f, -17.0f);
            PointF bridge = projected(longerPatch ? 7.0f : 8.2f, 0.0f);
            float cheekHeight = longerPatch ? 23.0f : 14.0f;
            PointF cheek = projected(longerPatch ? 31.0f : 26.0f, cheekHeight);
            PointF temple = projected(62.0f, longerPatch ? 8.0f : 5.0f);
            marking.AddBezier(crown, projected(2.0f, -29.0f), projected(9.0f, -25.0f), brow);
            marking.AddBezier(brow, projected(13.0f, -12.0f), projected(6.0f, -9.0f), bridge);
            marking.AddBezier(bridge, projected(14.0f, 5.0f), projected(23.0f, cheekHeight - 5.0f), cheek);
            marking.AddBezier(cheek, projected(43.0f, longerPatch ? 25.0f : 19.0f), projected(53.0f, 16.0f), temple);
            marking.AddBezier(temple, projected(63.0f, -14.0f), projected(65.0f, -32.0f), projected(65.0f, -46.0f));
            marking.CloseFigure();
            g.FillPath(&point, &marking);

            GraphicsState patchClip = g.Save();
            g.SetClip(&marking, CombineModeIntersect);
            PointF aroundEye = projected(longerPatch ? 25.0f : 21.0f, longerPatch ? 3.0f : 0.0f);
            shadeFur(g, RectF(aroundEye.X - 25.0f, aroundEye.Y - 19.0f, 50.0f, 39.0f),
                     Color(223, 81, 59, 49));
            PointF eyebrow = projected(27.0f, -18.0f);
            shadeFur(g, RectF(eyebrow.X - 16.0f, eyebrow.Y - 9.0f, 32.0f, 18.0f),
                     Color(150, 205, 174, 143));
            g.Restore(patchClip);
        }

        Pt chin = projectHead3D(0.0f, 32.0f, 24.0f, yaw, pitch);
        shadeFur(g, RectF(chin.x - 39.0f, chin.y - 24.0f, 78.0f, 30.0f), Color(153, 145, 115, 98));
        GraphicsPath ruff;
        addQuadBezier(ruff, {-46.0f, 20.0f}, {-50.0f, 40.0f}, {-34.0f, 49.0f});
        addQuadBezier(ruff, {-34.0f, 49.0f}, {-31.0f, 44.0f}, {-32.0f, 41.0f});
        addQuadBezier(ruff, {-32.0f, 41.0f}, {-29.0f, 53.0f}, {-18.0f, 58.0f});
        addQuadBezier(ruff, {-18.0f, 58.0f}, {-18.0f, 52.0f}, {-15.0f, 49.0f});
        addQuadBezier(ruff, {-15.0f, 49.0f}, {-12.0f, 62.0f}, {-2.0f, 65.0f});
        addQuadBezier(ruff, {-2.0f, 65.0f}, {6.0f, 61.0f}, {9.0f, 53.0f});
        addQuadBezier(ruff, {9.0f, 53.0f}, {13.0f, 54.0f}, {14.0f, 59.0f});
        addQuadBezier(ruff, {14.0f, 59.0f}, {29.0f, 52.0f}, {30.0f, 43.0f});
        addQuadBezier(ruff, {30.0f, 43.0f}, {33.0f, 43.0f}, {33.0f, 49.0f});
        addQuadBezier(ruff, {33.0f, 49.0f}, {51.0f, 35.0f}, {46.0f, 20.0f});
        ruff.CloseFigure();
        LinearGradientBrush chest(PointF(0.0f, 16.0f), PointF(0.0f, 70.0f), ragdollIvory(0), Color(255, 247, 230, 208));
        Color chestStops[] = { ragdollIvory(0), ragdollIvory(174), ragdollIvory(), Color(255, 247, 230, 208) };
        REAL chestPositions[] = { 0.0f, 0.23f, 0.59f, 1.0f };
        chest.SetInterpolationColors(chestStops, chestPositions, 4);
        g.FillPath(&chest, &ruff);

        Pen locks(Color(89, 181, 151, 126), 1.15f);
        locks.SetStartCap(LineCapRound);
        locks.SetEndCap(LineCapRound);
        const PointF furFlow[][4] = {
            {{-39, 28}, {-42, 34}, {-38, 40}, {-34, 42}},
            {{-27, 32}, {-30, 41}, {-23, 46}, {-21, 51}},
            {{-11, 36}, {-14, 45}, {-6, 48}, {-7, 55}},
            {{5, 36}, {8, 45}, {2, 51}, {0, 58}},
            {{21, 34}, {22, 43}, {16, 46}, {14, 50}},
            {{38, 27}, {42, 35}, {35, 40}, {32, 43}}
        };
        for (const auto& strand : furFlow) {
            g.DrawBezier(&locks, strand[0], strand[1], strand[2], strand[3]);
        }
        g.Restore(state);

        Pen edge(ragdollOutline(158), 1.15f);
        edge.SetLineJoin(LineJoinRound);
        g.DrawPath(&edge, &silhouette);
    }

    // MARK: - 3rd Key Hind Foot

    static void drawHindFoot(Gdiplus::Graphics& g, float seatX, float seatY, const CatPhysics& physics, CatCoatTheme coat) {
        Pt hip = { seatX + 26.0f, seatY + 2.0f };
        Pt target = physics.hindFoot.point();
        Pt mid = { (hip.x + target.x) * 0.5f + 10.0f, (hip.y + target.y) * 0.5f - 8.0f };
        drawLimb(g, hip, mid, target, 15.0f, 12.0f, true, 0.85f * physics.hindFootActive.value, coat);
    }

    // MARK: - 2.5D Eyes & Whiskers

    static void drawHeadFeatures(Gdiplus::Graphics& g, Pt center, const CatPhysics& physics, EyeColorTheme eyeTheme, CatCoatTheme coat) {
        using namespace Gdiplus;
        GraphicsState st = g.Save();
        g.TranslateTransform(center.x, center.y);
        g.RotateTransform(physics.headTilt.value * RadToDeg);

        float yawRad = physics.headYaw.value * 0.50f;
        float pitchRad = physics.headPitch.value * 0.36f;

        if (coat == CatCoatTheme::Ragdoll) {
            drawRagdollMuzzle(g, yawRad, pitchRad);
        }
        drawEye3D(g, true, yawRad, pitchRad, physics, eyeTheme, coat);
        drawEye3D(g, false, yawRad, pitchRad, physics, eyeTheme, coat);
        drawWhiskers3D(g, yawRad, pitchRad, physics, coat);
        g.Restore(st);
    }

    static void drawRagdollMuzzle(Gdiplus::Graphics& g, float yaw, float pitch) {
        using namespace Gdiplus;
        auto face = [&](float x, float y) { return projectHead3D(x, y, 29.0f, yaw, pitch); };
        for (float side : {-1.0f, 1.0f}) {
            Pt cheek = face(side * 11.0f, 13.0f);
            shadeFur(g, RectF(cheek.x - 12.5f, cheek.y - 6.5f, 25.0f, 17.0f), Color(221, 255, 248, 231));
        }
        Pt noseLeft = face(-4.3f, 10.0f);
        Pt noseRight = face(4.3f, 10.0f);
        Pt noseTip = face(0.0f, 15.4f);
        GraphicsPath nose;
        addQuadBezier(nose, noseLeft, face(0.0f, 8.2f), noseRight);
        addQuadBezier(nose, noseRight, face(3.8f, 13.0f), noseTip);
        addQuadBezier(nose, noseTip, face(-3.8f, 13.0f), noseLeft);
        nose.CloseFigure();
        LinearGradientBrush pink(PointF(noseTip.x, noseTip.y - 9.0f), PointF(noseTip.x, noseTip.y + 3.0f),
                                 Color(255, 225, 163, 149), Color(255, 161, 93, 90));
        g.FillPath(&pink, &nose);

        GraphicsPath mouth;
        addQuadBezier(mouth, face(-6.7f, 19.0f), face(-2.8f, 19.5f), face(0.0f, 16.8f));
        addQuadBezier(mouth, face(0.0f, 16.8f), face(2.8f, 19.5f), face(6.7f, 19.0f));
        Pen lip(Color(177, 127, 86, 78), 0.94f);
        lip.SetStartCap(LineCapRound);
        lip.SetEndCap(LineCapRound);
        g.DrawPath(&lip, &mouth);
        Pt upperLip = face(0.0f, 17.1f);
        g.DrawLine(&lip, noseTip.x, noseTip.y, upperLip.x, upperLip.y);
    }

    static void drawEye3D(Gdiplus::Graphics& g, bool isLeft, float yawRad, float pitchRad, const CatPhysics& physics, EyeColorTheme eyeTheme, CatCoatTheme coat) {
        using namespace Gdiplus;
        float sign = isLeft ? -1.0f : 1.0f;
        Color eyeCol = eyePrimary(eyeTheme);
        bool ragdoll = coat == CatCoatTheme::Ragdoll;
        float closure = physics.effectiveEyeClosure();
        Pt proj = projectHead3D(sign * (ragdoll ? 18.4f : 17.2f), -2.0f, 27.5f, yawRad, pitchRad);

        float eyeNormal = yawRad + sign * 0.34f;
        float foreX = std::clamp(std::cos(eyeNormal) / std::cos(0.34f), 0.64f, 1.06f);
        float foreY = std::clamp(std::cos(pitchRad * 0.85f), 0.78f, 1.03f);

        GraphicsState st = g.Save();
        g.TranslateTransform(proj.x, proj.y);
        g.ScaleTransform(foreX, foreY);

        // Mode 1: Happy closed glowing curved line eyes (petting head!)
        if (physics.isHappyClosedEyes() || (ragdoll && closure > 0.92f)) {
            GraphicsPath hp;
            addQuadBezier(hp, {-11.5f, 1.0f}, {sign * 1.5f, 3.4f}, {11.5f, 1.0f});
            Pen p(ragdoll ? Color(255, 80, 56, 48) : eyeCol, ragdoll ? 1.85f : 2.6f);
            p.SetStartCap(LineCapRound); p.SetEndCap(LineCapRound);
            g.DrawPath(&p, &hp);
            g.Restore(st);
            return;
        }

        // Mode 2: Blissful half-squint glowing slits (petting body!)
        if (physics.isBlissfulSquint()) {
            GraphicsPath sp;
            addQuadBezier(sp, {-11.5f, -0.5f}, {0.0f, 0.6f}, {11.5f, -0.5f});
            addQuadBezier(sp, {11.5f, -0.5f}, {0.0f, 4.8f}, {-11.5f, -0.5f});
            sp.CloseFigure();
            SolidBrush sb(ragdoll ? Color(255, 80, 56, 48) : eyeCol);
            g.FillPath(&sb, &sp);
            g.Restore(st);
            return;
        }

        float hunt = std::clamp(physics.huntIntensity.value, 0.0f, 1.0f);
        g.RotateTransform(sign * (0.06f * (1.0f - hunt * 0.6f)) * RadToDeg);
        if (ragdoll) {
            g.ScaleTransform(1.0f, std::max(0.1f, 1.0f - closure));
        }

        float rx = (ragdoll ? 11.15f : 12.0f) + hunt * 1.1f;
        float ry = (ragdoll ? 9.25f : 10.8f) + hunt * 1.8f;

        GraphicsPath eyeClip;
        eyeClip.AddEllipse(-rx, -ry, rx * 2.0f, ry * 2.0f);
        GraphicsState cs = g.Save();
        g.SetClip(&eyeClip);

        if (ragdoll) {
            LinearGradientBrush iris(PointF(0.0f, -ry - 2.0f), PointF(0.0f, ry + 2.0f),
                                     blendColor(eyeCol, Color(255, 39, 65, 102), 0.86f),
                                     blendColor(eyeCol, Color(255, 201, 219, 236), 0.43f));
            g.FillEllipse(&iris, -rx, -ry, rx * 2.0f, ry * 2.0f);
        } else {
            SolidBrush iris(eyeCol);
            g.FillRectangle(&iris, -rx - 1.0f, -ry - 1.0f, rx * 2.0f + 2.0f, ry * 2.0f + 2.0f);
        }

        // Pupil morphs smoothly from vertical slit -> HUGE round black boba pupil!
        Pt gaze = physics.eyeGaze.point();
        float px = gaze.x * (4.2f - hunt * 1.2f);
        float py = gaze.y * (3.6f - hunt * 1.0f);
        float pScale = physics.pupilScale.value;
        float normalW = (ragdoll ? 12.4f : 5.0f) * pScale;
        float normalH = (ragdoll ? 14.2f : 13.5f) * std::min(1.12f, pScale);
        float huntDiam = ragdoll ? 17.5f : 18.8f;
        float pupilW = normalW + (huntDiam - normalW) * hunt;
        float pupilH = normalH + (huntDiam - normalH) * hunt;

        SolidBrush pupilBrush(catBlack());
        g.FillEllipse(&pupilBrush, px - pupilW * 0.5f, py - pupilH * 0.5f, pupilW, pupilH);

        // Tiny crisp white sparkle glint as boba hunting pupils dilate!
        if (ragdoll) {
            SolidBrush glint(Color(242, 255, 251, 241));
            SolidBrush lowGlint(Color(150, 213, 231, 246));
            g.FillEllipse(&glint, px - 3.9f, py - 5.6f, 3.35f, 3.35f);
            g.FillEllipse(&lowGlint, px + 2.3f, py + 3.4f, 1.8f, 1.8f);
        } else if (hunt > 0.25f) {
            BYTE ga = (BYTE)(std::min(1.0f, (hunt - 0.25f) / 0.55f) * 235.0f);
            float gr = 2.1f + hunt * 0.9f;
            SolidBrush gb(Color(ga, 255, 255, 255));
            g.FillEllipse(&gb, px - pupilW * 0.22f - gr * 0.5f, py - pupilH * 0.22f - gr * 0.5f, gr * 2.0f, gr * 2.0f);
        }

        // Eyelid closure mask (matches black face or Siamese espresso mask!)
        if (!ragdoll && closure > 0.02f) {
            float lidH = (ry * 2.2f) * closure;
            SolidBrush lb(coat == CatCoatTheme::Siamese ? siamesePoint() : catBlack());
            g.FillRectangle(&lb, -rx - 2.0f, -ry - 2.0f, rx * 2.0f + 4.0f, lidH);
        }

        g.Restore(cs);
        if (ragdoll) {
            Pen rim(Color(255, 70, 52, 46), 1.2f);
            g.DrawPath(&rim, &eyeClip);
        }
        g.Restore(st);
    }

    static void drawWhiskers3D(Gdiplus::Graphics& g, float yawRad, float pitchRad, const CatPhysics& physics, CatCoatTheme coat) {
        using namespace Gdiplus;
        float wiggle = std::sin(physics.time * 4.5f) * (std::max(physics.petHeadIntensity, physics.petBodyIntensity) * 1.2f);
        Color color = whiskerWhite();
        if (coat == CatCoatTheme::Siamese) color = siameseWhisker();
        bool ragdoll = coat == CatCoatTheme::Ragdoll;
        if (ragdoll) color = Color(198, 254, 242, 224);
        Pen pen(color, ragdoll ? 0.85f : 1.05f);
        pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound);

        const float rows[3][3] = {
            { 10.5f, 36.5f, 8.8f + wiggle },
            { 14.2f, 37.8f, 14.2f + wiggle * 0.5f },
            { 18.0f, 35.5f, 19.6f - wiggle * 0.4f }
        };
        const float ragdollRows[3][3] = {
            { 13.0f, 44.5f, 10.0f + wiggle },
            { 16.0f, 46.5f, 18.0f + wiggle * 0.5f },
            { 19.0f, 42.0f, 24.0f - wiggle * 0.4f }
        };
        const auto& whiskerRows = ragdoll ? ragdollRows : rows;

        for (float sign : {-1.0f, 1.0f}) {
            for (int i = 0; i < 3; ++i) {
                Pt p0 = projectHead3D(sign * (ragdoll ? 17.8f : 24.0f), whiskerRows[i][0], 23.0f, yawRad, pitchRad);
                Pt p2 = projectHead3D(sign * whiskerRows[i][1], whiskerRows[i][2], 14.0f, yawRad, pitchRad);
                Pt ctrl = { (p0.x + p2.x) * 0.5f, (p0.y + p2.y) * 0.5f - 0.8f };
                GraphicsPath wp;
                addQuadBezier(wp, p0, ctrl, p2);
                g.DrawPath(&pen, &wp);
            }
        }
    }

    // MARK: - Front Paws

    static void drawFrontPaws(Gdiplus::Graphics& g, float seatX, float seatY, float breath, const CatPhysics& physics, CatCoatTheme coat) {
        Pt lPos = physics.leftPaw.point();
        Pt rPos = physics.rightPaw.point();

        float lDist = std::hypot(lPos.x - CatPhysics::LeftPawRest.x, lPos.y - CatPhysics::LeftPawRest.y);
        float rDist = std::hypot(rPos.x - CatPhysics::RightPawRest.x, rPos.y - CatPhysics::RightPawRest.y);

        Pt leftShoulder  = { seatX - 24.0f, seatY - 56.0f + breath };
        Pt rightShoulder = { seatX + 24.0f, seatY - 56.0f + breath };

        if (lDist < 6.0f && !physics.leftPawOnMouse && !physics.leftPawTyping) {
            drawIdleFrontLeg(g, { seatX - 18.0f, seatY - 30.0f }, { seatX - 16.0f, seatY + 12.0f }, true, coat);
        } else {
            Pt mid = { (leftShoulder.x + lPos.x) * 0.5f - (physics.leftPawOnMouse ? 8.0f : 4.0f),
                       (leftShoulder.y + lPos.y) * 0.5f - 6.0f };
            drawLimb(g, leftShoulder, mid, lPos, 19.0f, 13.5f, false, 0.78f, coat);
        }

        if (rDist < 6.0f && !physics.rightPawTyping) {
            drawIdleFrontLeg(g, { seatX + 18.0f, seatY - 30.0f }, { seatX + 16.0f, seatY + 12.0f }, false, coat);
        } else {
            Pt mid = { (rightShoulder.x + rPos.x) * 0.5f + 4.0f, (rightShoulder.y + rPos.y) * 0.5f - 6.0f };
            drawLimb(g, rightShoulder, mid, rPos, 19.0f, 13.5f, true, 0.78f, coat);
        }
    }

    static void drawIdleFrontLeg(Gdiplus::Graphics& g, Pt top, Pt bottom, bool isLeft, CatCoatTheme coat) {
        using namespace Gdiplus;
        float sign = isLeft ? -1.0f : 1.0f;
        bool ragdoll = coat == CatCoatTheme::Ragdoll;

        if (coat != CatCoatTheme::ClassicBlack) {
            GraphicsPath lp;
            addRoundedRect(lp, bottom.x - 7.5f, top.y, 15.0f, bottom.y - top.y + 1.0f, 6.5f);
            float pad = 10.0f;
            float span = std::max(1.0f, (bottom.y + 2.0f) - top.y);
            float total = span + pad * 2.0f;
            Color start = ragdoll ? ragdollIvory(0) : siameseCream(0);
            Color middle = ragdoll ? ragdollShade() : siameseMocha(190);
            Color paw = ragdoll ? ragdollIvory() : siamesePoint();
            LinearGradientBrush grad(PointF(bottom.x, top.y - pad), PointF(bottom.x, bottom.y + 2.0f + pad), start, paw);
            grad.SetWrapMode(WrapModeTileFlipXY);
            Color cols[5] = { start, start, middle, paw, paw };
            REAL pos[5] = { 0.0f, pad / total, (pad + span * 0.52f) / total, (pad + span * 0.85f) / total, 1.0f };
            grad.SetInterpolationColors(cols, pos, 5);
            g.FillPath(&grad, &lp);
        }

        Color legLine = contourMint();
        if (coat == CatCoatTheme::Siamese) legLine = siameseOutline(158);
        if (ragdoll) legLine = ragdollOutline(153);
        Pen p(legLine, 1.45f);
        p.SetStartCap(LineCapRound); p.SetEndCap(LineCapRound);

        GraphicsPath p1;
        addQuadBezier(p1, { top.x + sign * 8.5f, top.y }, { top.x + sign * 9.5f, (top.y + bottom.y) * 0.5f }, { bottom.x + sign * 7.5f, bottom.y - 1.0f });
        g.DrawPath(&p, &p1);

        GraphicsPath p2;
        addQuadBezier(p2, { top.x - sign * 7.0f, top.y + 4.0f }, { top.x - sign * 6.5f, (top.y + bottom.y) * 0.5f }, { bottom.x - sign * 6.0f, bottom.y - 1.0f });
        g.DrawPath(&p, &p2);

        if (ragdoll) {
            RectF foot(bottom.x - 8.1f, bottom.y - 6.0f, 16.2f, 8.4f);
            SolidBrush cream(ragdollIvory());
            g.FillEllipse(&cream, foot);
            Pen toe(ragdollOutline(130), 0.8f);
            toe.SetStartCap(LineCapRound);
            toe.SetEndCap(LineCapRound);
            g.DrawArc(&toe, foot, 4.0f, 174.0f);
            g.DrawLine(&toe, bottom.x - 2.9f, bottom.y - 0.4f, bottom.x - 2.4f, bottom.y + 1.9f);
            g.DrawLine(&toe, bottom.x + 2.9f, bottom.y - 0.4f, bottom.x + 2.4f, bottom.y + 1.9f);
        }
    }

    static void drawLimb(Gdiplus::Graphics& g, Pt start, Pt ctrl, Pt end, float startW, float endW, bool outlineOnLeft, float outlineAlpha, CatCoatTheme coat) {
        using namespace Gdiplus;
        constexpr int steps = 14;
        std::vector<PointF> leftEdge, rightEdge;
        leftEdge.reserve(steps + 1);
        rightEdge.reserve(steps + 1);

        for (int i = 0; i <= steps; ++i) {
            float t = (float)i / (float)steps;
            Pt p = quadPoint(start, ctrl, end, t);
            Pt tan = quadTangent(start, ctrl, end, t);
            float len = std::max(0.001f, std::hypot(tan.x, tan.y));
            float nx = -tan.y / len, ny = tan.x / len;
            float hw = (startW + (endW - startW) * t) * 0.5f;
            leftEdge.push_back(PointF(p.x + nx * hw, p.y + ny * hw));
            rightEdge.push_back(PointF(p.x - nx * hw, p.y - ny * hw));
        }

        std::vector<PointF> poly = leftEdge;
        for (int i = steps; i >= 0; --i) poly.push_back(rightEdge[i]);

        GraphicsPath limbPath;
        limbPath.AddPolygon(poly.data(), (INT)poly.size());

        float pawR = endW * 0.58f;
        RectF pawRect(end.x - pawR, end.y - pawR * 0.85f, pawR * 2.0f, pawR * 1.7f);
        bool ragdoll = coat == CatCoatTheme::Ragdoll;

        if (coat == CatCoatTheme::ClassicBlack) {
            SolidBrush b(catBlack());
            g.FillPath(&b, &limbPath);
            g.FillEllipse(&b, pawRect);
        } else {
            // Pad gradient endpoints beyond limb geometry + use WrapModeTileFlipXY so GDI+
            // NEVER wraps t < 0 shoulder corners around to t = 1.0 dark espresso!
            float dx = end.x - start.x;
            float dy = end.y - start.y;
            float dist = std::hypot(dx, dy);
            float ux = (dist > 1.0f) ? (dx / dist) : 0.0f;
            float uy = (dist > 1.0f) ? (dy / dist) : 1.0f;
            dist = std::max(1.0f, dist);
            float padStart = startW + 18.0f;
            float padEnd = endW + 18.0f;
            float totalLen = padStart + dist + padEnd;

            PointF gStart(start.x - ux * padStart, start.y - uy * padStart);
            PointF gEnd(end.x + ux * padEnd, end.y + uy * padEnd);

            Color root = ragdoll ? ragdollShade(0) : siameseCream(0);
            Color shoulder = ragdoll ? ragdollShade() : siameseCream();
            Color middle = ragdoll ? ragdollShade() : siameseMocha();
            Color paw = ragdoll ? ragdollIvory() : siamesePoint();
            LinearGradientBrush grad(gStart, gEnd, root, paw);
            grad.SetWrapMode(WrapModeTileFlipXY);
            Color cols[6] = {
                root, root, shoulder, middle, paw, paw
            };
            REAL pos[6] = {
                0.0f,
                padStart / totalLen,
                (padStart + dist * 0.16f) / totalLen,
                (padStart + dist * 0.48f) / totalLen,
                (padStart + dist * 0.78f) / totalLen,
                1.0f
            };
            grad.SetInterpolationColors(cols, pos, 6);
            g.FillPath(&grad, &limbPath);

            SolidBrush pawBrush(paw);
            g.FillEllipse(&pawBrush, pawRect);

            const auto& outerEdge = outlineOnLeft ? rightEdge : leftEdge;
            Pen op(ragdoll ? ragdollOutline(161) : siameseOutline(184), 1.25f);
            op.SetStartCap(LineCapRound); op.SetEndCap(LineCapRound); op.SetLineJoin(LineJoinRound);
            g.DrawLines(&op, outerEdge.data() + 3, (INT)outerEdge.size() - 4);
        }

        const auto& hiEdge = outlineOnLeft ? leftEdge : rightEdge;
        int startIdx = 2;
        int endIdx = (int)((hiEdge.size() - 1) * 0.68f);
        if (endIdx > startIdx) {
            Color sc = contourMint((BYTE)(outlineAlpha * 229));
            if (coat == CatCoatTheme::Siamese) sc = siameseOutline((BYTE)(outlineAlpha * 199));
            if (ragdoll) sc = ragdollOutline((BYTE)(outlineAlpha * 174));
            Pen hp(sc, 1.35f);
            hp.SetStartCap(LineCapRound); hp.SetEndCap(LineCapRound); hp.SetLineJoin(LineJoinRound);
            g.DrawLines(&hp, hiEdge.data() + startIdx, endIdx - startIdx + 1);
        }
    }

    // MARK: - Floating Mechanical Keycaps

    static void drawFloatingKeycaps(Gdiplus::Graphics& g, const CatPhysics& physics, KeyboardTheme theme) {
        using namespace Gdiplus;
        FontFamily ff(L"Segoe UI");
        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);

        for (const auto& b : physics.floatingKeycaps) {
            float prog = b.age / b.lifetime;
            float scale = 1.0f;
            if (b.age < 0.09f) {
                scale = 0.72f + 0.38f * std::sin((b.age / 0.09f) * 3.14159f * 0.65f);
            } else if (b.age < 0.18f) {
                scale = 1.10f - 0.10f * ((b.age - 0.09f) / 0.09f);
            }

            float alphaF = prog < 0.62f ? 1.0f : std::max(0.0f, 1.0f - (prog - 0.62f) / 0.38f);
            BYTE a = (BYTE)(alphaF * 255.0f);
            if (a < 4) continue;

            size_t len = b.label.size();
            float w = len <= 1 ? 23.0f : (len == 2 ? 30.0f : 38.0f);
            float h = 22.0f;

            GraphicsState st = g.Save();
            g.TranslateTransform(b.position.x, b.position.y);
            g.RotateTransform(b.rotation * RadToDeg);
            g.ScaleTransform(scale, scale);

            GraphicsPath basePath;
            addRoundedRect(basePath, -w * 0.5f, -h * 0.5f + 2.5f, w, h, 5.5f);
            bool pink = theme == KeyboardTheme::DaisyPink;
            bool macaron = theme == KeyboardTheme::LuluMacaron;
            BYTE baseAlpha = (BYTE)(alphaF * 242);
            SolidBrush baseBrush(macaron ? Color(baseAlpha, 170, 138, 195) :
                                 pink ? Color(baseAlpha, 199, 91, 138) : Color(baseAlpha, 38, 38, 43));
            g.FillPath(&baseBrush, &basePath);

            GraphicsPath facePath;
            addRoundedRect(facePath, -w * 0.5f, -h * 0.5f, w, h, 5.2f);
            SolidBrush faceBrush(macaron ? Color(a, 237, 219, 255) : pink ? Color(a, 255, 211, 232) : Color(a, 249, 248, 243));
            Pen facePen(macaron ? Color(a, 180, 141, 209) : pink ? Color(a, 215, 107, 154) : Color(a, 41, 41, 46), 1.45f);
            g.FillPath(&faceBrush, &facePath);
            g.DrawPath(&facePen, &facePath);

            Font font(&ff, len <= 1 ? 11.0f : 9.2f, FontStyleBold, UnitPixel);
            SolidBrush txtBrush(Color(a, 31, 31, 36));
            g.DrawString(b.label.c_str(), -1, &font, RectF(-w * 0.5f, -h * 0.5f, w, h), &sf, &txtBrush);

            g.Restore(st);
        }
    }

    // MARK: - Petting Sparkles & Sleep Z

    static void drawSparkles(Gdiplus::Graphics& g, const CatPhysics& physics, EyeColorTheme eyeTheme, bool sleepOnly) {
        using namespace Gdiplus;
        Color baseCol = eyePrimary(eyeTheme);
        FontFamily ff(L"Segoe UI");

        for (const auto& sp : physics.sparkles) {
            if (sleepOnly && !sp.isSleepZ) continue;
            float p = std::min(1.0f, sp.age / sp.lifetime);
            BYTE a = (BYTE)(std::sin(p * 3.14159f) * 215.0f);
            if (a < 4) continue;

            Color c(a, baseCol.GetR(), baseCol.GetG(), baseCol.GetB());
            if (sp.isSleepZ) {
                Font f(&ff, 11.0f + p * 3.0f, FontStyleBold, UnitPixel);
                SolidBrush sb(c);
                g.DrawString(L"z", -1, &f, PointF(sp.position.x, sp.position.y), &sb);
            } else {
                float r = 3.5f + p * 1.5f;
                GraphicsState st = g.Save();
                g.TranslateTransform(sp.position.x, sp.position.y);
                GraphicsPath star;
                addQuadBezier(star, {0, -r}, {0.8f, -0.8f}, {r, 0});
                addQuadBezier(star, {r, 0}, {0.8f, 0.8f}, {0, r});
                addQuadBezier(star, {0, r}, {-0.8f, 0.8f}, {-r, 0});
                addQuadBezier(star, {-r, 0}, {-0.8f, -0.8f}, {0, -r});
                star.CloseFigure();
                SolidBrush sb(c);
                g.FillPath(&sb, &star);
                g.Restore(st);
            }
        }
    }
};
