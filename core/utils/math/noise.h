#pragma once

#include <cmath>
#include <cstdint>
#include <array>
#include <algorithm>
#include <random>

class Noise {
private:
    static const int P_SIZE = 256;
    static const int P_MASK = P_SIZE - 1;
    std::array<int, P_SIZE * 2> p;

    static float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    static float grad(int hash, float x) {
        int h = hash & 15;
        float grad = 1.0f + (h & 7);
        if (h & 8) grad = -grad;
        return grad * x;
    }

    static float grad(int hash, float x, float y) {
        int h = hash & 3;
        float u = h < 2 ? x : y;
        float v = h < 2 ? y : x;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    static float grad(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    Noise(uint32_t seed = 0) {
        std::array<int, P_SIZE> perm;
        for (int i = 0; i < P_SIZE; i++)
            perm[i] = i;

        std::mt19937 rng(seed ? seed : 1);
        std::shuffle(perm.begin(), perm.end(), rng);

        for (int i = 0; i < P_SIZE; i++) {
            p[i] = perm[i];
            p[P_SIZE + i] = perm[i];
        }
    }

    void setSeed(uint32_t seed) {
        std::array<int, P_SIZE> perm;
        for (int i = 0; i < P_SIZE; i++)
            perm[i] = i;

        std::mt19937 rng(seed ? seed : 1);
        std::shuffle(perm.begin(), perm.end(), rng);

        for (int i = 0; i < P_SIZE; i++) {
            p[i] = perm[i];
            p[P_SIZE + i] = perm[i];
        }
    }

    float perlin1D(float x) const {
        int xi = static_cast<int>(std::floor(x)) & P_MASK;
        float xf = x - std::floor(x);
        float u = fade(xf);
        int a = p[xi];
        int b = p[xi + 1];
        return lerp(grad(a, xf), grad(b, xf - 1.0f), u);
    }

    float perlin2D(float x, float y) const {
        int xi = static_cast<int>(std::floor(x)) & P_MASK;
        int yi = static_cast<int>(std::floor(y)) & P_MASK;
        float xf = x - std::floor(x);
        float yf = y - std::floor(y);
        float u = fade(xf);
        float v = fade(yf);

        int aa = p[p[xi] + yi];
        int ab = p[p[xi] + yi + 1];
        int ba = p[p[xi + 1] + yi];
        int bb = p[p[xi + 1] + yi + 1];

        float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0f, yf), u);
        float x2 = lerp(grad(ab, xf, yf - 1.0f), grad(bb, xf - 1.0f, yf - 1.0f), u);
        return lerp(x1, x2, v);
    }

    float perlin3D(float x, float y, float z) const {
        int xi = static_cast<int>(std::floor(x)) & P_MASK;
        int yi = static_cast<int>(std::floor(y)) & P_MASK;
        int zi = static_cast<int>(std::floor(z)) & P_MASK;
        float xf = x - std::floor(x);
        float yf = y - std::floor(y);
        float zf = z - std::floor(z);
        float u = fade(xf);
        float v = fade(yf);
        float w = fade(zf);

        int a = p[xi] + yi;
        int aa = p[a] + zi;
        int ab = p[a + 1] + zi;
        int b = p[xi + 1] + yi;
        int ba = p[b] + zi;
        int bb = p[b + 1] + zi;

        float x1 = lerp(grad(p[aa], xf, yf, zf), grad(p[ba], xf - 1.0f, yf, zf), u);
        float x2 = lerp(grad(p[ab], xf, yf - 1.0f, zf), grad(p[bb], xf - 1.0f, yf - 1.0f, zf), u);
        float y1 = lerp(x1, x2, v);

        x1 = lerp(grad(p[aa + 1], xf, yf, zf - 1.0f), grad(p[ba + 1], xf - 1.0f, yf, zf - 1.0f), u);
        x2 = lerp(grad(p[ab + 1], xf, yf - 1.0f, zf - 1.0f), grad(p[bb + 1], xf - 1.0f, yf - 1.0f, zf - 1.0f), u);
        float y2 = lerp(x1, x2, v);

        return lerp(y1, y2, w);
    }

    float simplex2D(float x, float y) const {
        const float F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
        const float G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;

        float s = (x + y) * F2;
        int i = static_cast<int>(std::floor(x + s));
        int j = static_cast<int>(std::floor(y + s));

        float t = static_cast<float>(i + j) * G2;
        float X0 = i - t;
        float Y0 = j - t;
        float x0 = x - X0;
        float y0 = y - Y0;

        int i1, j1;
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else { i1 = 0; j1 = 1; }

        float x1 = x0 - i1 + G2;
        float y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2;
        float y2 = y0 - 1.0f + 2.0f * G2;

        int ii = i & P_MASK;
        int jj = j & P_MASK;

        float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;

        float t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 > 0) {
            t0 *= t0;
            int gi = p[p[ii] + jj];
            n0 = t0 * t0 * grad(gi, x0, y0);
        }

        float t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 > 0) {
            t1 *= t1;
            int gi = p[p[ii + i1] + jj + j1];
            n1 = t1 * t1 * grad(gi, x1, y1);
        }

        float t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 > 0) {
            t2 *= t2;
            int gi = p[p[ii + 1] + jj + 1];
            n2 = t2 * t2 * grad(gi, x2, y2);
        }

        return 70.0f * (n0 + n1 + n2);
    }

    float value2D(float x, float y) const {
        int xi = static_cast<int>(std::floor(x)) & P_MASK;
        int yi = static_cast<int>(std::floor(y)) & P_MASK;
        float xf = x - std::floor(x);
        float yf = y - std::floor(y);
        float u = fade(xf);
        float v = fade(yf);

        float a = static_cast<float>(p[p[xi] + yi]) / 255.0f;
        float b = static_cast<float>(p[p[xi + 1] + yi]) / 255.0f;
        float c = static_cast<float>(p[p[xi] + yi + 1]) / 255.0f;
        float d = static_cast<float>(p[p[xi + 1] + yi + 1]) / 255.0f;

        return lerp(lerp(a, b, u), lerp(c, d, u), v);
    }

    float fbm2D(float x, float y, int octaves = 6, float lacunarity = 2.0f, float gain = 0.5f) const {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            value += amplitude * perlin2D(x * frequency, y * frequency);
            maxValue += amplitude;
            amplitude *= gain;
            frequency *= lacunarity;
        }

        return value / maxValue;
    }

    float fbmSimplex2D(float x, float y, int octaves = 6, float lacunarity = 2.0f, float gain = 0.5f) const {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            value += amplitude * simplex2D(x * frequency, y * frequency);
            maxValue += amplitude;
            amplitude *= gain;
            frequency *= lacunarity;
        }

        return value / maxValue;
    }

    float domainWarp2D(float x, float y, float warpScale = 1.0f) const {
        float warpX = perlin2D(x, y) * warpScale;
        float warpY = perlin2D(x + 5.2f, y + 1.3f) * warpScale;
        return perlin2D(x + warpX, y + warpY);
    }

    float fbmDomainWarp2D(float x, float y, int octaves = 6, float lacunarity = 2.0f, float gain = 0.5f, float warpScale = 1.0f) const {
        float warpX = fbm2D(x, y, octaves, lacunarity, gain) * warpScale;
        float warpY = fbm2D(x + 5.2f, y + 1.3f, octaves, lacunarity, gain) * warpScale;
        return fbm2D(x + warpX, y + warpY, octaves, lacunarity, gain);
    }
};
