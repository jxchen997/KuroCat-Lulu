#pragma once

#include <gdiplus.h>
#include "CatPhysics.hpp"
#include "LuluReferenceArt.hpp"

namespace LuluArt {
using namespace Gdiplus;

inline void volume(Graphics& g, GraphicsPath& path, PointF light, Color bright, Color shade,
                   const REAL focus = 0.13f) {
    PathGradientBrush brush(&path);
    brush.SetCenterPoint(light);
    brush.SetCenterColor(bright);
    INT count = 1;
    brush.SetSurroundColors(&shade, &count);
    brush.SetFocusScales(focus, focus);
    g.FillPath(&brush, &path);
}

inline void oval(Graphics& g, RectF bounds, Color bright, Color shade, float lightX = 0.42f, float lightY = 0.34f) {
    GraphicsPath path;
    path.AddEllipse(bounds);
    volume(g, path, {bounds.X + bounds.Width * lightX, bounds.Y + bounds.Height * lightY}, bright, shade);
}

inline float mix(float a, float b, float t) { return a + (b - a) * t; }

struct Pose {
    float x;
    float headY;
    float seated;
    float floor;
    float yaw;
    float pitch;
    float content;
    float impact;
};

inline Pose poseFor(const CatPhysics& physics) {
    float seated = std::clamp(physics.typingSeat.value, 0.0f, 1.0f);
    float content = std::max(physics.petHeadIntensity, physics.petBodyIntensity * 0.82f);
    float impact = std::max(physics.leftStrike.keyTravel(physics.leftStrike.key),
                            physics.rightStrike.keyTravel(physics.rightStrike.key));
    float yaw = std::clamp(physics.headYaw.value, -1.0f, 1.0f);
    float pitch = std::clamp(physics.headPitch.value, -1.0f, 1.0f);
    return {184.0f + physics.bodyShiftX.value * 0.46f,
            112.0f + 24.0f * seated + pitch * 3.6f + impact * 1.25f - content * 4.2f + std::sin(physics.time * 2.3f) * 1.1f,
            seated, 238.0f, yaw, pitch, content, impact};
}

inline CatHitZone hitTest(Pt point, const CatPhysics& physics) {
    Pose pose = poseFor(physics);
    if (LuluReference::available()) return LuluReference::hitTest(point, physics);
    float x = point.x - pose.x;
    float y = point.y - pose.headY;
    if (x * x / 3940.0f + y * y / 2320.0f <= 1.0f ||
        (std::abs(x) < 20.0f && y < -42.0f && y > -69.0f)) return CatHitZone::Head;
    float bodyY = point.y - mix(194.0f, 210.0f, pose.seated);
    float radius = mix(43.0f, 31.0f, pose.seated);
    if (x * x / 3200.0f + bodyY * bodyY / (radius * radius) <= 1.0f) return CatHitZone::Body;
    return CatHitZone::None;
}

inline void drawBody(Graphics& g, const Pose& pose) {
    float top = mix(143.0f, 166.0f, pose.seated);
    float waist = mix(207.0f, 216.0f, pose.seated);
    float hips = mix(46.0f, 54.0f, pose.seated);
    for (float side : {-1.0f, 1.0f}) {
        float foot = pose.x + side * mix(24.0f, 36.0f, pose.seated);
        oval(g, {foot - 12.0f, pose.floor - 15.0f, 24.0f, 15.0f},
             Color(255, 255, 228, 132), Color(255, 213, 156, 56), 0.49f, 0.58f);
    }

    GraphicsPath shirt;
    shirt.AddBezier(pose.x - 31, top, pose.x - 44, top + 14, pose.x - hips, waist - 18, pose.x - hips + 2, waist);
    shirt.AddBezier(pose.x - hips + 2, waist, pose.x - 30, waist + 16, pose.x + 35, waist + 14, pose.x + hips - 2, waist);
    shirt.AddBezier(pose.x + hips - 2, waist, pose.x + hips, waist - 17, pose.x + 41, top + 10, pose.x + 30, top);
    shirt.CloseFigure();
    volume(g, shirt, {pose.x + 13.0f, top + 42.0f}, Color(255, 255, 245, 137), Color(255, 227, 175, 68), 0.19f);

    float bottom = pose.floor - mix(8.0f, 5.0f, pose.seated);
    GraphicsPath shorts;
    shorts.AddBezier(pose.x - hips + 1, waist - 9, pose.x - 27, waist - 2, pose.x + 23, waist - 2, pose.x + hips - 1, waist - 9);
    shorts.AddBezier(pose.x + hips - 1, waist - 9, pose.x + hips + 2, waist - 1, pose.x + hips - 1, bottom - 1, pose.x + hips - 12, bottom);
    shorts.AddBezier(pose.x + hips - 12, bottom, pose.x + 26, bottom + 3, pose.x + 10, bottom + 3, pose.x + 8 - pose.seated * 3, bottom - 5);
    shorts.AddBezier(pose.x + 8 - pose.seated * 3, bottom - 5, pose.x + 5, bottom - 10, pose.x - 6, bottom - 10, pose.x - 9 + pose.seated * 2, bottom - 5);
    shorts.AddBezier(pose.x - 9 + pose.seated * 2, bottom - 5, pose.x - 13, bottom + 3, pose.x - 30, bottom + 3, pose.x - hips + 11, bottom);
    shorts.AddBezier(pose.x - hips + 11, bottom, pose.x - hips + 1, bottom - 2, pose.x - hips - 4, waist - 1, pose.x - hips + 1, waist - 9);
    shorts.CloseFigure();
    volume(g, shorts, {pose.x + 10, waist - 2}, Color(255, 252, 170, 89), Color(255, 191, 94, 38), 0.22f);
    Pen seam(Color(115, 255, 205, 132), 0.85f);
    g.DrawBezier(&seam, pose.x - hips + 8, waist - 4, pose.x - 25, waist + 2, pose.x + 27, waist + 2, pose.x + hips - 8, waist - 4);
}

inline void drawArm(Graphics& g, const Pose& pose, const PetKeystroke& strike, float side) {
    Pt root = {pose.x + side * 33.0f, mix(170.0f, 184.0f, pose.seated)};
    Pt hand = {mix(pose.x + side * (49.0f + pose.content * 8.0f), strike.tip.x, pose.seated),
               mix(216.0f - pose.content * 7.0f, strike.tip.y, pose.seated)};
    Pt middle = {mix(root.x, hand.x, 0.46f) + side * mix(12.0f, 7.0f, pose.seated), mix(root.y, hand.y, 0.44f)};
    float dx = hand.x - root.x;
    float dy = hand.y - root.y;
    float length = std::max(1.0f, std::hypot(dx, dy));
    Pt normal = {-dy / length, dx / length};
    float flatten = strike.onKey() ? pose.seated * 2.2f : 0.0f;
    Pt end = {hand.x, hand.y - 6.1f + flatten};
    float tipWidth = 7.9f + flatten * 0.7f;
    auto edge = [&](Pt p, float spread) -> PointF { return {p.x + spread * normal.x, p.y + spread * normal.y}; };
    if (strike.onKey() && pose.seated > 0.65f) {
        SolidBrush shadow(Color(75, 136, 67, 18));
        g.FillEllipse(&shadow, hand.x - 9.7f, hand.y - 1.5f, 19.4f, 4.2f);
    }
    GraphicsPath arm;
    arm.AddBezier(edge(root, 9.2f), edge(middle, 11.5f), edge(end, tipWidth + 1.0f), edge(end, tipWidth));
    arm.AddBezier(edge(end, tipWidth), {hand.x - tipWidth * 0.88f, hand.y + 2.0f},
                  {hand.x + tipWidth * 0.85f, hand.y + 2.0f}, edge(end, -tipWidth));
    arm.AddBezier(edge(end, -tipWidth), edge(end, -tipWidth - 1.0f), edge(middle, -9.8f), edge(root, -8.0f));
    PointF shoulderInside = edge(root, -8.0f);
    PointF shoulderOutside = edge(root, 9.2f);
    arm.AddBezier(shoulderInside, {shoulderInside.X, shoulderInside.Y - 11.5f},
                  {shoulderOutside.X, shoulderOutside.Y - 10.5f}, shoulderOutside);
    arm.CloseFigure();
    volume(g, arm, {mix(root.x, hand.x, 0.57f) - 4.0f, mix(root.y, hand.y, 0.57f)},
           Color(255, 255, 245, 139), Color(255, 216, 155, 53), 0.20f);
    Pen crease(Color(110, 177, 116, 38), 0.75f);
    crease.SetStartCap(LineCapRound);
    crease.SetEndCap(LineCapRound);
    g.DrawBezier(&crease, hand.x - 4.3f, hand.y - 5.0f, hand.x - 3.4f, hand.y - 5.4f,
                 hand.x - 2.4f, hand.y - 3.6f, hand.x - 2.3f, hand.y - 2.2f);
}

inline void drawFruit(Graphics& g, float x, float bounce) {
    GraphicsPath fruit;
    fruit.AddBezier(x, -61 + bounce, x - 23, -61 + bounce, x - 24, -41 + bounce, x - 13, -35 + bounce);
    fruit.AddBezier(x - 13, -35 + bounce, x - 4, -31 + bounce, x + 19, -34 + bounce, x + 20, -46 + bounce);
    fruit.AddBezier(x + 20, -46 + bounce, x + 21, -56 + bounce, x + 11, -61 + bounce, x, -61 + bounce);
    fruit.CloseFigure();
    volume(g, fruit, {x - 4, -53 + bounce}, Color(255, 255, 216, 75), Color(255, 237, 132, 34), 0.25f);
    GraphicsPath leaf;
    leaf.AddBezier(x - 0.8f, -60 + bounce, x - 3, -70 + bounce, x + 4, -73 + bounce, x + 5.0f, -66 + bounce);
    leaf.AddBezier(x + 5.0f, -66 + bounce, x + 5.0f, -63 + bounce, x + 1.7f, -62 + bounce, x - 0.8f, -60 + bounce);
    leaf.CloseFigure();
    volume(g, leaf, {x + 2, -67 + bounce}, Color(255, 171, 198, 99), Color(255, 94, 133, 59));
}

inline void drawEye(Graphics& g, const Pose& pose, const CatPhysics& physics, float side) {
    float x = side * 26.0f + pose.yaw * 9.2f;
    float y = -17.0f + pose.pitch * 3.0f;
    float closure = std::max(physics.effectiveEyeClosure(), pose.content);
    float w = 17.9f * (1.0f - std::max(0.0f, -side * pose.yaw) * 0.17f);
    if (closure > 0.59f) {
        Pen lid(Color(219, 151, 94, 35), 1.45f);
        lid.SetStartCap(LineCapRound);
        lid.SetEndCap(LineCapRound);
        g.DrawBezier(&lid, x - w * 0.49f, y + 3.0f, x - w * 0.18f, y - 4.8f,
                     x + w * 0.21f, y - 4.1f, x + w * 0.49f, y + 1.7f);
        return;
    }
    float h = 16.4f * (1.0f - closure * 0.73f);
    oval(g, {x - w * .51f, y - h * .54f, w * 1.03f, h * 1.08f}, Color(255, 244, 207, 98), Color(255, 188, 132, 62));
    oval(g, {x - w * .48f, y - h * .49f, w * .96f, h}, Color(255, 255, 255, 250), Color(255, 220, 226, 232));
    Pt look = physics.eyeGaze.point();
    float iris = std::min(w * .71f, h * .86f);
    float cx = x - side * 1.55f + look.x * w * .12f;
    float cy = y + look.y * h * .14f + .55f;
    oval(g, {cx - iris * .5f, cy - iris * .5f, iris, iris}, Color(255, 161, 199, 249), Color(255, 49, 73, 140));
    SolidBrush pupil(Color(255, 20, 24, 39));
    g.FillEllipse(&pupil, cx - iris * .39f, cy - iris * .37f, iris * .78f, iris * .78f);
    SolidBrush light(Color(255, 255, 254, 245));
    g.FillEllipse(&light, cx - iris * .20f, cy - iris * .29f, iris * .28f, iris * .26f);
    g.FillEllipse(&light, cx + iris * .18f, cy + iris * .17f, iris * .10f, iris * .10f);
}

inline void drawHead(Graphics& g, const Pose& pose, const CatPhysics& physics) {
    GraphicsState saved = g.Save();
    g.TranslateTransform(pose.x + pose.yaw * 3.8f, pose.headY);
    g.RotateTransform(std::clamp(physics.headTilt.value, -.18f, .18f) * 38.0f + pose.content * 2.4f);
    drawFruit(g, pose.yaw * 4.2f - 1.5f, std::sin(physics.time * 2.5f) * 0.75f - pose.impact * 1.1f);
    for (float side : {-1.0f, 1.0f}) {
        GraphicsState ear = g.Save();
        g.TranslateTransform(side * 45.0f, -27.0f);
        g.RotateTransform(side * 35.0f);
        oval(g, {-9, -13, 18, 29}, Color(255, 255, 246, 139), Color(255, 231, 176, 61));
        oval(g, {-4.9f, -7.5f, 10.0f, 18.1f}, Color(255, 251, 214, 109), Color(255, 220, 157, 61));
        g.Restore(ear);
    }

    GraphicsPath head;
    head.AddBezier(0, -42, -24, -43, -43, -31, -51, -12);
    head.AddBezier(-51, -12, -79, 22, -62, 44, -31, 49);
    head.AddBezier(-31, 49, -11, 55, 21, 54, 40, 46);
    head.AddBezier(40, 46, 66, 38, 75, 20, 55, -9);
    head.AddBezier(55, -9, 40, -34, 28, -43, 0, -42);
    head.CloseFigure();
    volume(g, head, {-10.0f, -20.0f}, Color(255, 255, 252, 154), Color(255, 235, 173, 60), 0.30f);

    GraphicsState clip = g.Save();
    g.SetClip(&head);
    float muzzleX = pose.yaw * 6.7f;
    float muzzleY = 19.0f - 2.8f * pose.content + 1.3f * pose.pitch;
    oval(g, {muzzleX - 64.0f, muzzleY - 32.0f, 128.0f, 72.0f},
         Color(240, 248, 148, 42), Color(0, 249, 179, 53), 0.51f, 0.69f);
    oval(g, {muzzleX - 54.0f, muzzleY - 25.0f, 110.0f, 54.0f},
         Color(187, 254, 177, 59), Color(0, 255, 168, 46), 0.40f, 0.29f);
    g.Restore(clip);

    drawEye(g, pose, physics, -1.0f);
    drawEye(g, pose, physics, 1.0f);
    Pen smile(Color(245, 111, 63, 29), 1.30f);
    smile.SetStartCap(LineCapRound);
    smile.SetEndCap(LineCapRound);
    float mouth = 30.0f - 3.2f * pose.content;
    g.DrawBezier(&smile, muzzleX - 15.4f, mouth - 1.8f, muzzleX - 9.0f, mouth + 5.6f,
                 muzzleX + 8.8f, mouth + 5.8f, muzzleX + 16.0f, mouth - 2.5f - pose.content * 2.1f);
    g.Restore(saved);
}

inline void drawFlowers(Graphics& g, const CatPhysics& physics) {
    constexpr float pi = 3.14159265f;
    for (const PurrSparkle& sparkle : physics.sparkles) {
        if (sparkle.isSleepZ) continue;
        float time = std::clamp(sparkle.age / sparkle.lifetime, 0.0f, 1.0f);
        BYTE alpha = static_cast<BYTE>(std::sin(pi * time) * 228.0f);
        float radius = 3.1f + 0.9f * time;
        SolidBrush petal(Color(alpha, 255, 233, 151));
        for (int i = 0; i < 5; ++i) {
            float angle = i * (2.0f * pi / 5.0f) + time;
            g.FillEllipse(&petal, sparkle.position.x + std::cos(angle) * radius - 1.9f,
                          sparkle.position.y + std::sin(angle) * radius - 1.9f, 3.8f, 3.8f);
        }
        SolidBrush center(Color(alpha, 224, 144, 52));
        g.FillEllipse(&center, sparkle.position.x - 1.4f, sparkle.position.y - 1.4f, 2.8f, 2.8f);
    }
}

inline void draw(Graphics& g, const CatPhysics& physics) {
    Pose pose = poseFor(physics);
    drawBody(g, pose);
    drawArm(g, pose, physics.leftStrike, -1.0f);
    drawArm(g, pose, physics.rightStrike, 1.0f);
    drawHead(g, pose, physics);
    drawFlowers(g, physics);
}
}  // namespace LuluArt
