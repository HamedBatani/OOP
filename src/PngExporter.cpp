#include "PngExporter.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <vector>

namespace {
void u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 24)); out.push_back(static_cast<std::uint8_t>(value >> 16));
    out.push_back(static_cast<std::uint8_t>(value >> 8)); out.push_back(static_cast<std::uint8_t>(value));
}
std::uint32_t crc32(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < size; ++i) { crc ^= data[i]; for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u))); }
    return ~crc;
}
void chunk(std::vector<std::uint8_t>& png, const char type[4], const std::vector<std::uint8_t>& data) {
    u32(png, static_cast<std::uint32_t>(data.size())); const std::size_t start = png.size();
    png.insert(png.end(), type, type + 4); png.insert(png.end(), data.begin(), data.end()); u32(png, crc32(png.data() + start, data.size() + 4));
}
std::uint32_t adler32(const std::vector<std::uint8_t>& data) {
    std::uint32_t a = 1, b = 0; for (auto byte : data) { a = (a + byte) % 65521; b = (b + a) % 65521; } return (b << 16) | a;
}
}

bool PngExporter::saveRendererRegion(SDL_Renderer* renderer, const SDL_Rect& region,
                                     const std::string& filename, std::string* error) {
    SDL_Surface* captured = SDL_RenderReadPixels(renderer, &region);
    if (!captured) { if (error) *error = SDL_GetError(); return false; }
    SDL_Surface* rgba = SDL_ConvertSurface(captured, SDL_PIXELFORMAT_RGBA32); SDL_DestroySurface(captured);
    if (!rgba) { if (error) *error = SDL_GetError(); return false; }
    const int width = rgba->w, height = rgba->h; std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<std::size_t>(height) * (1u + static_cast<std::size_t>(width) * 4u));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0); const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
        raw.insert(raw.end(), row, row + static_cast<std::size_t>(width) * 4u);
    }
    SDL_DestroySurface(rgba);
    std::vector<std::uint8_t> zlib{0x78, 0x01};
    std::size_t offset = 0;
    while (offset < raw.size()) {
        const std::size_t length = std::min<std::size_t>(65535, raw.size() - offset); const bool final = offset + length == raw.size();
        zlib.push_back(final ? 1 : 0); zlib.push_back(static_cast<std::uint8_t>(length)); zlib.push_back(static_cast<std::uint8_t>(length >> 8));
        const std::uint16_t inverse = static_cast<std::uint16_t>(~length); zlib.push_back(static_cast<std::uint8_t>(inverse)); zlib.push_back(static_cast<std::uint8_t>(inverse >> 8));
        zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset), raw.begin() + static_cast<std::ptrdiff_t>(offset + length)); offset += length;
    }
    u32(zlib, adler32(raw));
    std::vector<std::uint8_t> png{137,80,78,71,13,10,26,10}, ihdr; u32(ihdr, width); u32(ihdr, height);
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0}); chunk(png, "IHDR", ihdr); chunk(png, "IDAT", zlib); chunk(png, "IEND", {});
    std::ofstream file(filename, std::ios::binary | std::ios::trunc); if (!file) { if (error) *error = "Cannot open export file."; return false; }
    file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size())); return file.good();
}
