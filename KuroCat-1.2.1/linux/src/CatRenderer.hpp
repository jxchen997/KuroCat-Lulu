#pragma once
#include <cairo.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include "CatPhysics.hpp"

enum class CatHitZone {
    None,
    Head,
    Body
};

struct RGBA {
    double r, g, b, a;
};

class CatRenderer {
public:
    static CatHitZone hitTest(Pt p, const CatPhysics& physics) {
        float cx = CatPhysics::SeatAnchor.x + physics.bodyShiftX.value;
        float breath = std::sin(physics.time * 2.1f) * 1.6f;
        Pt headCenter = { cx + physics.bodyTilt.value * 36.0f + physics.headYaw.value * 7.5f,
                          126.0f + physics.headOffsetY.value + physics.headPitch.value * 4.8f + breath * 1.1f };

        float hdx = (p.x - headCenter.x) / 48.0f;
        float hdy = (p.y - headCenter.y) / 42.0f;
        if (hdx * hdx + hdy * hdy <= 1.0f) return CatHitZone::Head;

        float bdx = (p.x - cx) / 58.0f;
        float bdy = (p.y - 196.0f) / 56.0f;
        if (bdx * bdx + bdy * bdy <= 1.0f) return CatHitZone::Body;

        if (p.x >= 58.0f && p.x <= 294.0f && p.y >= 232.0f && p.y <= 296.0f) return CatHitZone::Body;
        return CatHitZone::None;
    }

    static void draw(cairo_t* cr, const CatPhysics& physics, const CatSettings& settings) {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

        float breath = std::sin(physics.time * 2.1f) * 1.6f;
        float seatX = CatPhysics::SeatAnchor.x + physics.bodyShiftX.value;
        float seatY = CatPhysics::SeatAnchor.y;

        Pt headCenter = {
            seatX + physics.bodyTilt.value * 36.0f + physics.headYaw.value * 7.5f,
            126.0f + physics.headOffsetY.value + physics.headPitch.value * 4.8f + breath * 1.1f
        };

        drawMouse(cr, physics, settings.keyboardTheme);
        drawKeyboard(cr, physics, settings.keyboardTheme, settings.eyeTheme);
        drawTail(cr, seatX, seatY, physics, settings.coatTheme);
        drawEarsLayer(cr, headCenter, physics, settings.coatTheme);
        drawUnifiedSilhouette(cr, seatX, seatY, headCenter, breath, physics, settings.coatTheme);

        if (physics.hindFootActive.value > 0.03f) {
            drawHindFoot(cr, seatX, seatY, physics, settings.coatTheme);
        }

        drawHeadFeatures(cr, headCenter, physics, settings.eyeTheme, settings.coatTheme);
        drawFrontPaws(cr, seatX, seatY, breath, physics, settings.coatTheme);

        if (settings.showKeyBadges) {
            drawFloatingKeycaps(cr, physics);
        }
        drawSparkles(cr, physics, settings.eyeTheme);
    }

private:
    static constexpr double PI = 3.14159265358979323846;

    static constexpr RGBA catBlack      = { 0.085, 0.085, 0.095, 1.0 };
    static constexpr RGBA contourMint   = { 0.82,  0.92,  0.89,  0.76 };
    static constexpr RGBA whiskerWhite  = { 0.88,  0.90,  0.92,  0.68 };
    static constexpr RGBA siameseCream  = { 0.945, 0.905, 0.835, 1.0 };
    static constexpr RGBA siameseLatte  = { 0.865, 0.795, 0.695, 1.0 };
    static constexpr RGBA siameseMocha  = { 0.45,  0.34,  0.27,  1.0 };
    static constexpr RGBA siamesePoint  = { 0.175, 0.128, 0.105, 1.0 };
    static constexpr RGBA siameseOut    = { 0.26,  0.19,  0.155, 0.88 };
    static constexpr RGBA siameseWhisk  = { 0.36,  0.27,  0.22,  0.68 };

