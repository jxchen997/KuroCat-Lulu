#pragma once

#include <array>
#include "AppResources.h"
#include "CatPhysics.hpp"
#include "LuluMesh.hpp"

namespace LuluReference {
using namespace Gdiplus;
using Vec = LuluMesh::Vector;
using LuluMesh::Vertex;
using Pixel = EmbeddedRaster::Pixel;

constexpr float Center = 164.0f;
constexpr float Top = 43.0f;
constexpr float Scale = 0.445f;
constexpr float ChartTop = 59.0f;
constexpr float FrontAxis = 644.0f;
constexpr float SideAxis = 296.0f;
constexpr float Neck = Top + (310.0f - ChartTop) * Scale;
constexpr float CuffJoin = 432.0f;
constexpr int FaceLeft = 495;
constexpr int FaceTop = 109;
constexpr int FrontWidth = 300;
constexpr int SideLeft = 131;
constexpr int SideWidth = 287;
constexpr int FaceWidth = FrontWidth + SideWidth;
constexpr int FaceHeight = 203;

inline float smooth(float from, float to, float value) {
    float t = std::clamp((value - from) / (to - from), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline Pixel opacity(Pixel pixel, unsigned weight) {
    constexpr Pixel channels = 0x00ff00ffu;
    auto rb = ((pixel & channels) * weight) >> 8;
    auto ag = (((pixel >> 8) & channels) * weight) >> 8;
    return (rb & channels) | ((ag & channels) << 8);
}

inline Pixel mixPixel(Pixel a, Pixel b, float fraction) {
    unsigned weight = static_cast<unsigned>(fraction * 256.0f + 0.5f);
    return opacity(a, 256 - weight) + opacity(b, weight);
}

inline Pixel over(Pixel foreground, Pixel background) {
    unsigned alpha = foreground >> 24;
    return foreground + opacity(background, 256 - alpha - (alpha >> 7));
}

struct Artwork {
    EmbeddedRaster chart;
    EmbeddedRaster face;
    EmbeddedRaster animated;
};

struct Eye { int x, y; float width, height; };
constexpr Eye Eyes[] = {{588 - FaceLeft, 158 - FaceTop, 19.5f, 17.5f},
                        {696 - FaceLeft, 152 - FaceTop, 19.5f, 17.5f},
                        {FrontWidth + 229 - SideLeft, 155 - FaceTop, 10.0f, 18.5f}};

inline Artwork loadArtwork() {
    Artwork art;
    art.chart = EmbeddedRaster::load(ID_LULU_TURNAROUND, 1280, 581);
    if (art.chart.pixels.empty()) return art;
    art.face = {FaceWidth, FaceHeight, std::vector<Pixel>(FaceWidth * FaceHeight)};
    art.animated = art.face;
    for (int y = 0; y < FaceHeight; ++y) {
        for (int x = 0; x < FaceWidth; ++x) {
            bool front = x < FrontWidth;
            int sourceX = front ? x + FaceLeft : x - FrontWidth + SideLeft;
            Pixel photo = art.chart.pixels[(y + FaceTop) * art.chart.width + sourceX];
            int red = (photo >> 16) & 255, blue = photo & 255;
            float pigment = smooth(12.0f, 43.0f, static_cast<float>(red - blue));
            for (Eye eye : Eyes) {
                float u = (x - eye.x) / eye.width, v = (y - eye.y) / eye.height;
                pigment = std::max(pigment, 1.0f - smooth(0.82f, 1.07f, u * u + v * v));
            }
            if (x >= 123 && x <= 192 && y >= 144 && y <= 171 && red < 103) pigment = 1.0f;
            art.face.pixels[y * FaceWidth + x] = opacity(photo, static_cast<unsigned>(256.0f * pigment + 0.5f));
        }
    }
    return art;
}

inline Artwork& artwork() {
    static Artwork art = loadArtwork();
    return art;
}

inline bool available() { return !artwork().face.pixels.empty(); }

inline Pixel sample(const EmbeddedRaster& image, float x, float y) {
    x = std::clamp(x, 0.0f, image.width - 1.001f);
    y = std::clamp(y, 0.0f, image.height - 1.001f);
    int column = static_cast<int>(x), row = static_cast<int>(y);
    const auto* pixel = image.pixels.data() + row * image.width + column;
    return mixPixel(mixPixel(pixel[0], pixel[1], x - column),
                    mixPixel(pixel[image.width], pixel[image.width + 1], x - column), y - row);
}

inline const EmbeddedRaster& faceTexture(Artwork& art, const CatPhysics& physics) {
    Pt gaze = physics.eyeGaze.point();
    float closure = physics.luluEyeClosure();
    float smile = smooth(0.12f, 0.92f, physics.luluContent.value);
    if (std::hypot(gaze.x, gaze.y) < 0.016f && closure < 0.01f && smile < 0.01f) return art.face;
    art.animated.pixels = art.face.pixels;
    float squint = smooth(0.0f, 0.72f, closure);
    float shut = smooth(0.53f, 0.91f, closure);
    for (Eye eye : Eyes) {
        int halfWidth = static_cast<int>(std::ceil(eye.width * 1.33f));
        int halfHeight = static_cast<int>(std::ceil(eye.height * 1.31f));
        for (int y = eye.y - halfHeight; y <= eye.y + halfHeight; ++y) {
            for (int x = eye.x - halfWidth; x <= eye.x + halfWidth; ++x) {
                float dx = (x - eye.x) / static_cast<float>(halfWidth), dy = (y - eye.y) / static_cast<float>(halfHeight);
                float falloff = smooth(0.0f, 0.81f, 1.0f - dx * dx - dy * dy);
                float yy = y + falloff * (-gaze.y * 6.0f * (1.0f - shut) + squint * ((y - eye.y) * 0.93f - 5.0f));
                Pixel color = sample(art.face, x - gaze.x * eye.width * 0.40f * falloff * (1.0f - shut), yy);
                if (shut > 0.0f) {
                    Pixel skin = sample(art.face, x, eye.y - eye.height - 3.0f - std::max(0.0f, -dy * 2.0f));
                    float lidCurve = eye.y - 2.2f + 8.0f * dx * dx;
                    float ink = (1.0f - smooth(0.15f, 1.5f, std::abs(y - lidCurve))) * (1.0f - smooth(0.44f, 0.68f, std::abs(dx)));
                    Pixel lid = mixPixel(skin, 0xffa36a28u, ink * 0.76f);
                    color = over(opacity(lid, static_cast<unsigned>(256.0f * shut * smooth(0.06f, 0.65f, falloff))), color);
                }
                art.animated.pixels[y * FaceWidth + x] = color;
            }
        }
    }
    if (smile > 0.0f) {
        for (int y = 141; y <= 181; ++y) {
            for (int x = 105; x <= 205; ++x) {
                float dx = (x - 155.0f) / 50.0f, dy = (y - 161.0f) / 20.0f;
                float falloff = smooth(0.0f, 0.67f, 1.0f - dx * dx - dy * dy) * smile;
                art.animated.pixels[y * FaceWidth + x] = sample(art.face, x - (x - 155.0f) * 0.10f * falloff,
                                                              y + (dx * dx * 11.0f - 1.7f) * falloff);
            }
        }
    }
    return art.animated;
}

struct Landmark {
    float y, left, right, muzzle, back;
};

// Outline landmarks in the supplied chart: x from the front, depth from the side.
// The hands and the two trouser legs are modelled separately from this main shape.
constexpr Landmark Outline[] = {
    {109, 599, 679, 255, 324}, {118, 580, 704, 238, 338},
    {132, 558, 724, 221, 356}, {149, 548, 738, 212, 371},
    {165, 537, 750, 196, 380}, {175, 530, 758, 173, 385},
    {197, 518, 772, 148, 390}, {218, 511, 777, 141, 392},
    {232, 509, 778, 139, 391}, {252, 512, 772, 142, 386},
    {274, 525, 755, 151, 380}, {288, 540, 739, 160, 378},
    {301, 550, 740, 181, 379}, {314, 549, 743, 209, 381},
    {331, 545, 746, 211, 384}, {352, 542, 750, 201, 389},
    {376, 540, 751, 196, 393}, {395, 540, 751, 195, 394},
    {415, 542, 750, 198, 385}, {432, 545, 748, 213, 369},
    {442, 548, 743, 229, 355}, {447, 577, 717, 240, 347},
    {452, 643, 645, 293, 295}
};

inline float interpolate(size_t index, float y, float Landmark::*field) {
    const auto& a = Outline[index];
    const auto& b = Outline[index + 1];
    const auto& prev = Outline[index == 0 ? index : index - 1];
    const auto& next = Outline[std::min(index + 2, std::size(Outline) - 1)];
    float length = b.y - a.y;
    float t = (y - a.y) / length;
    float t0 = (b.*field - prev.*field) * length / (b.y - prev.y);
    float t1 = (next.*field - a.*field) * length / (next.y - a.y);
    float delta = b.*field - a.*field;
    return a.*field + t * (t0 + t * (3.0f * delta - 2.0f * t0 - t1 + t * (-2.0f * delta + t0 + t1)));
}

struct Section { float x, width, front, rear; };

inline Section outlineAt(float y) {
    y = std::clamp(y, Outline[0].y, Outline[std::size(Outline) - 1].y);
    size_t index = 0;
    while (Outline[index + 1].y < y) ++index;
    float l = interpolate(index, y, &Landmark::left), r = interpolate(index, y, &Landmark::right);
    return {(l + r) * 0.5f - FrontAxis, std::max(1.0f, (r - l) * 0.5f),
            std::max(1.0f, SideAxis - interpolate(index, y, &Landmark::muzzle)),
            std::max(1.0f, interpolate(index, y, &Landmark::back) - SideAxis)};
}

constexpr Vec SkinBright{255, 245, 147};
constexpr Vec SkinShade{226, 180, 82};
constexpr Vec ClothBright{234, 154, 92};
constexpr Vec ClothShade{168, 88, 45};

inline Vec bend(Vec point, float y, float z, float angle) {
    float height = point.y - y, depth = point.z - z;
    return {point.x, y + height * std::cos(angle) - depth * std::sin(angle),
            z + height * std::sin(angle) + depth * std::cos(angle)};
}

struct SeatedPose {
    float hips, shoulders, head, center, lean, lift;
    bool seated;

    explicit SeatedPose(const CatPhysics& physics) : seated(true) {
        float input = std::clamp(physics.headYaw.value, -1.0f, 1.0f);
        hips = std::clamp(physics.luluSwivel.value, -1.0f, 1.0f) * 0.72f;
        shoulders = hips + 0.12f * input;
        float target = input * (0.53f + 0.49f * std::abs(input));
        head = shoulders + std::clamp(target - shoulders, -0.30f, 0.30f);
        center = Center - 5.0f * std::sin(hips);
        float content = smooth(0.0f, 1.0f, physics.luluContent.value);
        lean = physics.luluPetLean.value * content;
        lift = -0.025f * physics.headPitch.value + content * (0.027f + 0.011f * std::sin(physics.time * 6.0f));
    }

    explicit SeatedPose(float angle) : hips(angle), shoulders(angle), head(angle), center(Center), lean(0), lift(0), seated(false) {}

    float angleAt(float y) const {
        float chest = shoulders + (hips - shoulders) * smooth(329.0f, 395.0f, y);
        return head + (chest - head) * smooth(270.0f, 329.0f, y);
    }

    Vec project(Vec local) const {
        float y = Top + (local.y - ChartTop) * Scale;
        float depth = local.z * Scale;
        float fromNeck = 1.0f - smooth(145.0f, 162.0f, y);
        float perspective = seated ? smooth(151.0f, 212.0f, y) : 0.0f;
        return {center + local.x * Scale * (1.0f + 0.0013f * depth * perspective) + (Neck - y) * lean * 0.13f * fromNeck,
                y + (y - Neck) * lift * fromNeck + perspective * (depth * 0.235f - 3.0f),
                depth - perspective * (y - 160.0f) * 0.11f};
    }

    Vec joint(Vec body, float angle) const { return project(LuluMesh::yaw(body, std::sin(angle), std::cos(angle))); }

    Vec pelvis(Vec p) const {
        if (!seated) return p;
        return bend(p, 408.0f, 4.0f, 0.34f * smooth(390.0f, 452.0f, p.y));
    }

    Vec leg(Vec p) const {
        if (!seated) return p;
        constexpr float hip = 413.0f, knee = 454.0f, pitch = 0.76f;
        Vec thigh = bend(p, hip, 5.0f, pitch * smooth(402.0f, 438.0f, p.y));
        Vec restKnee{0.0f, knee, 1.0f};
        Vec swungKnee = bend(restKnee, hip, 5.0f, pitch);
        Vec shin = bend(p, knee, 1.0f, -0.19f) + (swungKnee - restKnee);
        float transition = smooth(445.0f, 462.0f, p.y);
        Vec bent = thigh * (1.0f - transition) + shin * transition;
        float join = smooth(CuffJoin, 450.0f, p.y);
        return pelvis(p) * (1.0f - join) + bent * join;
    }
};

inline Vec asColor(Pixel pixel) {
    return {static_cast<float>((pixel >> 16) & 255), static_cast<float>((pixel >> 8) & 255), static_cast<float>(pixel & 255)};
}

struct BodyRow {
    Section shape, before, after;
    float y, sn, cs;
};

inline Vec outlineNormal(Section shape, Section before, Section after, float u, float v) {
    float d = v >= 0.0f ? shape.front : shape.rear;
    float prev = v >= 0.0f ? before.front : before.rear;
    float next = v >= 0.0f ? after.front : after.rear;
    float nx = u / shape.width, nz = v / d;
    float tiltX = (after.x - before.x + (after.width - before.width) * u) / 3.0f;
    float tiltZ = (next - prev) * v / 3.0f;
    return LuluMesh::unit({nx, -nx * tiltX - nz * tiltZ, nz});
}

inline void meshBody(LuluMesh::Layer& layer, const EmbeddedRaster& face, const SeatedPose& pose) {
    constexpr size_t Rows = 106;
    std::array<BodyRow, Rows + 1> rows;
    for (size_t i = 0; i <= Rows; ++i) {
        float y = Outline[0].y + (CuffJoin - Outline[0].y) * static_cast<float>(i) / Rows;
        float yaw = pose.angleAt(y);
        rows[i] = {outlineAt(y), outlineAt(y - 1.5f), outlineAt(y + 1.5f), y, std::sin(yaw), std::cos(yaw)};
    }
    float absoluteYaw = std::abs(pose.head);
    float viewSide = smooth(0.72f, 1.56f, absoluteYaw) * (1.0f - smooth(1.82f, 2.48f, absoluteYaw));
    LuluMesh::surface<Rows, 76>(layer, nullptr, &face, [&](float t, float u, float v) -> Vertex {
        const auto& ring = rows[static_cast<size_t>(t * Rows + 0.5f)];
        float d = v >= 0.0f ? ring.shape.front : ring.shape.rear;
        float x = ring.shape.x + ring.shape.width * u;
        Vec normal = outlineNormal(ring.shape, ring.before, ring.after, u, v);
        normal = LuluMesh::yaw(normal, ring.sn, ring.cs);
        Vec p{x, ring.y, d * v};
        float throat = smooth(272.0f, 303.0f, ring.y) * (1.0f - smooth(315.0f, 344.0f, ring.y));
        Vec skin = LuluMesh::lit(SkinShade, SkinBright, normal, 1.0f - 0.19f * throat);
        float waist = 384.5f + 8.0f * v;
        float shorts = smooth(waist - 1.8f, waist + 1.8f, ring.y);
        Vec orange = LuluMesh::lit(ClothShade, ClothBright, normal);
        Vec color = skin * (1.0f - shorts) + orange * shorts;
        float eyeHeight = smooth(122.0f, 135.0f, ring.y) * (1.0f - smooth(169.0f, 185.0f, ring.y));
        float keepFrontEyes = eyeHeight * (1.0f - smooth(1.18f, 1.55f, absoluteYaw));
        float side = viewSide * (1.0f - keepFrontEyes) * smooth(0.0f, 0.25f, pose.head > 0.0f ? -u : u)
            * (1.0f - smooth(284.0f, 309.0f, ring.y));
        float paint = smooth(0.0f, 0.61f, v) * (1.0f - smooth(281.0f, 316.0f, ring.y)) * (1.0f - side);
        return {pose.project(LuluMesh::yaw(pose.pelvis(p), ring.sn, ring.cs)), color, paint,
                x + FrontAxis - FaceLeft, ring.y - FaceTop, side, FrontWidth + SideAxis - p.z - SideLeft};
    });
}

struct LegRing { float y, center, width, depth, z; };

// Cuffs break at the measured inside notch. Feet have the low, flat profile of the chart.
constexpr LegRing Leg[] = {
    {432, 54, 47, 65, 5}, {444, 56, 43, 55, 4},
    {451, 57, 38.5f, 49, 3}, {459, 56, 37, 43, 1},
    {468, 56, 34, 35, -2}, {475, 56, 28, 32, -2},
    {487, 56, 26, 33, -1}, {491, 56, 22, 30, 0}, {493, 56, 2, 2, 1}
};

inline void meshLeg(LuluMesh::Layer& layer, const SeatedPose& pose, float side) {
    float sn = std::sin(pose.hips), cs = std::cos(pose.hips);
    Section join = outlineAt(CuffJoin);
    Section before = outlineAt(CuffJoin - 1.5f), after = outlineAt(CuffJoin + 1.5f);
    LuluMesh::surface<39, 76>(layer, nullptr, nullptr, [&](float t, float u, float v) -> Vertex {
        float y = Leg[0].y + (Leg[std::size(Leg) - 1].y - Leg[0].y) * t;
        size_t index = 0;
        while (Leg[index + 1].y < y) ++index;
        const auto& a = Leg[index];
        const auto& b = Leg[index + 1];
        float k = (y - a.y) / (b.y - a.y);
        float width = a.width + (b.width - a.width) * k, depth = a.depth + (b.depth - a.depth) * k;
        float center = a.center + (b.center - a.center) * k, z = a.z + (b.z - a.z) * k;
        float trouserBlend = smooth(CuffJoin, 454.0f, y);
        float split = smooth(442.0f, 453.0f, y);
        float hipX = join.x + side * join.width * std::max(0.0f, u);
        float hipZ = (v >= 0.0f ? join.front : join.rear) * v;
        float legX = side * (center + width * u);
        float towardLeg = trouserBlend * smooth(-0.65f, 0.0f, u) + split * (1.0f - smooth(-0.65f, 0.0f, u));
        Vec local{hipX + (legX - hipX) * towardLeg, y, hipZ + (z + depth * v - hipZ) * trouserBlend};
        Vec hipNormal = outlineNormal(join, before, after, side * u, v);
        Vec legNormal = LuluMesh::unit({side * u / width, 0.003f + 0.014f * t, v / depth});
        Vec normal = LuluMesh::unit(hipNormal * (1.0f - trouserBlend) + legNormal * trouserBlend);
        normal = LuluMesh::yaw(normal, sn, cs);
        float cuff = 466.0f + 4.2f * v - 3.1f * side * u;
        float skin = smooth(cuff - 1.8f, cuff + 1.8f, y);
        Vec color = LuluMesh::lit(ClothShade, ClothBright, normal) * (1.0f - skin)
            + LuluMesh::lit(SkinShade, SkinBright, normal) * skin;
        return {pose.project(LuluMesh::yaw(pose.leg(local), sn, cs)), color};
    });
}

inline void ellipsoid(LuluMesh::Layer& layer, const SeatedPose& pose, Vec center, Vec axis, Vec radii,
                      float angle, Vec shade, Vec light) {
    Vec crossAxis{-axis.y, axis.x, 0.0f};
    float sn = std::sin(angle), cs = std::cos(angle);
    LuluMesh::surface<20, 36>(layer, nullptr, nullptr, [&](float t, float u, float v) -> Vertex {
        float phi = t * 3.14159265f, width = std::sin(phi), along = std::cos(phi);
        Vec p = center + axis * (along * radii.y) + crossAxis * (u * width * radii.x) + Vec{0, 0, v * width * radii.z};
        Vec n = LuluMesh::unit(axis * (along / radii.y) + crossAxis * (u * width / radii.x) + Vec{0, 0, v * width / radii.z});
        return {pose.project(LuluMesh::yaw(p, sn, cs)), LuluMesh::lit(shade, light, LuluMesh::yaw(n, sn, cs))};
    });
}

struct RenderedPose {
    LuluMesh::Layer body;
    LuluMesh::Layer hands;
};

inline RenderedPose& renderedPose() {
    static RenderedPose frame;
    return frame;
}

inline void drawLayer(Graphics& g, LuluMesh::Layer& layer) {
    auto& image = layer.surface;
    Bitmap bitmap(image.width, image.height, image.width * sizeof(Pixel), PixelFormat32bppPARGB, reinterpret_cast<BYTE*>(image.pixels.data()));
    RectF rect(LuluMesh::Left, LuluMesh::Top, LuluMesh::FrameWidth / LuluMesh::Resolution,
               LuluMesh::FrameHeight / LuluMesh::Resolution);
    g.DrawImage(&bitmap, rect, 0.0f, 0.0f, image.width, image.height, UnitPixel);
}

inline void renderBody(Graphics& g, const SeatedPose& pose, const EmbeddedRaster& face) {
    auto& layer = renderedPose().body;
    layer.clear();
    meshBody(layer, face, pose);
    for (float side : {-1.0f, 1.0f}) {
        meshLeg(layer, pose, side);
        // The ear axes correspond to the slight outward tilt seen in the front and rear views.
        ellipsoid(layer, pose, {side * 94.0f, side < 0.0f ? 141.0f : 134.0f, -21.0f},
                  {side * 0.71f, -0.704f, 0.0f}, {12.5f, 24.0f, 15.0f}, pose.head,
                  {215, 169, 75}, {255, 243, 151});
    }
    ellipsoid(layer, pose, {-8.0f, 92.0f, 7.0f}, {0, -1, 0}, {36.0f, 23.0f, 36.0f}, pose.head,
              {226, 139, 47}, {255, 204, 104});
    ellipsoid(layer, pose, {-7.2f, 66.5f, 7.0f}, {0.28f, -0.96f, 0}, {4.0f, 8.5f, 4.4f}, pose.head,
              {106, 129, 60}, {166, 186, 86});
    Vec tail = pose.pelvis({0.0f, 409.0f, -105.0f});
    ellipsoid(layer, pose, tail, {0, -1, 0}, {12, 11.8f, 20.0f}, pose.hips,
              SkinShade, SkinBright);
    GraphicsState state = g.Save();
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    drawLayer(g, layer);
    g.Restore(state);
}

inline void drawBody(Graphics& g, const CatPhysics& physics) {
    renderBody(g, SeatedPose(physics), faceTexture(artwork(), physics));
}

inline void meshArm(RenderedPose& frame, Vec shoulder, Vec hand, float side, bool seated) {
    Vec line = hand - shoulder;
    Vec bow{side * (seated ? 4.7f : 3.0f), seated ? 2.6f : 0.0f, 0.0f};
    Pixel root = frame.body.at(shoulder.x, shoulder.y + 2.0f);
    Vec shoulderTone = root >> 24 ? asColor(root) : SkinBright;
    LuluMesh::surface<31, 32>(frame.hands, &frame.body, nullptr, [&](float t, float u, float v) -> Vertex {
        Vec point = shoulder + line * t + bow * (4.0f * t * (1.0f - t));
        Vec tangent = LuluMesh::unit(line + bow * (4.0f - 8.0f * t));
        Vec crossAxis = LuluMesh::unit({-tangent.y, tangent.x, 0.0f});
        Vec faceAxis = LuluMesh::cross(tangent, crossAxis);
        float width = ((seated ? 8.0f : 10.8f) - (seated ? 2.7f : 2.9f) * t)
            * std::sqrt(std::clamp((1.0f - t) * 11.2f, 0.0f, 1.0f))
            * std::sqrt(std::clamp(t * 8.0f, 0.0f, 1.0f));
        Vec radial = crossAxis * u + faceAxis * v;
        Vec normal = LuluMesh::unit(radial + tangent * (0.71f * smooth(0.84f, 1.0f, t) - 0.60f * (1.0f - smooth(0.0f, 0.18f, t))));
        Vec light = LuluMesh::lit(SkinShade, SkinBright, normal);
        float blend = smooth(0.06f, 0.41f, t);
        return {point + radial * width, shoulderTone * (1.0f - blend) + light * blend};
    });
}

inline void paintArms(Graphics& g, const SeatedPose& pose, const CatPhysics* physics) {
    auto& frame = renderedPose();
    frame.hands.clear();
    for (int i = 0; i < 2; ++i) {
        float side = i == 0 ? -1.0f : 1.0f;
        float shoulderY = pose.seated ? 316.0f : 302.5f;
        Vec shoulder = pose.joint({side * 86.0f, shoulderY, pose.seated ? 19.0f : -5.0f}, pose.angleAt(shoulderY));
        Vec hand;
        if (physics) {
            const PetKeystroke& strike = i == 0 ? physics->leftStrike : physics->rightStrike;
            Pt tip = strike.key != PhysicalKey::None ? strike.tip : KeyboardLayout::luluRestingTip(i == 0);
            hand = {tip.x, tip.y, 57.0f};
        } else {
            hand = pose.joint({side * 115.0f, 414.0f, 10.0f}, pose.shoulders);
        }
        float outward = pose.seated ? side : side * std::cos(pose.shoulders);
        meshArm(frame, shoulder, hand, outward, pose.seated);
    }
    GraphicsState state = g.Save();
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    drawLayer(g, frame.hands);
    g.Restore(state);
}

inline void drawHands(Graphics& g, const CatPhysics& physics) {
    paintArms(g, SeatedPose(physics), &physics);
}

inline void drawStanding(Graphics& g, float yaw) {
    const SeatedPose pose(yaw);
    renderBody(g, pose, artwork().face);
    paintArms(g, pose, nullptr);
}

inline CatHitZone hitTest(Pt point, const CatPhysics& physics) {
    const auto& frame = renderedPose();
    if (frame.body.at(point.x, point.y) >> 24) return point.y < Neck ? CatHitZone::Head : CatHitZone::Body;
    if (frame.hands.at(point.x, point.y) >> 24) return CatHitZone::Body;
    if (point.y > 82.0f && point.y < 138.0f && std::abs(point.x - SeatedPose(physics).center) < 23.0f) {
        return CatHitZone::Head;
    }
    return CatHitZone::None;
}

inline bool drawIcon(Graphics& g) {
    if (!available()) return false;
    auto state = g.Save();
    g.ScaleTransform(0.248f, 0.248f);
    g.TranslateTransform(-100.0f, -37.0f);
    renderBody(g, SeatedPose(0.0f), artwork().face);
    g.Restore(state);
    return true;
}
}  // namespace LuluReference

