#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include "EmbeddedRaster.hpp"

namespace LuluMesh {
using Pixel = EmbeddedRaster::Pixel;

struct Vector {
    float x, y, z;
    Vector operator+(Vector p) const { return {x + p.x, y + p.y, z + p.z}; }
    Vector operator-(Vector p) const { return {x - p.x, y - p.y, z - p.z}; }
    Vector operator*(float k) const { return {x * k, y * k, z * k}; }
};

inline float dot(Vector a, Vector b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vector cross(Vector a, Vector b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline Vector unit(Vector v) { return v * (1.0f / std::sqrt(std::max(0.00001f, dot(v, v)))); }
inline Vector yaw(Vector v, float sine, float cosine) {
    return {v.x * cosine + v.z * sine, v.y, v.z * cosine - v.x * sine};
}

constexpr float Left = 72.0f;
constexpr float Top = 42.0f;
constexpr float Resolution = 2.7f;
constexpr int FrameWidth = 483;
constexpr int FrameHeight = 569;

struct Vertex {
    Vector point;
    Vector light;
    float paint = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float sidePaint = 0.0f;
    float sideU = 0.0f;
};

struct Layer {
    EmbeddedRaster surface{FrameWidth, FrameHeight, std::vector<Pixel>(FrameWidth * FrameHeight)};
    std::vector<float> depth = std::vector<float>(FrameWidth * FrameHeight);

    void clear() {
        std::fill(surface.pixels.begin(), surface.pixels.end(), 0u);
        std::fill(depth.begin(), depth.end(), -std::numeric_limits<float>::infinity());
    }

    Pixel at(float x, float y) const {
        int column = static_cast<int>(std::floor((x - Left) * Resolution));
        int row = static_cast<int>(std::floor((y - Top) * Resolution));
        if (column < 0 || column >= FrameWidth || row < 0 || row >= FrameHeight) return 0;
        return surface.pixels[row * FrameWidth + column];
    }
};

inline Vector lit(Vector shadow, Vector light, Vector normal, float occlusion = 1.0f) {
    constexpr Vector key{-0.46f, -0.59f, 0.66f};
    constexpr Vector fill{0.81f, -0.17f, 0.56f};
    float diffuse = 0.14f + std::max(0.0f, dot(normal, key)) * 0.74f
        + std::max(0.0f, dot(normal, fill)) * 0.17f;
    return shadow + (light - shadow) * std::clamp(diffuse * occlusion, 0.0f, 1.0f);
}

inline Pixel colorPixel(Vector color) {
    auto channel = [](float value) { return static_cast<Pixel>(std::clamp(value, 0.0f, 255.0f) + 0.5f); };
    return 0xff000000u | channel(color.x) << 16 | channel(color.y) << 8 | channel(color.z);
}

inline float edge(Vector a, Vector b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

inline void triangle(Layer& layer, const Layer* occluder, const EmbeddedRaster* texture,
                     Vertex a, Vertex b, Vertex c) {
    auto raster = [](Vector p) -> Vector { return {(p.x - Left) * Resolution, (p.y - Top) * Resolution, p.z}; };
    a.point = raster(a.point);
    b.point = raster(b.point);
    c.point = raster(c.point);
    float area = edge(a.point, b.point, c.point.x, c.point.y);
    if (std::abs(area) < 0.00001f) return;
    int left = std::max(0, static_cast<int>(std::floor(std::min({a.point.x, b.point.x, c.point.x}))));
    int right = std::min(FrameWidth - 1, static_cast<int>(std::ceil(std::max({a.point.x, b.point.x, c.point.x}))));
    int top = std::max(0, static_cast<int>(std::floor(std::min({a.point.y, b.point.y, c.point.y}))));
    int bottom = std::min(FrameHeight - 1, static_cast<int>(std::ceil(std::max({a.point.y, b.point.y, c.point.y}))));
    float inverse = 1.0f / area;
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            float alpha = edge(b.point, c.point, x + 0.5f, y + 0.5f) * inverse;
            float beta = edge(c.point, a.point, x + 0.5f, y + 0.5f) * inverse;
            float gamma = 1.0f - alpha - beta;
            if (std::min({alpha, beta, gamma}) < -0.0001f) continue;
            int index = y * FrameWidth + x;
            float depth = a.point.z * alpha + b.point.z * beta + c.point.z * gamma;
            if (depth < layer.depth[index] || (occluder && depth + 0.12f < occluder->depth[index])) continue;
            Vector color = a.light * alpha + b.light * beta + c.light * gamma;
            if (texture) {
                int v = std::clamp(static_cast<int>(a.v * alpha + b.v * beta + c.v * gamma + 0.5f), 0, texture->height - 1);
                auto addPaint = [&](float u, float amount) {
                    if (amount <= 0.001f) return;
                    int column = std::clamp(static_cast<int>(u + 0.5f), 0, texture->width - 1);
                    Pixel px = texture->pixels[v * texture->width + column];
                    float coverage = static_cast<float>(px >> 24) / 255.0f;
                    Vector pigment{static_cast<float>((px >> 16) & 255), static_cast<float>((px >> 8) & 255),
                                   static_cast<float>(px & 255)};
                    color = color * (1.0f - amount * coverage) + pigment * amount;
                };
                addPaint(a.u * alpha + b.u * beta + c.u * gamma, a.paint * alpha + b.paint * beta + c.paint * gamma);
                addPaint(a.sideU * alpha + b.sideU * beta + c.sideU * gamma,
                         a.sidePaint * alpha + b.sidePaint * beta + c.sidePaint * gamma);
            }
            layer.surface.pixels[index] = colorPixel(color);
            layer.depth[index] = depth;
        }
    }
}

template <size_t Sides>
inline void join(Layer& layer, const Layer* occluder, const EmbeddedRaster* texture,
                 const std::array<Vertex, Sides>& upper, const std::array<Vertex, Sides>& lower) {
    for (size_t i = 0; i < Sides; ++i) {
        size_t next = (i + 1) % Sides;
        triangle(layer, occluder, texture, upper[i], lower[i], lower[next]);
        triangle(layer, occluder, texture, upper[i], lower[next], upper[next]);
    }
}

template <size_t Segments, size_t Sides, class Sample>
inline void surface(Layer& layer, const Layer* occluder, const EmbeddedRaster* texture, Sample sample) {
    static const auto directions = [] {
        std::array<Gdiplus::PointF, Sides> result;
        for (size_t i = 0; i < Sides; ++i) {
            float azimuth = static_cast<float>(i) * 6.2831853f / Sides;
            result[i] = Gdiplus::PointF(std::sin(azimuth), std::cos(azimuth));
        }
        return result;
    }();
    std::array<Vertex, Sides> previous, ring;
    for (size_t row = 0; row <= Segments; ++row) {
        for (size_t i = 0; i < Sides; ++i) {
            ring[i] = sample(static_cast<float>(row) / Segments, directions[i].X, directions[i].Y);
        }
        if (row > 0) join(layer, occluder, texture, previous, ring);
        previous = ring;
    }
}
}  // namespace LuluMesh
