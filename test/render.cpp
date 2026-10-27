// Renders the docs' listening page: the five documented patches and the three
// macro sweeps (test/patches.hpp), each as a 48 kHz 16-bit mono WAV normalised
// to -1 dBFS.
//
//   build/render <dir>     (make -C test renders)
//
// Parameters update once per 48 samples, like the firmware's 1 kHz control rate.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "patches.hpp"

using namespace doom;

static constexpr float kFs = 48000.f;

static bool writeWav(const std::string& path, const std::vector<float>& x) {
    float peak = 1e-9f;
    for (float v : x) peak = std::fmax(peak, std::fabs(v));
    const float g = 0.891f / peak; // -1 dBFS
    FILE*       f = std::fopen(path.c_str(), "wb");
    if (!f) {
        std::perror(path.c_str());
        return false;
    }
    const uint32_t n = static_cast<uint32_t>(x.size()), bytes = n * 2, rate = 48000;
    auto           u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto           u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f);
    u32(36 + bytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16);
    u16(1); // PCM
    u16(1); // mono
    u32(rate);
    u32(rate * 2);
    u16(2);
    u16(16);
    std::fwrite("data", 1, 4, f);
    u32(bytes);
    for (float v : x) {
        const int16_t s = static_cast<int16_t>(std::lround(std::fmax(-1.f, std::fmin(1.f, v * g)) * 32767.f));
        std::fwrite(&s, 2, 1, f);
    }
    std::fclose(f);
    std::printf("  wrote %s\n", path.c_str());
    return true;
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "out/listen";
    bool              ok  = true;
    for (const Clip& c : clips()) ok = writeWav(dir + "/" + c.file + ".wav", renderClip(c, kFs)) && ok;
    return ok ? 0 : 1;
}
