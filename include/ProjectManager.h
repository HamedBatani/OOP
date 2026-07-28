#pragma once

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "ComponentInstance.h"
#include "Wire.h"

class ProjectManager {
public:
    static std::string serialize(const std::vector<std::string>& active,
                                 const std::vector<ComponentInstance>& components,
                                 const std::vector<Wire>& wires) {
        std::ostringstream out;
        out << "OOP_CIRCUIT 2\n" << "ACTIVE " << active.size() << '\n';
        for (const auto& item : active) out << std::quoted(item) << '\n';
        out << "COMPONENTS " << components.size() << '\n';
        out << std::setprecision(9);
        for (const auto& c : components) {
            out << std::quoted(c.type) << ' ' << std::quoted(c.labelId) << ' ' << std::quoted(c.valueStr) << ' '
                << c.worldPos.x << ' ' << c.worldPos.y << ' ' << c.rotationDegrees << ' '
                << c.isMirroredH << ' ' << c.isMirroredV << ' ' << c.interactiveStateBool << ' '
                << c.ledColorMode << ' ' << static_cast<unsigned>(c.activeSevenSegmentByte) << ' '
                << c.propagationDelayNs << ' ' << c.inputCount << ' ' << c.adcResolutionBits << ' '
                << c.adcConversionDelayNs << ' ' << c.dacResolutionBits << ' ' << c.dacConversionDelayNs << ' '
                << c.potWiperPosition << ' ' << c.lcdCursorRow << ' ' << c.lcdCursorCol << ' '
                << std::quoted(c.lcdLines.size() > 0 ? c.lcdLines[0] : "") << ' '
                << std::quoted(c.lcdLines.size() > 1 ? c.lcdLines[1] : "") << ' '
                << (c.externalMemory ? c.externalMemory->size() : 0) << '\n';
        }
        out << "WIRES " << wires.size() << '\n';
        for (const auto& w : wires) {
            out << std::quoted(w.uid) << ' ' << std::quoted(w.startCompId) << ' ' << std::quoted(w.startPinName) << ' '
                << std::quoted(w.endCompId) << ' ' << std::quoted(w.endPinName) << ' ' << w.isCompleted << ' '
                << static_cast<int>(w.startAnchor.kind) << ' ' << std::quoted(w.startAnchor.lockedCompId) << ' '
                << std::quoted(w.startAnchor.lockedPinName) << ' ' << std::quoted(w.startAnchor.lockedWireUid) << ' '
                << w.startAnchor.lockedSegmentIndex << ' ' << w.startAnchor.lockedSegmentT << ' '
                << w.startAnchor.cachedWorldPos.x << ' ' << w.startAnchor.cachedWorldPos.y << ' '
                << static_cast<int>(w.endAnchor.kind) << ' ' << std::quoted(w.endAnchor.lockedCompId) << ' '
                << std::quoted(w.endAnchor.lockedPinName) << ' ' << std::quoted(w.endAnchor.lockedWireUid) << ' '
                << w.endAnchor.lockedSegmentIndex << ' ' << w.endAnchor.lockedSegmentT << ' '
                << w.endAnchor.cachedWorldPos.x << ' ' << w.endAnchor.cachedWorldPos.y << ' '
                << w.routingPoints.size();
            for (const auto& point : w.routingPoints) out << ' ' << point.x << ' ' << point.y;
            out << '\n';
        }
        const std::string snapshot = out.str();
        std::ostringstream json;
        json << "{\n"
             << "  \"format\": \"OOP_CIRCUIT\",\n"
             << "  \"version\": 3,\n"
             << "  \"componentCount\": " << components.size() << ",\n"
             << "  \"wireCount\": " << wires.size() << ",\n"
             << "  \"snapshot\": \"" << escapeJson(snapshot) << "\"\n"
             << "}\n";
        return json.str();
    }

