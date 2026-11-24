#pragma once

#include <array>
#include "LuluRenderer.hpp"

namespace LuluOffice {
using namespace Gdiplus;

inline void rounded(GraphicsPath& path, RectF r, float radius) {
    float d = radius * 2.0f;
    path.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
    path.AddArc(r.GetRight() - d, r.Y, d, d, 270.0f, 90.0f);
    path.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0.0f, 90.0f);
    path.AddArc(r.X, r.GetBottom() - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

inline void gradient(Graphics& g, GraphicsPath& path, float top, float bottom, Color light, Color shade) {
    LinearGradientBrush fill(PointF(0.0f, top), PointF(0.0f, bottom), light, shade);
    g.FillPath(&fill, &path);
}

inline void panel(Graphics& g, RectF bounds, float radius, Color light, Color shade) {
    GraphicsPath path;
    rounded(path, bounds, radius);
    gradient(g, path, bounds.Y - 1.0f, bounds.GetBottom() + 1.0f, light, shade);
}

struct ChairView {
    static constexpr float Elevation = 0.30f;
    static constexpr float ElevationCos = 0.9539392f;
    static constexpr float FocalLength = 415.0f;
    float sine;
    float cosine;

    explicit ChairView(float angle) : sine(std::sin(angle)), cosine(std::cos(angle)) {}

    PointF project(float x, float y, float z) const {
        float right = x * cosine + z * sine;
        float forward = z * cosine - x * sine;
        float depth = forward * ElevationCos - y * Elevation;
        float perspective = FocalLength / (FocalLength - depth);
        return {165.0f + right * perspective, 240.0f + (y * ElevationCos + forward * Elevation) * perspective};
    }
};

template <size_t Size>
using Ring = std::array<PointF, Size>;

template <size_t Size>
inline void chairFace(GraphicsPath& path, const Ring<Size>& ring) {
    path.AddPolygon(ring.data(), static_cast<INT>(ring.size()));
}

template <size_t Size>
inline void upholsterySilhouette(GraphicsPath& path, const Ring<Size>& front, const Ring<Size>& back) {
    std::array<PointF, Size * 2> points;
    std::copy(front.begin(), front.end(), points.begin());
    std::copy(back.begin(), back.end(), points.begin() + Size);
    std::sort(points.begin(), points.end(), [](PointF a, PointF b) {
        return a.X < b.X || (a.X == b.X && a.Y < b.Y);
    });
    std::array<PointF, Size * 4> hull;
    size_t count = 0;
    auto add = [&](PointF point, size_t limit) {
        while (count > limit + 1) {
            PointF a = hull[count - 2], b = hull[count - 1];
            if ((b.X - a.X) * (point.Y - a.Y) - (b.Y - a.Y) * (point.X - a.X) > 0.0f) break;
            --count;
        }
        hull[count++] = point;
    };
    for (PointF point : points) add(point, 0);
    const size_t lower = count - 1;
    for (auto point = points.rbegin() + 1; point != points.rend(); ++point) add(*point, lower);
    path.AddPolygon(hull.data(), static_cast<INT>(count - 1));
}

inline Ring<32> backrestRing(const ChairView& view, float halfWidth, float halfHeight, float radius, float depth) {
    Ring<32> ring;
    for (size_t i = 0; i < ring.size(); ++i) {
        int corner = static_cast<int>(i / 8);
        float angle = (corner + static_cast<float>(i % 8) / 7.0f) * 1.5707963f;
        float x = (corner == 0 || corner == 3 ? 1.0f : -1.0f) * (halfWidth - radius) + radius * std::cos(angle);
        float y = (corner < 2 ? 1.0f : -1.0f) * (halfHeight - radius) + radius * std::sin(angle);
        ring[i] = view.project(x, y - 44.0f, depth + y * 0.11f);
    }
    return ring;
}

inline Ring<48> cushionRing(const ChairView& view, float halfWidth, float halfDepth, float height) {
    Ring<48> ring;
    for (size_t i = 0; i < ring.size(); ++i) {
        float angle = static_cast<float>(i) * 6.2831853f / ring.size();
        ring[i] = view.project(halfWidth * std::cos(angle), height, halfDepth * std::sin(angle) + 5.0f);
    }
    return ring;
}

inline void chairUpholstery(Graphics& g, float swivel) {
    ChairView view(std::clamp(swivel, -1.0f, 1.0f) * 0.72f);

    PointF bracket = view.project(0.0f, -10.0f, -46.0f);
    PointF mount = view.project(0.0f, 1.0f, 0.0f);
    Pen support(Color(255, 185, 144, 175), 9.0f);
    support.SetStartCap(LineCapRound);
    support.SetEndCap(LineCapRound);
    g.DrawLine(&support, bracket, mount);

    auto rim = backrestRing(view, 64.0f, 49.0f, 23.0f, -36.0f);
    auto shell = backrestRing(view, 61.0f, 47.0f, 22.0f, -56.0f);
    GraphicsPath backrest;
    upholsterySilhouette(backrest, rim, shell);
    LuluArt::volume(g, backrest, view.project(-22.0f, -72.0f, -46.0f),
                    Color(255, 238, 166, 206), Color(255, 170, 106, 164), 0.21f);

    GraphicsPath pad;
    chairFace(pad, rim);
    LuluArt::volume(g, pad, view.project(-25.0f, -71.0f, -34.0f),
                    Color(255, 255, 219, 234), Color(255, 219, 143, 198), 0.18f);
    Pen piping(Color(209, 255, 227, 240), 1.35f);
    piping.SetLineJoin(LineJoinRound);
    g.DrawPath(&piping, &pad);
    GraphicsPath stitching;
    chairFace(stitching, backrestRing(view, 58.0f, 43.0f, 18.0f, -35.0f));
    Pen seam(Color(151, 201, 128, 182), 0.65f);
    g.DrawPath(&seam, &stitching);

    auto top = cushionRing(view, 58.0f, 39.0f, -8.0f);
    auto underside = cushionRing(view, 55.5f, 37.0f, 3.0f);
    GraphicsPath cushion;
    upholsterySilhouette(cushion, top, underside);
    LuluArt::volume(g, cushion, view.project(-26.0f, -1.0f, 28.0f),
                    Color(255, 199, 237, 216), Color(255, 139, 183, 170), 0.23f);
    GraphicsPath sittingSurface;
    chairFace(sittingSurface, top);
    LuluArt::volume(g, sittingSurface, view.project(-24.0f, -8.0f, -5.0f),
                    Color(255, 232, 251, 225), Color(255, 169, 218, 187), 0.18f);
    Pen cushionRim(Color(182, 218, 247, 226), 0.9f);
    g.DrawPath(&cushionRim, &sittingSurface);
}

inline void chair(Graphics& g, float swivel) {
    LuluArt::oval(g, {96.0f, 283.0f, 137.0f, 24.0f}, Color(69, 90, 76, 109), Color(0, 90, 76, 109));
    for (PointF wheel : {PointF(116.0f, 294.0f), PointF(154.0f, 300.0f), PointF(209.0f, 298.0f)}) {
        Pen stem(Color(255, 128, 156, 170), 8.3f);
        stem.SetStartCap(LineCapRound); stem.SetEndCap(LineCapRound);
        g.DrawLine(&stem, 165.0f, 282.0f, wheel.X, wheel.Y - 2.0f);
        Pen bevel(Color(255, 176, 207, 202), 3.6f);
        bevel.SetStartCap(LineCapRound); bevel.SetEndCap(LineCapRound);
        g.DrawLine(&bevel, 165.0f, 279.9f, wheel.X, wheel.Y - 4.1f);
        LuluArt::oval(g, {wheel.X - 6.1f, wheel.Y - 3.6f, 12.2f, 8.6f},
                     Color(255, 181, 201, 214), Color(255, 114, 138, 158));
    }
    panel(g, {154.0f, 239.0f, 22.0f, 46.0f}, 8.0f, Color(255, 224, 204, 231), Color(255, 177, 156, 190));
    Pen join(Color(157, 245, 226, 239), 2.8f);
    join.SetEndCap(LineCapRound);
    g.DrawLine(&join, 157.5f, 248.0f, 157.5f, 278.0f);

    chairUpholstery(g, swivel);
}

inline void drawer(Graphics& g, RectF front) {
    panel(g, front, 5.0f, Color(255, 236, 223, 243), Color(255, 212, 194, 227));
    panel(g, {front.X + front.Width * .34f, front.Y + 4.3f, front.Width * .32f, 4.4f}, 2.2f,
          Color(255, 255, 221, 232), Color(255, 217, 166, 193));
}

inline void drawSupport(Graphics& g) {
    LuluArt::oval(g, {45.0f, 280.0f, 276.0f, 28.0f}, Color(60, 83, 60, 104), Color(0, 83, 60, 104), .59f, .66f);
    panel(g, {81.0f, 195.0f, 12.0f, 75.0f}, 4.0f, Color(255, 204, 186, 227), Color(255, 157, 142, 185));
    GraphicsPath frontLeg;
    frontLeg.AddBezier(55.0f, 215.0f, 56.0f, 215.0f, 66.0f, 216.0f, 70.0f, 217.0f);
    frontLeg.AddLine(70.0f, 217.0f, 69.0f, 288.0f);
    frontLeg.AddBezier(69.0f, 288.0f, 68.0f, 295.0f, 61.0f, 296.0f, 58.0f, 289.0f);
    frontLeg.CloseFigure();
    gradient(g, frontLeg, 211.0f, 297.0f, Color(255, 231, 216, 245), Color(255, 182, 166, 210));
    panel(g, {259.0f, 207.0f, 47.0f, 88.0f}, 7.0f, Color(255, 222, 204, 239), Color(255, 179, 157, 206));
    panel(g, {262.0f, 226.0f, 40.0f, 64.0f}, 4.4f, Color(255, 189, 163, 214), Color(255, 178, 154, 207));
    drawer(g, {263.0f, 232.0f, 37.0f, 15.9f});
    drawer(g, {263.0f, 250.0f, 37.0f, 16.1f});
    drawer(g, {263.0f, 268.3f, 37.0f, 17.6f});
}

inline void plant(Graphics& g) {
    LuluArt::oval(g, {267.0f, 193.5f, 33.0f, 9.0f}, Color(56, 113, 92, 72), Color(0, 113, 92, 72));
    GraphicsPath pot;
    pot.AddBezier(269.0f, 179.0f, 271.0f, 175.5f, 289.0f, 176.0f, 294.0f, 180.0f);
    pot.AddLine(294.0f, 180.0f, 291.0f, 195.0f);
    pot.AddBezier(291.0f, 195.0f, 289.0f, 200.5f, 275.0f, 199.0f, 273.0f, 195.4f);
    pot.CloseFigure();
    LuluArt::volume(g, pot, {276.0f, 184.0f}, Color(255, 255, 228, 215), Color(255, 225, 174, 173));
    LuluArt::oval(g, {269.0f, 176.4f, 25.0f, 6.5f}, Color(255, 253, 226, 210), Color(255, 217, 174, 162));
    SolidBrush dirt(Color(255, 155, 117, 91));
    g.FillEllipse(&dirt, 272.0f, 178.0f, 19.4f, 3.6f);
    struct Leaf { PointF tip; float bend; Color light; Color shade; };
    const Leaf leaves[] = {
        {{269.0f, 151.0f}, -5.0f, Color(255, 194, 223, 152), Color(255, 113, 164, 122)},
        {{284.0f, 145.0f}, -8.0f, Color(255, 215, 235, 158), Color(255, 136, 183, 124)},
        {{300.0f, 153.0f}, 5.0f, Color(255, 191, 226, 167), Color(255, 108, 164, 128)},
        {{265.0f, 164.0f}, -6.0f, Color(255, 211, 233, 175), Color(255, 122, 172, 137)},
        {{299.0f, 166.0f}, 3.0f, Color(255, 207, 231, 159), Color(255, 129, 178, 122)}
    };
    PointF root(281.6f, 180.2f);
    for (const Leaf& leaf : leaves) {
        PointF mid((root.X + leaf.tip.X) * .5f, (root.Y + leaf.tip.Y) * .5f);
        GraphicsPath shape;
        shape.AddBezier(root, {mid.X + leaf.bend - 5.2f, mid.Y + 1.0f}, {leaf.tip.X - 5.5f, leaf.tip.Y + 5.0f}, leaf.tip);
        shape.AddBezier(leaf.tip, {leaf.tip.X + 8.0f, leaf.tip.Y + 5.0f}, {mid.X + leaf.bend + 4.5f, mid.Y}, root);
        shape.CloseFigure();
        LuluArt::volume(g, shape, mid, leaf.light, leaf.shade);
    }
}

inline void drawDesktop(Graphics& g) {
    GraphicsPath face;
    face.AddBezier(53.0f, 210.5f, 84.0f, 209.0f, 266.0f, 220.0f, 307.0f, 222.5f);
    face.AddBezier(307.0f, 222.5f, 312.0f, 223.0f, 313.5f, 223.0f, 313.5f, 225.0f);
    face.AddBezier(313.5f, 225.0f, 314.5f, 229.0f, 311.8f, 231.5f, 305.0f, 233.0f);
    face.AddLine(305.0f, 233.0f, 61.0f, 219.5f);
    face.AddBezier(61.0f, 219.5f, 53.0f, 219.0f, 51.0f, 216.0f, 53.0f, 210.5f);
    face.CloseFigure();
    gradient(g, face, 208.0f, 235.0f, Color(255, 255, 232, 216), Color(255, 218, 183, 181));

    GraphicsPath top;
    top.AddBezier(81.0f, 168.0f, 122.0f, 167.0f, 234.0f, 172.3f, 277.0f, 178.0f);
    top.AddBezier(277.0f, 178.0f, 285.0f, 179.0f, 288.5f, 181.0f, 294.0f, 190.0f);
    top.AddBezier(294.0f, 190.0f, 300.0f, 199.0f, 310.0f, 211.0f, 314.0f, 221.0f);
    top.AddBezier(314.0f, 221.0f, 317.4f, 228.0f, 296.0f, 229.0f, 288.0f, 228.5f);
    top.AddLine(288.0f, 228.5f, 63.0f, 215.6f);
    top.AddBezier(63.0f, 215.6f, 53.0f, 215.0f, 49.0f, 212.0f, 54.0f, 207.0f);
    top.AddBezier(54.0f, 207.0f, 63.7f, 193.0f, 74.0f, 170.1f, 81.0f, 168.0f);
    top.CloseFigure();
    gradient(g, top, 166.0f, 234.0f, Color(255, 249, 232, 219), Color(255, 255, 249, 231));
    Pen bevel(Color(213, 255, 255, 245), 1.35f);
    bevel.SetLineJoin(LineJoinRound);
    g.DrawPath(&bevel, &top);
    plant(g);
}
}  // namespace LuluOffice