    static void setCol(cairo_t* cr, RGBA c, double alphaMul = 1.0) {
        cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a * alphaMul);
    }

    static RGBA eyePrimary(EyeColorTheme t) {
        switch (t) {
            case EyeColorTheme::NeonChartreuse: return { 0.79, 0.95, 0.25, 1.0 };
            case EyeColorTheme::SiameseBlue:    return { 0.37, 0.78, 0.98, 1.0 };
            case EyeColorTheme::WarmAmber:      return { 0.98, 0.76, 0.22, 1.0 };
            case EyeColorTheme::IceCyan:        return { 0.45, 0.92, 0.96, 1.0 };
        }
        return { 0.79, 0.95, 0.25, 1.0 };
    }

    static RGBA blend(RGBA a, RGBA b, double t) {
        t = std::clamp(t, 0.0, 1.0);
        return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t };
    }

    static void quadCurveTo(cairo_t* cr, double x0, double y0, double cx, double cy, double x2, double y2) {
        double c1x = x0 + (2.0 / 3.0) * (cx - x0);
        double c1y = y0 + (2.0 / 3.0) * (cy - y0);
        double c2x = x2 + (2.0 / 3.0) * (cx - x2);
        double c2y = y2 + (2.0 / 3.0) * (cy - y2);
        cairo_curve_to(cr, c1x, c1y, c2x, c2y, x2, y2);
    }

    static void roundedRect(cairo_t* cr, double x, double y, double w, double h, double r) {
        r = std::min(r, std::min(w * 0.5, h * 0.5));
        cairo_new_sub_path(cr);
        cairo_arc(cr, x + w - r, y + r, r, -PI * 0.5, 0);
        cairo_arc(cr, x + w - r, y + h - r, r, 0, PI * 0.5);
        cairo_arc(cr, x + r, y + h - r, r, PI * 0.5, PI);
        cairo_arc(cr, x + r, y + r, r, PI, PI * 1.5);
        cairo_close_path(cr);
    }

    static void fillEllipse(cairo_t* cr, double cx, double cy, double rx, double ry) {
        cairo_save(cr);
        cairo_translate(cr, cx, cy);
        cairo_scale(cr, rx, ry);
        cairo_arc(cr, 0, 0, 1.0, 0, PI * 2.0);
        cairo_restore(cr);
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

    static void drawMouse(cairo_t* cr, const CatPhysics& physics, KeyboardTheme theme) {
        Pt center = { CatPhysics::MouseBaseCenter.x + physics.mouseOffset.point().x,
                      CatPhysics::MouseBaseCenter.y + physics.mouseOffset.point().y };
        float pressShift = std::max(physics.mousePressLeft.value, physics.mousePressRight.value) * 1.6f;

        cairo_save(cr);
        cairo_translate(cr, center.x, center.y + pressShift);
        cairo_rotate(cr, 0.22);

        // Shadow
        cairo_new_path(cr);
        fillEllipse(cr, 0, 4.0, 13.5, 15.5);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.24);
        cairo_fill(cr);

        RGBA bodyCol, borderCol, wheelCol;
        if (theme == KeyboardTheme::MidnightHacker) {
            bodyCol = {0.20, 0.21, 0.24, 1.0}; borderCol = {0.08, 0.08, 0.10, 1.0}; wheelCol = {0.78, 0.95, 0.25, 1.0};
        } else if (theme == KeyboardTheme::RetroCream) {
            bodyCol = {0.89, 0.87, 0.82, 1.0}; borderCol = {0.20, 0.19, 0.18, 1.0}; wheelCol = {0.88, 0.42, 0.24, 1.0};
        } else {
            bodyCol = {0.955, 0.950, 0.925, 1.0}; borderCol = {0.14, 0.14, 0.16, 1.0}; wheelCol = {0.96, 0.69, 0.16, 1.0};
        }

        cairo_new_path(cr);
        fillEllipse(cr, 0, 0, 13.5, 16.5);
        setCol(cr, bodyCol);
        cairo_fill_preserve(cr);

        if (physics.mousePressLeft.value > 0.05f || physics.mousePressRight.value > 0.05f) {
            cairo_save(cr);
            cairo_clip(cr);
            if (physics.mousePressLeft.value > 0.05f) {
                cairo_set_source_rgba(cr, 0.79, 0.95, 0.25, 0.38 * physics.mousePressLeft.value);
                cairo_rectangle(cr, 0, 0, 14, 17);
                cairo_fill(cr);
            }
            if (physics.mousePressRight.value > 0.05f) {
                cairo_set_source_rgba(cr, 0.79, 0.95, 0.25, 0.38 * physics.mousePressRight.value);
                cairo_rectangle(cr, -14, 0, 14, 17);
                cairo_fill(cr);
            }
            cairo_restore(cr);
        } else {
            cairo_new_path(cr);
        }

        // Front button split seam pointing away from cat (bottom)
        setCol(cr, borderCol, 0.32);
        cairo_set_line_width(cr, 1.05);
        cairo_move_to(cr, 0, 16.0);
        cairo_line_to(cr, 0, 1.5);
        cairo_stroke(cr);

        // Amber scroll wheel
        roundedRect(cr, -1.9, 2.5, 3.8, 8.5, 1.9);
        setCol(cr, wheelCol);
        cairo_fill(cr);

        // Outer border
        cairo_new_path(cr);
        fillEllipse(cr, 0, 0, 13.5, 16.5);
        setCol(cr, borderCol);
        cairo_set_line_width(cr, 1.7);
        cairo_stroke(cr);

        cairo_restore(cr);
    }

    // MARK: - Mini 65% Mechanical Keyboard (180° facing the cat)

    static void drawQuadPath(cairo_t* cr, Pt tl, Pt tr, Pt br, Pt bl) {
        cairo_new_path(cr);
        cairo_move_to(cr, tl.x, tl.y);
        cairo_line_to(cr, tr.x, tr.y);
        cairo_line_to(cr, br.x, br.y);
        cairo_line_to(cr, bl.x, bl.y);
        cairo_close_path(cr);
    }

    static void drawKeyboard(cairo_t* cr, const CatPhysics& physics, KeyboardTheme theme, EyeColorTheme eyeTheme) {
        Pt tl = KeyboardLayout::screenQuadPoint(-0.025f, -0.08f);
        Pt tr = KeyboardLayout::screenQuadPoint(1.025f, -0.08f);
        Pt bl = KeyboardLayout::screenQuadPoint(-0.032f, 1.08f);
        Pt br = KeyboardLayout::screenQuadPoint(1.032f, 1.08f);
        constexpr float depth = 6.5f;

        drawQuadPath(cr, {tl.x, tl.y + depth + 2}, {tr.x, tr.y + depth + 2},
                         {br.x, br.y + depth + 3}, {bl.x, bl.y + depth + 3});
        cairo_set_source_rgba(cr, 0, 0, 0, 0.22);
        cairo_fill(cr);

        RGBA baseCol, deckCol, keyCol, borderCol, accentCol;
        if (theme == KeyboardTheme::MidnightHacker) {
            baseCol = {0.06, 0.06, 0.08, 1}; deckCol = {0.10, 0.11, 0.13, 1};
            keyCol = {0.21, 0.22, 0.25, 1}; borderCol = {0.11, 0.11, 0.13, 0.95}; accentCol = {0.77, 0.94, 0.25, 1};
        } else if (theme == KeyboardTheme::RetroCream) {
            baseCol = {0.25, 0.24, 0.22, 1}; deckCol = {0.38, 0.36, 0.33, 1};
            keyCol = {0.90, 0.88, 0.82, 1}; borderCol = {0.68, 0.65, 0.60, 0.85}; accentCol = {0.88, 0.42, 0.24, 1};
        } else {
            baseCol = {0.10, 0.10, 0.12, 1}; deckCol = {0.14, 0.14, 0.16, 1};
            keyCol = {0.968, 0.964, 0.942, 1}; borderCol = {0.78, 0.77, 0.74, 0.75}; accentCol = {0.97, 0.70, 0.16, 1};
        }

        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        drawQuadPath(cr, {tl.x, tl.y + depth}, {tr.x, tr.y + depth}, {br.x, br.y + depth}, {bl.x, bl.y + depth});
        setCol(cr, baseCol);
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 5.0);
        cairo_stroke(cr);

        drawQuadPath(cr, tl, tr, br, bl);
        setCol(cr, deckCol);
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 4.0);
        cairo_stroke(cr);

        constexpr float padU = 0.0028f;
        constexpr float padV = 0.014f;
        RGBA glowCol = eyePrimary(eyeTheme);

        for (const auto& cap : KeyboardLayout::keycaps()) {
            float glow = 0.0f;
            auto it = physics.keyGlow.find(cap.key);
            if (it != physics.keyGlow.end()) glow = it->second;

            float su0 = (1.0f - (cap.xUnits + cap.widthUnits)) + padU;
            float su1 = (1.0f - cap.xUnits) - padU;
            float sv0 = (1.0f - (cap.row + 1) / 5.0f) + padV;
            float sv1 = (1.0f - cap.row / 5.0f) - padV;

            float dy = glow * 1.9f;
            Pt kTL = KeyboardLayout::screenQuadPoint(su0, sv0); kTL.y += dy;
            Pt kTR = KeyboardLayout::screenQuadPoint(su1, sv0); kTR.y += dy;
            Pt kBR = KeyboardLayout::screenQuadPoint(su1, sv1); kBR.y += dy;
            Pt kBL = KeyboardLayout::screenQuadPoint(su0, sv1); kBL.y += dy;

            drawQuadPath(cr, kTL, kTR, kBR, kBL);
            RGBA fill = cap.isAmberAccent ? accentCol : keyCol;
            if (glow > 0.02f) fill = blend(fill, glowCol, glow * 0.55);

            setCol(cr, fill);
            cairo_fill_preserve(cr);
            setCol(cr, borderCol);
            cairo_set_line_width(cr, 0.7);
            cairo_stroke(cr);
        }

        // Backlit keycap glow halos
        for (const auto& kv : physics.keyGlow) {
            if (kv.second <= 0.05f) continue;
            Pt c = KeyboardLayout::canvasPointForKey(kv.first);
            double r = 17.0 + kv.second * 6.0;
            cairo_pattern_t* pat = cairo_pattern_create_radial(c.x, c.y, 0, c.x, c.y, r);
            cairo_pattern_add_color_stop_rgba(pat, 0.0, glowCol.r, glowCol.g, glowCol.b, 0.36 * kv.second);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, glowCol.r, glowCol.g, glowCol.b, 0.0);
            cairo_set_source(cr, pat);
            cairo_arc(cr, c.x, c.y, r, 0, PI * 2.0);
            cairo_fill(cr);
            cairo_pattern_destroy(pat);
        }
    }

    // MARK: - Tail

    static void drawTail(cairo_t* cr, float seatX, float seatY, const CatPhysics& physics, CatCoatTheme coat) {
        float p = physics.tailPhase;
        float hunt = std::clamp(physics.huntIntensity.value, 0.0f, 1.0f);
        float wag1 = std::sin(p) * (8.0f + hunt * 5.5f);
        float wag2 = std::cos(p * 1.3f) * (6.0f + hunt * 4.5f);
        float huntDrop = hunt * 14.0f;

        cairo_save(cr);
        setCol(cr, coat == CatCoatTheme::Siamese ? siamesePoint : catBlack);
        cairo_set_line_width(cr, 11.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_move_to(cr, seatX + 34.0, seatY - 6.0);
        cairo_curve_to(cr,
            seatX + 64.0, seatY + 2.0 + wag1 * 0.35 + huntDrop * 0.4,
            seatX + 66.0 + wag2 * 0.4, seatY - 26.0 + wag1 * 0.7 + huntDrop,
            seatX + 91.0 + wag2 * 0.5, seatY - 30.0 + wag1 * 0.85 + huntDrop);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    // MARK: - 3D-Projected Ears

    static void drawEarsLayer(cairo_t* cr, Pt center, const CatPhysics& physics, CatCoatTheme coat) {
        cairo_save(cr);
        cairo_translate(cr, center.x, center.y);
        cairo_rotate(cr, physics.headTilt.value);

        float yawRad = physics.headYaw.value * 0.50f;
        float pitchRad = physics.headPitch.value * 0.36f;
        float fold = physics.earFold.value;
        float twitch = physics.earTwitch.value * 0.045f;

        drawEar3D(cr, true, yawRad, pitchRad, fold, twitch, coat);
        drawEar3D(cr, false, yawRad, pitchRad, fold, -twitch, coat);
        cairo_restore(cr);
    }

    static void drawEar3D(cairo_t* cr, bool isLeft, float yawRad, float pitchRad, float fold, float twitch, CatCoatTheme coat) {
        float sign = isLeft ? -1.0f : 1.0f;
        float foldAngle = sign * (0.06f + fold * 0.34f) + twitch;
        float tipDx = std::sin(foldAngle) * 32.0f;
        float tipDy = -std::cos(foldAngle) * 32.0f;

        Pt inB  = projectHead3D(sign * 9.0f, -16.0f, 0.0f, yawRad, pitchRad);
        Pt outB = projectHead3D(sign * 35.0f, -5.0f, -2.0f, yawRad, pitchRad);
        Pt tip  = projectHead3D(sign * 24.5f + tipDx, -20.0f + tipDy, -3.0f, yawRad, pitchRad);
        Pt tipL = { tip.x - sign * 3.8f, tip.y + 2.8f };
        Pt tipR = { tip.x + sign * 3.8f, tip.y + 3.2f };

        cairo_new_path(cr);
        cairo_move_to(cr, inB.x, inB.y);
        quadCurveTo(cr, inB.x, inB.y, (inB.x + tipL.x) * 0.5 - sign * 1.5, (inB.y + tipL.y) * 0.5 - 2.0, tipL.x, tipL.y);
        quadCurveTo(cr, tipL.x, tipL.y, tip.x, tip.y - 2.2, tipR.x, tipR.y);
        quadCurveTo(cr, tipR.x, tipR.y, (outB.x + tipR.x) * 0.5 + sign * 2.5, (outB.y + tipR.y) * 0.5 - 2.0, outB.x, outB.y);
        cairo_close_path(cr);

        setCol(cr, coat == CatCoatTheme::Siamese ? siamesePoint : catBlack);
        cairo_fill(cr);
    }

    // MARK: - Unified C1-Continuous Cat Silhouette (+ Siamese 2.5D Face Mask)

    static void drawUnifiedSilhouette(cairo_t* cr, float seatX, float seatY, Pt headCenter, float breath, const CatPhysics& physics, CatCoatTheme coat) {
        float yaw = physics.headYaw.value;
        float pitch = physics.headPitch.value;
        float hTilt = physics.headTilt.value;
        float bTilt = physics.bodyTilt.value;

        float cH = std::cos(hTilt), sH = std::sin(hTilt);
        auto H = [&](float lx, float ly) -> Pt {
            return { headCenter.x + lx * cH - ly * sH, headCenter.y + lx * sH + ly * cH };
        };

        float cB = std::cos(bTilt), sB = std::sin(bTilt);
        float sy = physics.bodySquash.value;
        float sx = 1.0f + (1.0f - sy) * 0.55f;
        auto B = [&](float lx, float ly) -> Pt {
            float rx = lx * sx, ry = ly * sy;
            return { seatX + rx * cB - ry * sB, seatY + rx * sB + ry * cB };
        };

        float cheekShift = yaw * 3.0f;
        float crownDrop = pitch * 2.0f;
        float lCX = -42.5f + cheekShift * 0.4f;
        float rCX =  42.5f + cheekShift * 0.4f;

        Pt crown  = H(cheekShift * 0.2f, -30.0f + crownDrop);
        Pt rCheek = H(rCX, 5.0f);
        Pt rHip   = B(49.0f, -14.0f);
        Pt rBot   = B(24.0f, 12.0f);
        Pt lBot   = B(-24.0f, 12.0f);
        Pt lHip   = B(-49.0f, -14.0f);
        Pt lCheek = H(lCX, 5.0f);

        auto buildPath = [&]() {
            Pt c1, c2;
            cairo_new_path(cr);
            cairo_move_to(cr, crown.x, crown.y);
            c1 = H(26.0f + cheekShift * 0.3f, -30.0f + crownDrop); c2 = H(rCX + 1.5f, -14.0f);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, rCheek.x, rCheek.y);
            c1 = H(rCX - 2.5f, 24.0f); c2 = B(51.5f, -48.0f + breath * 0.5f);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, rHip.x, rHip.y);
            c1 = B(47.0f, 5.0f); c2 = B(36.0f, 12.0f);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, rBot.x, rBot.y);
            cairo_line_to(cr, lBot.x, lBot.y);
            c1 = B(-36.0f, 12.0f); c2 = B(-47.0f, 5.0f);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, lHip.x, lHip.y);
            c1 = B(-51.5f, -48.0f + breath * 0.5f); c2 = H(lCX + 2.5f, 24.0f);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, lCheek.x, lCheek.y);
            c1 = H(lCX - 1.5f, -14.0f); c2 = H(-26.0f + cheekShift * 0.3f, -30.0f + crownDrop);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, crown.x, crown.y);
            cairo_close_path(cr);
        };

        if (coat == CatCoatTheme::ClassicBlack) {
            buildPath();
            setCol(cr, catBlack);
            cairo_fill(cr);
        } else {
            cairo_save(cr);
            buildPath();
            cairo_clip(cr);

            cairo_pattern_t* bg = cairo_pattern_create_linear(seatX, headCenter.y - 34.0, seatX, seatY + 18.0);
            cairo_pattern_add_color_stop_rgba(bg, 0.15, siameseCream.r, siameseCream.g, siameseCream.b, 1.0);
            cairo_pattern_add_color_stop_rgba(bg, 1.0,  siameseLatte.r, siameseLatte.g, siameseLatte.b, 1.0);
            cairo_set_source(cr, bg);
            cairo_paint(cr);
            cairo_pattern_destroy(bg);

            // 2.5D Rotating Seal-Point "挖煤" Face Mask
            float yawRad = yaw * 0.50f;
            float pitchRad = pitch * 0.36f;
            Pt maskC = projectHead3D(0.0f, 2.5f, 27.0f, yawRad, pitchRad);
            double maskSX = std::max(0.76, std::cos((double)yawRad * 0.7)) * 1.18;
            double maskSY = std::max(0.82, std::cos((double)pitchRad * 0.7)) * 0.88;

            cairo_save(cr);
            cairo_translate(cr, headCenter.x, headCenter.y);
            cairo_rotate(cr, hTilt);
            cairo_translate(cr, maskC.x, maskC.y);
            cairo_scale(cr, maskSX, maskSY);

            cairo_pattern_t* mp = cairo_pattern_create_radial(0, 0, 2.0, 0, 0, 31.0);
            cairo_pattern_add_color_stop_rgba(mp, 0.0,  siamesePoint.r, siamesePoint.g, siamesePoint.b, 1.0);
            cairo_pattern_add_color_stop_rgba(mp, 0.44, siamesePoint.r, siamesePoint.g, siamesePoint.b, 0.96);
            cairo_pattern_add_color_stop_rgba(mp, 0.74, siameseMocha.r, siameseMocha.g, siameseMocha.b, 0.70);
            cairo_pattern_add_color_stop_rgba(mp, 1.0,  siameseMocha.r, siameseMocha.g, siameseMocha.b, 0.0);
            cairo_set_source(cr, mp);
            cairo_arc(cr, 0, 0, 31.0, 0, PI * 2.0);
            cairo_fill(cr);
            cairo_pattern_destroy(mp);

            cairo_restore(cr);
            cairo_restore(cr);

            buildPath();
            setCol(cr, siameseOut);
            cairo_set_line_width(cr, 1.5);
            cairo_stroke(cr);
        }

        // Tucked hind paws at bottom
        cairo_save(cr);
        cairo_translate(cr, seatX, seatY);
        cairo_rotate(cr, bTilt);
        cairo_scale(cr, sx, sy);

        setCol(cr, coat == CatCoatTheme::Siamese ? siamesePoint : catBlack);
        cairo_new_path(cr);
        fillEllipse(cr, -26.5, 8.5, 8.5, 5.5);
        cairo_fill(cr);
        if (physics.hindFootActive.value < 0.35f) {
            cairo_new_path(cr);
            fillEllipse(cr, 26.5, 8.5, 8.5, 5.5);
            cairo_fill(cr);
        }

        setCol(cr, coat == CatCoatTheme::Siamese ? siameseCream : contourMint, 0.56);
        cairo_set_line_width(cr, 1.3);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_new_path(cr);
        cairo_arc(cr, -26.5, 8.5, 7.5, 0.25, PI - 0.25);
        cairo_stroke(cr);
        if (physics.hindFootActive.value < 0.35f) {
            cairo_new_path(cr);
            cairo_arc(cr, 26.5, 8.5, 7.5, 0.25, PI - 0.25);
            cairo_stroke(cr);
        }
        cairo_restore(cr);
    }

    // MARK: - 3rd Key Hind Foot

    static void drawHindFoot(cairo_t* cr, float seatX, float seatY, const CatPhysics& physics, CatCoatTheme coat) {
        Pt hip = { seatX + 26.0f, seatY + 2.0f };
        Pt target = physics.hindFoot.point();
        Pt mid = { (hip.x + target.x) * 0.5f + 10.0f, (hip.y + target.y) * 0.5f - 8.0f };
        drawLimb(cr, hip, mid, target, 15.0f, 12.0f, true, 0.85f * physics.hindFootActive.value, coat);
    }

    // MARK: - 2.5D Eyes & Whiskers

    static void drawHeadFeatures(cairo_t* cr, Pt center, const CatPhysics& physics, EyeColorTheme eyeTheme, CatCoatTheme coat) {
        cairo_save(cr);
        cairo_translate(cr, center.x, center.y);
        cairo_rotate(cr, physics.headTilt.value);

        float yawRad = physics.headYaw.value * 0.50f;
        float pitchRad = physics.headPitch.value * 0.36f;

        drawEye3D(cr, true, yawRad, pitchRad, physics, eyeTheme, coat);
        drawEye3D(cr, false, yawRad, pitchRad, physics, eyeTheme, coat);
        drawWhiskers3D(cr, yawRad, pitchRad, physics, coat);
        cairo_restore(cr);
    }

    static void drawEye3D(cairo_t* cr, bool isLeft, float yawRad, float pitchRad, const CatPhysics& physics, EyeColorTheme eyeTheme, CatCoatTheme coat) {
        float sign = isLeft ? -1.0f : 1.0f;
        RGBA eyeCol = eyePrimary(eyeTheme);
        Pt proj = projectHead3D(sign * 17.2f, -2.0f, 27.5f, yawRad, pitchRad);

        float eyeNormal = yawRad + sign * 0.34f;
        double foreX = std::clamp((double)std::cos(eyeNormal) / std::cos(0.34), 0.64, 1.06);
        double foreY = std::clamp((double)std::cos(pitchRad * 0.85f), 0.78, 1.03);

        cairo_save(cr);
        cairo_translate(cr, proj.x, proj.y);
        cairo_scale(cr, foreX, foreY);

        if (physics.isHappyClosedEyes()) {
            setCol(cr, eyeCol);
            cairo_set_line_width(cr, 2.6);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_move_to(cr, -11.5, 1.0);
            quadCurveTo(cr, -11.5, 1.0, sign * 1.5, 3.4, 11.5, 1.0);
            cairo_stroke(cr);
            cairo_restore(cr);
            return;
        }

        if (physics.isBlissfulSquint()) {
            setCol(cr, eyeCol);
            cairo_new_path(cr);
            cairo_move_to(cr, -11.5, -0.5);
            quadCurveTo(cr, -11.5, -0.5, 0.0, 0.6, 11.5, -0.5);
            quadCurveTo(cr, 11.5, -0.5, 0.0, 4.8, -11.5, -0.5);
            cairo_close_path(cr);
            cairo_fill(cr);
            cairo_restore(cr);
            return;
        }

        float hunt = std::clamp(physics.huntIntensity.value, 0.0f, 1.0f);
        cairo_rotate(cr, sign * (0.06f * (1.0f - hunt * 0.6f)));

        double rx = 12.0 + hunt * 1.1;
        double ry = 10.8 + hunt * 1.8;

        cairo_save(cr);
        cairo_new_path(cr);
        fillEllipse(cr, 0, 0, rx, ry);
        cairo_clip(cr);

        setCol(cr, eyeCol);
        cairo_rectangle(cr, -rx - 1, -ry - 1, rx * 2 + 2, ry * 2 + 2);
        cairo_fill(cr);

        Pt gaze = physics.eyeGaze.point();
        double px = gaze.x * (4.2 - hunt * 1.2);
        double py = gaze.y * (3.6 - hunt * 1.0);
        double pScale = physics.pupilScale.value;
        double normalW = 5.0 * pScale;
        double normalH = 13.5 * std::min(1.12, pScale);
        double pupilW = normalW + (18.8 - normalW) * hunt;
        double pupilH = normalH + (18.8 - normalH) * hunt;

        setCol(cr, catBlack);
        cairo_new_path(cr);
        fillEllipse(cr, px, py, pupilW * 0.5, pupilH * 0.5);
        cairo_fill(cr);

        if (hunt > 0.25f) {
            double ga = std::min(1.0, (hunt - 0.25) / 0.55) * 0.92;
            double gr = 2.1 + hunt * 0.9;
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, ga);
            cairo_arc(cr, px - pupilW * 0.22, py - pupilH * 0.22, gr, 0, PI * 2.0);
            cairo_fill(cr);
        }

        float closure = physics.effectiveEyeClosure();
        if (closure > 0.02f) {
            setCol(cr, coat == CatCoatTheme::Siamese ? siamesePoint : catBlack);
            cairo_rectangle(cr, -rx - 2, -ry - 2, rx * 2 + 4, (ry * 2.2) * closure);
            cairo_fill(cr);
        }

        cairo_restore(cr);
        cairo_restore(cr);
    }

    static void drawWhiskers3D(cairo_t* cr, float yawRad, float pitchRad, const CatPhysics& physics, CatCoatTheme coat) {
        float wiggle = std::sin(physics.time * 4.5f) * (std::max(physics.petHeadIntensity, physics.petBodyIntensity) * 1.2f);
        setCol(cr, coat == CatCoatTheme::Siamese ? siameseWhisk : whiskerWhite);
        cairo_set_line_width(cr, 1.05);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

        const float rows[3][3] = {
            { 10.5f, 36.5f, 8.8f + wiggle },
            { 14.2f, 37.8f, 14.2f + wiggle * 0.5f },
            { 18.0f, 35.5f, 19.6f - wiggle * 0.4f }
        };

        for (float sign : {-1.0f, 1.0f}) {
            for (int i = 0; i < 3; ++i) {
                Pt p0 = projectHead3D(sign * 24.0f, rows[i][0], 23.0f, yawRad, pitchRad);
                Pt p2 = projectHead3D(sign * rows[i][1], rows[i][2], 14.0f, yawRad, pitchRad);
                cairo_move_to(cr, p0.x, p0.y);
                quadCurveTo(cr, p0.x, p0.y, (p0.x + p2.x) * 0.5, (p0.y + p2.y) * 0.5 - 0.8, p2.x, p2.y);
            }
        }
        cairo_stroke(cr);
    }

    // MARK: - Front Paws

    static void drawFrontPaws(cairo_t* cr, float seatX, float seatY, float breath, const CatPhysics& physics, CatCoatTheme coat) {
        Pt lPos = physics.leftPaw.point();
        Pt rPos = physics.rightPaw.point();
        float lDist = std::hypot(lPos.x - CatPhysics::LeftPawRest.x, lPos.y - CatPhysics::LeftPawRest.y);
        float rDist = std::hypot(rPos.x - CatPhysics::RightPawRest.x, rPos.y - CatPhysics::RightPawRest.y);

        Pt lSh = { seatX - 24.0f, seatY - 56.0f + breath };
        Pt rSh = { seatX + 24.0f, seatY - 56.0f + breath };

        if (lDist < 6.0f && !physics.leftPawOnMouse && !physics.leftPawTyping) {
            drawIdleFrontLeg(cr, { seatX - 18.0f, seatY - 30.0f }, { seatX - 16.0f, seatY + 12.0f }, true, coat);
        } else {
            Pt mid = { (lSh.x + lPos.x) * 0.5f - (physics.leftPawOnMouse ? 8.0f : 4.0f), (lSh.y + lPos.y) * 0.5f - 6.0f };
            drawLimb(cr, lSh, mid, lPos, 19.0f, 13.5f, false, 0.78f, coat);
        }

        if (rDist < 6.0f && !physics.rightPawTyping) {
            drawIdleFrontLeg(cr, { seatX + 18.0f, seatY - 30.0f }, { seatX + 16.0f, seatY + 12.0f }, false, coat);
        } else {
            Pt mid = { (rSh.x + rPos.x) * 0.5f + 4.0f, (rSh.y + rPos.y) * 0.5f - 6.0f };
            drawLimb(cr, rSh, mid, rPos, 19.0f, 13.5f, true, 0.78f, coat);
        }
    }

    static void drawIdleFrontLeg(cairo_t* cr, Pt top, Pt bottom, bool isLeft, CatCoatTheme coat) {
        float sign = isLeft ? -1.0f : 1.0f;
        if (coat == CatCoatTheme::Siamese) {
            cairo_save(cr);
            roundedRect(cr, bottom.x - 7.5, top.y, 15.0, bottom.y - top.y + 1.0, 6.5);
            cairo_pattern_t* grad = cairo_pattern_create_linear(top.x, top.y, bottom.x, bottom.y + 2.0);
            cairo_pattern_add_color_stop_rgba(grad, 0.05, siameseCream.r, siameseCream.g, siameseCream.b, 0.0);
            cairo_pattern_add_color_stop_rgba(grad, 0.52, siameseMocha.r, siameseMocha.g, siameseMocha.b, 0.75);
            cairo_pattern_add_color_stop_rgba(grad, 0.85, siamesePoint.r, siamesePoint.g, siamesePoint.b, 1.0);
            cairo_set_source(cr, grad);
            cairo_fill(cr);
            cairo_pattern_destroy(grad);
            cairo_restore(cr);
        }

        setCol(cr, coat == CatCoatTheme::Siamese ? siameseOut : contourMint, coat == CatCoatTheme::Siamese ? 0.65 : 1.0);
        cairo_set_line_width(cr, 1.45);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

        cairo_move_to(cr, top.x + sign * 8.5, top.y);
        quadCurveTo(cr, top.x + sign * 8.5, top.y, top.x + sign * 9.5, (top.y + bottom.y) * 0.5, bottom.x + sign * 7.5, bottom.y - 1.0);
        cairo_move_to(cr, top.x - sign * 7.0, top.y + 4.0);
        quadCurveTo(cr, top.x - sign * 7.0, top.y + 4.0, top.x - sign * 6.5, (top.y + bottom.y) * 0.5, bottom.x - sign * 6.0, bottom.y - 1.0);
        cairo_stroke(cr);
    }

    static void drawLimb(cairo_t* cr, Pt start, Pt ctrl, Pt end, float startW, float endW, bool outlineOnLeft, float outlineAlpha, CatCoatTheme coat) {
        constexpr int steps = 14;
        std::vector<Pt> leftEdge, rightEdge;
        leftEdge.reserve(steps + 1); rightEdge.reserve(steps + 1);

        for (int i = 0; i <= steps; ++i) {
            float t = (float)i / (float)steps;
            Pt p = quadPoint(start, ctrl, end, t);
            Pt tan = quadTangent(start, ctrl, end, t);
            float len = std::max(0.001f, std::hypot(tan.x, tan.y));
            float nx = -tan.y / len, ny = tan.x / len;
            float hw = (startW + (endW - startW) * t) * 0.5f;
            leftEdge.push_back({ p.x + nx * hw, p.y + ny * hw });
            rightEdge.push_back({ p.x - nx * hw, p.y - ny * hw });
        }

        auto traceLimb = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, leftEdge[0].x, leftEdge[0].y);
            for (int i = 1; i <= steps; ++i) cairo_line_to(cr, leftEdge[i].x, leftEdge[i].y);
            for (int i = steps; i >= 0; --i) cairo_line_to(cr, rightEdge[i].x, rightEdge[i].y);
            cairo_close_path(cr);
        };

        double pawR = endW * 0.58;
        if (coat == CatCoatTheme::ClassicBlack) {
            traceLimb();
            setCol(cr, catBlack);
            cairo_fill(cr);
            cairo_new_path(cr);
            fillEllipse(cr, end.x, end.y, pawR, pawR * 0.85);
            cairo_fill(cr);
        } else {
            cairo_save(cr);
            traceLimb();
            cairo_pattern_t* grad = cairo_pattern_create_linear(start.x, start.y, end.x, end.y);
            cairo_pattern_add_color_stop_rgba(grad, 0.0,  siameseCream.r, siameseCream.g, siameseCream.b, 0.0);
            cairo_pattern_add_color_stop_rgba(grad, 0.16, siameseCream.r, siameseCream.g, siameseCream.b, 1.0);
            cairo_pattern_add_color_stop_rgba(grad, 0.48, siameseMocha.r, siameseMocha.g, siameseMocha.b, 1.0);
            cairo_pattern_add_color_stop_rgba(grad, 0.78, siamesePoint.r, siamesePoint.g, siamesePoint.b, 1.0);
            cairo_set_source(cr, grad);
            cairo_fill(cr);
            cairo_pattern_destroy(grad);
            cairo_restore(cr);

            setCol(cr, siamesePoint);
            cairo_new_path(cr);
            fillEllipse(cr, end.x, end.y, pawR, pawR * 0.85);
            cairo_fill(cr);

            const auto& outerEdge = outlineOnLeft ? rightEdge : leftEdge;
            setCol(cr, siameseOut, 0.78);
            cairo_set_line_width(cr, 1.25);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_move_to(cr, outerEdge[3].x, outerEdge[3].y);
            for (int i = 4; i < steps; ++i) cairo_line_to(cr, outerEdge[i].x, outerEdge[i].y);
            cairo_stroke(cr);
        }

        const auto& hiEdge = outlineOnLeft ? leftEdge : rightEdge;
        int sIdx = 2, eIdx = (int)(steps * 0.68f);
        setCol(cr, coat == CatCoatTheme::Siamese ? siameseOut : contourMint, outlineAlpha * 0.9);
        cairo_set_line_width(cr, 1.35);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_move_to(cr, hiEdge[sIdx].x, hiEdge[sIdx].y);
        for (int i = sIdx + 1; i <= eIdx; ++i) cairo_line_to(cr, hiEdge[i].x, hiEdge[i].y);
        cairo_stroke(cr);
    }

    // MARK: - Floating Keycaps

    static void drawFloatingKeycaps(cairo_t* cr, const CatPhysics& physics) {
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

        for (const auto& b : physics.floatingKeycaps) {
            float prog = b.age / b.lifetime;
            double scale = 1.0;
            if (b.age < 0.09f) scale = 0.72 + 0.38 * std::sin((b.age / 0.09) * PI * 0.65);
            else if (b.age < 0.18f) scale = 1.10 - 0.10 * ((b.age - 0.09) / 0.09);

            double alpha = prog < 0.62f ? 1.0 : std::max(0.0, 1.0 - (prog - 0.62) / 0.38);
            if (alpha < 0.02) continue;

            // Estimate display glyph count (UTF-8 multi-byte symbols count as 1 char)
            size_t glyphLen = 0;
            for (unsigned char c : b.label) if ((c & 0xC0) != 0x80) glyphLen++;
            double w = glyphLen <= 1 ? 23.0 : (glyphLen == 2 ? 30.0 : 38.0);
            double h = 22.0;

            cairo_save(cr);
            cairo_translate(cr, b.position.x, b.position.y);
            cairo_rotate(cr, b.rotation);
            cairo_scale(cr, scale, scale);

            roundedRect(cr, -w * 0.5, -h * 0.5 + 2.5, w, h, 5.5);
            cairo_set_source_rgba(cr, 0.15, 0.15, 0.17, 0.95 * alpha);
            cairo_fill(cr);

            roundedRect(cr, -w * 0.5, -h * 0.5, w, h, 5.2);
            cairo_set_source_rgba(cr, 0.975, 0.972, 0.952, alpha);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 0.16, 0.16, 0.18, alpha);
            cairo_set_line_width(cr, 1.45);
            cairo_stroke(cr);

            cairo_set_font_size(cr, glyphLen <= 1 ? 11.5 : 9.8);
            cairo_text_extents_t ext;
            cairo_text_extents(cr, b.label.c_str(), &ext);
            cairo_set_source_rgba(cr, 0.12, 0.12, 0.14, alpha);
            cairo_move_to(cr, -(ext.width * 0.5 + ext.x_bearing), -(ext.height * 0.5 + ext.y_bearing));
            cairo_show_text(cr, b.label.c_str());

            cairo_restore(cr);
        }
    }

    // MARK: - Sparkles & Sleep Z

    static void drawSparkles(cairo_t* cr, const CatPhysics& physics, EyeColorTheme eyeTheme) {
        RGBA c = eyePrimary(eyeTheme);
        for (const auto& sp : physics.sparkles) {
            double p = std::min(1.0, (double)(sp.age / sp.lifetime));
            double alpha = std::sin(p * PI) * 0.85;
            if (alpha < 0.02) continue;

            cairo_save(cr);
            setCol(cr, c, alpha);
            if (sp.isSleepZ) {
                cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 11.0 + p * 3.0);
                cairo_move_to(cr, sp.position.x, sp.position.y);
                cairo_show_text(cr, "z");
            } else {
                double r = 3.5 + p * 1.5;
                cairo_translate(cr, sp.position.x, sp.position.y);
                cairo_move_to(cr, 0, -r);
                quadCurveTo(cr, 0, -r, 0.8, -0.8, r, 0);
                quadCurveTo(cr, r, 0, 0.8, 0.8, 0, r);
                quadCurveTo(cr, 0, r, -0.8, 0.8, -r, 0);
                quadCurveTo(cr, -r, 0, -0.8, -0.8, 0, -r);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            cairo_restore(cr);
        }
    }
};