    static bool deserialize(const std::string& data, std::vector<std::string>& active,
                            std::vector<ComponentInstance>& components, std::vector<Wire>& wires,
                            std::string* error = nullptr) {
        std::string payload = data;
        const auto first = data.find_first_not_of(" \t\r\n");
        if (first != std::string::npos && data[first] == '{') {
            if (!extractJsonString(data, "snapshot", payload)) return fail(error, "Damaged JSON project file.");
        }
        std::istringstream in(payload);
        std::string token; int version = 0; std::size_t count = 0;
        if (!(in >> token >> version) || token != "OOP_CIRCUIT" || version != 2) return fail(error, "Unsupported or damaged project format.");
        std::vector<std::string> newActive; std::vector<ComponentInstance> newComponents; std::vector<Wire> newWires;
        if (!(in >> token >> count) || token != "ACTIVE") return fail(error, "Missing active component list.");
        for (std::size_t i = 0; i < count; ++i) { std::string item; if (!(in >> std::quoted(item))) return fail(error, "Damaged active list."); newActive.push_back(item); }
        if (!(in >> token >> count) || token != "COMPONENTS") return fail(error, "Missing components section.");
        for (std::size_t i = 0; i < count; ++i) {
            std::string type, label, value, lcd0, lcd1; float x, y, propagation, adcDelay, dacDelay, pot; int rotation, led, segment, inputs, adcBits, dacBits, row, col; bool mirrorH, mirrorV, interactive; std::size_t memorySize;
            if (!(in >> std::quoted(type) >> std::quoted(label) >> std::quoted(value) >> x >> y >> rotation
                     >> mirrorH >> mirrorV >> interactive >> led >> segment >> propagation >> inputs
                     >> adcBits >> adcDelay >> dacBits >> dacDelay >> pot >> row >> col
                     >> std::quoted(lcd0) >> std::quoted(lcd1) >> memorySize)) return fail(error, "Damaged component record.");
            ComponentInstance c(type, label, value, {x, y});
            c.rotationDegrees = rotation; c.isMirroredH = mirrorH; c.isMirroredV = mirrorV; c.interactiveStateBool = interactive;
            c.ledColorMode = led; c.activeSevenSegmentByte = static_cast<std::uint8_t>(segment); c.propagationDelayNs = propagation;
            c.inputCount = inputs; c.adcResolutionBits = adcBits; c.adcConversionDelayNs = adcDelay; c.dacResolutionBits = dacBits; c.dacConversionDelayNs = dacDelay;
            c.potWiperPosition = pot; c.lcdCursorRow = row; c.lcdCursorCol = col; c.lcdLines = {lcd0, lcd1};
            if (type == "ADC") c.worldHeight = std::max(70.0f, static_cast<float>(adcBits) * 15.0f);
            if (type == "DAC") c.worldHeight = std::max(70.0f, static_cast<float>(dacBits) * 15.0f);
            if (type == "External Memory" && memorySize > 0) c.externalMemory = std::make_shared<ExternalMemory>(memorySize);
            c.initializeBasePins(); c.updatePinPositions(); newComponents.push_back(std::move(c));
        }
        if (!(in >> token >> count) || token != "WIRES") return fail(error, "Missing wires section.");
        for (std::size_t i = 0; i < count; ++i) {
            std::string uid, sc, sp, ec, ep; bool completed; int sk, ek; std::size_t points;
            WireAnchor sa, ea;
            if (!(in >> std::quoted(uid) >> std::quoted(sc) >> std::quoted(sp) >> std::quoted(ec) >> std::quoted(ep) >> completed
                     >> sk >> std::quoted(sa.lockedCompId) >> std::quoted(sa.lockedPinName) >> std::quoted(sa.lockedWireUid)
                     >> sa.lockedSegmentIndex >> sa.lockedSegmentT >> sa.cachedWorldPos.x >> sa.cachedWorldPos.y
                     >> ek >> std::quoted(ea.lockedCompId) >> std::quoted(ea.lockedPinName) >> std::quoted(ea.lockedWireUid)
                     >> ea.lockedSegmentIndex >> ea.lockedSegmentT >> ea.cachedWorldPos.x >> ea.cachedWorldPos.y >> points)) return fail(error, "Damaged wire record.");
            sa.kind = static_cast<AnchorKind>(sk); ea.kind = static_cast<AnchorKind>(ek);
            Wire w(sc, sp, sa.cachedWorldPos); w.uid = uid; w.endCompId = ec; w.endPinName = ep; w.isCompleted = completed; w.startAnchor = sa; w.endAnchor = ea; w.routingPoints.clear();
            for (std::size_t p = 0; p < points; ++p) { Point point; if (!(in >> point.x >> point.y)) return fail(error, "Damaged wire route."); w.routingPoints.push_back(point); }
            newWires.push_back(std::move(w));
        }
        active = std::move(newActive); components = std::move(newComponents); wires = std::move(newWires); return true;
    }

    static bool saveProject(const std::string& filename, const std::vector<std::string>& active,
                            const std::vector<ComponentInstance>& components, const std::vector<Wire>& wires) {
        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        if (!file) return false;
        const std::string data = serialize(active, components, wires); file.write(data.data(), static_cast<std::streamsize>(data.size())); return file.good();
    }
    static bool loadProject(const std::string& filename, std::vector<std::string>& active,
                            std::vector<ComponentInstance>& components, std::vector<Wire>& wires) {
        std::ifstream file(filename, std::ios::binary); if (!file) return false;
        std::ostringstream data; data << file.rdbuf(); return deserialize(data.str(), active, components, wires);
    }
private:
    static std::string escapeJson(const std::string& value) {
        std::string result; result.reserve(value.size() + 32);
        for (unsigned char ch : value) {
            switch (ch) {
                case '\\': result += "\\\\"; break;
                case '"': result += "\\\""; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += static_cast<char>(ch); break;
            }
        }
        return result;
    }
    static bool extractJsonString(const std::string& json, const std::string& key, std::string& value) {
        const std::string marker = "\"" + key + "\"";
        std::size_t pos = json.find(marker); if (pos == std::string::npos) return false;
        pos = json.find(':', pos + marker.size()); if (pos == std::string::npos) return false;
        pos = json.find('"', pos + 1); if (pos == std::string::npos) return false;
        ++pos; value.clear();
        while (pos < json.size()) {
            const char ch = json[pos++];
            if (ch == '"') return true;
            if (ch != '\\') { value += ch; continue; }
            if (pos >= json.size()) return false;
            const char escaped = json[pos++];
            switch (escaped) {
                case '"': value += '"'; break; case '\\': value += '\\'; break;
                case 'n': value += '\n'; break; case 'r': value += '\r'; break; case 't': value += '\t'; break;
                default: return false;
            }
        }
        return false;
    }
    static bool fail(std::string* error, const char* message) { if (error) *error = message; return false; }
};
