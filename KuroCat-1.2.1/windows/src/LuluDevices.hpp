#pragma once

#include <array>
#include "LuluRenderer.hpp"

namespace LuluDevices {
using namespace Gdiplus;

struct Quad {
    PointF tl, tr, br, bl;
};

inline Quad keyboardQuad(float u0, float v0, float u1, float v1, float elevation, bool onDesk) {
    auto project = [&](float u, float v) -> PointF {
        Pt p = KeyboardLayout::screenQuadPoint(u, v);
        if (onDesk) p = KeyboardLayout::onLuluDesk(p);
        return {p.x, p.y + elevation};
    };
    return {project(u0, v0), project(u1, v0), project(u1, v1), project(u0, v1)};
}

inline void fill(Graphics& g, const Quad& q, const Brush& brush) {
    PointF corners[] = {q.tl, q.tr, q.br, q.bl};
    g.FillPolygon(&brush, corners, 4);
}

inline void roundedQuad(GraphicsPath& path, const Quad& quad, float radius) {
    PointF points[] = {quad.tl, quad.tr, quad.br, quad.bl};
    auto toward = [radius](PointF from, PointF to) -> PointF {
        float length = std::hypot(to.X - from.X, to.Y - from.Y);
        float t = std::min(.40f, radius / std::max(0.01f, length));
        return {from.X + (to.X - from.X) * t, from.Y + (to.Y - from.Y) * t};
    };
    for (int i = 0; i < 4; ++i) {
        PointF p = points[i];
        PointF next = toward(p, points[(i + 1) % 4]);
        PointF prev = toward(p, points[(i + 3) % 4]);
        path.AddBezier(prev, p, p, next);
    }
    path.CloseFigure();
}

inline void extrude(Graphics& g, const Quad& upper, const Quad& lower, const Brush& front, const Brush& side) {
    fill(g, {upper.bl, upper.br, lower.br, lower.bl}, front);
    fill(g, {upper.tr, lower.tr, lower.br, upper.br}, side);
}

inline Color mix(Color from, Color to, float t) {
    auto channel = [t](BYTE a, BYTE b) { return static_cast<BYTE>(a + (b - a) * t + 0.5f); };
    return Color(255, channel(from.GetR(), to.GetR()), channel(from.GetG(), to.GetG()), channel(from.GetB(), to.GetB()));
}

inline Color keyColor(const VisualKeycap& key) {
    const Color banana(255, 255, 219, 143), apricot(255, 255, 187, 142), pink(255, 250, 160, 186);
    const Color blue(255, 171, 188, 238), lavender(255, 204, 177, 230);
    const Color mint(255, 155, 223, 189), aqua(255, 142, 220, 222);
    if (key.key == PhysicalKey::Enter || key.key == PhysicalKey::Escape || key.key == PhysicalKey::Backspace) return pink;
    if (key.key == PhysicalKey::Space || key.widthUnits > 1.48f / 15.0f) return blue;
    int column = static_cast<int>(key.xUnits * 15.0f + .5f);
    if (key.row == 0) return column % 4 == 0 ? apricot : banana;
    if (key.row == 4) return column % 3 == 0 ? apricot : lavender;
    if (column <= 1 || column >= 13) return column % 2 == 0 ? blue : lavender;
    return (column + key.row) % 3 == 0 ? mint : aqua;
}

inline void drawKeyboard(Graphics& g, const CatPhysics& physics, bool onDesk) {
    auto quad = [onDesk](float u0, float v0, float u1, float v1, float elevation = 0.0f) {
        return keyboardQuad(u0, v0, u1, v1, elevation, onDesk);
    };
    Quad shadow = quad(-.066f, -.14f, 1.086f, 1.23f, 8.8f);
    SolidBrush shadowBrush(Color(48, 111, 75, 104));
    GraphicsPath shadowShape;
    roundedQuad(shadowShape, shadow, 7.0f);
    g.FillPath(&shadowBrush, &shadowShape);

    Quad rim = quad(-.058f, -.31f, 1.062f, 1.19f);
    Quad sole = quad(-.053f, -.18f, 1.079f, 1.22f, 7.6f);
    LinearGradientBrush front(PointF(0, rim.bl.Y - 1.0f), PointF(0, sole.br.Y + 1.0f),
                              Color(255, 236, 163, 185), Color(255, 193, 117, 153));
    SolidBrush darkerSide(Color(255, 205, 129, 169));
    GraphicsPath bottomShell;
    roundedQuad(bottomShell, sole, 6.0f);
    g.FillPath(&darkerSide, &bottomShell);
    extrude(g, rim, sole, front, darkerSide);
    Pen seam(Color(197, 255, 205, 218), .92f);
    float leftY = LuluArt::mix(rim.bl.Y, sole.bl.Y, .72f);
    float rightY = LuluArt::mix(rim.br.Y, sole.br.Y, .72f);
    g.DrawLine(&seam, rim.bl.X + 2.0f, leftY, rim.br.X - 1.0f, rightY);

    LinearGradientBrush shell(PointF(0, rim.tl.Y - 1.0f), PointF(0, rim.br.Y + 1.0f),
                              Color(255, 255, 226, 221), Color(255, 244, 174, 199));
    GraphicsPath topShell;
    roundedQuad(topShell, rim, 6.2f);
    g.FillPath(&shell, &topShell);
    Pen bevel(Color(222, 255, 241, 230), 1.34f);
    bevel.SetLineJoin(LineJoinRound);
    g.DrawPath(&bevel, &topShell);
    Quad recess = quad(-.017f, -.051f, 1.017f, 1.055f, 1.2f);
    SolidBrush inset(Color(255, 198, 129, 162));
    GraphicsPath keywell;
    roundedQuad(keywell, recess, 2.0f);
    g.FillPath(&inset, &keywell);
    Pen rimLight(Color(212, 255, 214, 220), .75f);
    g.DrawLine(&rimLight, recess.bl, recess.br);

    for (auto slot : {std::array<float,3>{.07f, .26f, .0f}, {.34f, .66f, 1.0f}, {.73f, .91f, .0f}}) {
        Quad trim = quad(slot[0], -.235f, slot[1], -.135f, -.2f);
        GraphicsPath pill;
        roundedQuad(pill, trim, 2.4f);
        SolidBrush shade(slot[2] > .5f ? Color(255, 239, 152, 176) : Color(255, 250, 195, 196));
        g.FillPath(&shade, &pill);
        Pen gloss(Color(175, 255, 235, 228), .60f);
        g.DrawPath(&gloss, &pill);
    }

    const auto& keycaps = KeyboardLayout::keycaps();
    for (auto it = keycaps.rbegin(); it != keycaps.rend(); ++it) {
        const VisualKeycap& key = *it;
        float glow = 0.0f;
        auto found = physics.keyGlow.find(key.key);
        if (found != physics.keyGlow.end()) glow = found->second;
        float travel = std::max(physics.leftStrike.keyTravel(key.key), physics.rightStrike.keyTravel(key.key));
        if (key.key != physics.leftStrike.key && key.key != physics.rightStrike.key) travel = 0.68f * glow;

        constexpr float uPad = .0040f;
        constexpr float vPad = .021f;
        float us = 1.0f - key.xUnits - key.widthUnits;
        float ue = 1.0f - key.xUnits;
        float vs = 1.0f - (key.row + 1) / 5.0f;
        float ve = 1.0f - key.row / 5.0f;
        constexpr float raised = -1.9f;
        float topY = LuluArt::mix(raised, PetKeystroke::ContactOffset, travel);
        Quad cap = quad(us + uPad * 1.4f, vs + vPad * 1.1f, ue - uPad * 1.4f, ve - vPad * 1.3f, topY);
        Quad bottom = quad(us + uPad * .65f, vs + vPad * .78f, ue - uPad * .65f, ve - vPad * .54f, 2.45f);
        Color face = keyColor(key);
        Color frontColor = mix(face, Color(255, 126, 113, 161), .38f);
        Color sideColor = mix(face, Color(255, 105, 101, 156), .52f);
        face = mix(face, Color(255, 255, 249, 206), std::clamp(glow * .32f + travel * .51f, 0.0f, 1.0f));
        SolidBrush sideBrush(sideColor), frontBrush(frontColor);
        extrude(g, cap, bottom, frontBrush, sideBrush);
        LinearGradientBrush top(PointF(0, cap.tl.Y - .2f), PointF(0, cap.br.Y + .2f),
                                mix(face, Color(255, 255, 255, 245), .49f), face);
        GraphicsPath topCap;
        roundedQuad(topCap, cap, 1.15f);
        g.FillPath(&top, &topCap);
        Pen edge(mix(face, Color(255, 255, 255, 245), .57f), 0.54f);
        g.DrawPath(&edge, &topCap);
        if (key.key == PhysicalKey::Space) {
            Pen marking(Color(183, 130, 123, 184), .60f);
            Pt p = KeyboardLayout::canvasPointForKey(key.key);
            if (onDesk) p = KeyboardLayout::onLuluDesk(p);
            g.DrawLine(&marking, p.x - 4.0f, p.y + topY + .3f, p.x + 4.0f, p.y + topY + .7f);
        }
    }

    for (const PetKeystroke* strike : {&physics.leftStrike, &physics.rightStrike}) {
        if (strike->key == PhysicalKey::None || !strike->onKey()) continue;
        Pt p = strike->keyCenter;
        LuluArt::oval(g, {p.x - 12.5f, p.y - 8.2f, 25.0f, 17.6f},
                      Color(134, 255, 238, 202), Color(0, 255, 210, 177), .50f, .54f);
    }

    Pt status = KeyboardLayout::screenQuadPoint(.953f, -.127f);
    if (onDesk) status = KeyboardLayout::onLuluDesk(status);
    LuluArt::oval(g, {status.x - 2.1f, status.y - 1.1f, 4.2f, 2.2f},
                  Color(255, 217, 235, 138), Color(255, 117, 151, 66));
}

inline void drawMouse(Graphics& g, const CatPhysics& physics, bool onDesk) {
    Pt offset = physics.mouseOffset.point();
    Pt center = {CatPhysics::MouseBaseCenter.x + offset.x, CatPhysics::MouseBaseCenter.y + offset.y};
    if (onDesk) center = KeyboardLayout::onLuluDesk(center);
    GraphicsState saved = g.Save();
    g.TranslateTransform(center.x, center.y);
    g.RotateTransform(12.0f);
    if (onDesk) g.ScaleTransform(.85f, .85f);
    LuluArt::oval(g, {-16, 0, 32, 19}, Color(58, 96, 79, 106), Color(0, 96, 79, 106));
    LuluArt::oval(g, {-14, -13, 28, 35}, Color(255, 229, 190, 217), Color(255, 181, 144, 181));
    LuluArt::oval(g, {-14.1f, -17.0f, 28.2f, 34.5f}, Color(255, 221, 253, 226), Color(255, 137, 194, 180), .42f, .35f);
    GraphicsPath surface;
    surface.AddEllipse(-14.1f, -17.0f, 28.2f, 34.5f);
    GraphicsState clip = g.Save();
    g.SetClip(&surface);
    auto press = [&](float x, float strength) {
        if (strength < .02f) return;
        RectF area(x, -4.0f, 14.2f, 23.0f);
        LinearGradientBrush button(area, Color(35, 252, 222, 223), Color(static_cast<BYTE>(strength * 165.0f), 233, 162, 199), LinearGradientModeVertical);
        g.FillRectangle(&button, area);
    };
    press(0.0f, physics.mousePressLeft.value);
    press(-14.2f, physics.mousePressRight.value);
    g.Restore(clip);
    Pen groove(Color(180, 108, 150, 147), .82f);
    g.DrawBezier(&groove, -13.4f, .8f, -6.0f, -1.2f, 6.0f, -1.2f, 13.4f, .8f);
    g.DrawLine(&groove, 0.0f, 5.5f, 0.0f, 16.6f);
    LuluArt::oval(g, {-2.5f, -1.6f, 5.0f, 10.2f}, Color(255, 234, 212, 250), Color(255, 167, 136, 196));
    Pen grip(Color(155, 250, 228, 251), .65f);
    for (float y : {1.7f, 3.3f, 4.9f}) g.DrawLine(&grip, -1.45f, y, 1.45f, y);
    g.Restore(saved);
}
}  // namespace LuluDevices
