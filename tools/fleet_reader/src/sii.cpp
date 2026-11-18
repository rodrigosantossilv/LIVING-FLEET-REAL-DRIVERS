#include "sii.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace sii {

const std::string* Unit::get(const std::string& key) const {
    auto it = values.find(key);
    return it == values.end() ? nullptr : &it->second;
}

const std::vector<std::string>* Unit::getArray(const std::string& key) const {
    auto it = arrays.find(key);
    return it == arrays.end() ? nullptr : &it->second;
}

const Unit* Document::find(const std::string& name) const {
    auto it = byName.find(name);
    return it == byName.end() ? nullptr : &units[it->second];
}

std::vector<const Unit*> Document::ofType(const std::string& type) const {
    std::vector<const Unit*> out;
    for (const auto& u : units)
        if (u.type == type) out.push_back(&u);
    return out;
}

FileKind detectKind(const std::string& data) {
    size_t i = 0;
    if (data.size() >= 3 && (unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF)
        i = 3;  // BOM UTF-8
    std::string head = data.substr(i, 8);
    if (head.rfind("SiiNunit", 0) == 0) return FileKind::Text;
    // "ScsC" = criptografado; "BSII" = binario; "3nK" = texto ofuscado.
    if (head.rfind("ScsC", 0) == 0 || head.rfind("BSII", 0) == 0 || head.rfind("3nK", 0) == 0)
        return FileKind::BinaryOrEncrypted;
    return FileKind::Unknown;
}

namespace {

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Remove comentarios "#" e "//" que estejam fora de aspas.
std::string stripComment(const std::string& line) {
    bool inQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"' && (i == 0 || line[i - 1] != '\\')) inQuote = !inQuote;
        if (inQuote) continue;
        if (c == '#') return line.substr(0, i);
        if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') return line.substr(0, i);
    }
    return line;
}

std::string unquote(const std::string& v) {
    if (v.size() >= 2 && v.front() == '"' && v.back() == '"') return v.substr(1, v.size() - 2);
    return v;
}

[[noreturn]] void fail(size_t lineNo, const std::string& msg) {
    std::ostringstream os;
    os << "SII linha " << lineNo << ": " << msg;
    throw std::runtime_error(os.str());
}

}  // namespace

Document parse(const std::string& text) {
    Document doc;
    std::istringstream in(text);
    std::string raw;
    size_t lineNo = 0;
    bool seenHeader = false, inBlockComment = false;
    int depth = 0;  // 0 = fora; 1 = dentro de SiiNunit; 2 = dentro de unidade
    Unit cur;

    while (std::getline(in, raw)) {
        ++lineNo;
        std::string line = raw;
        if (lineNo == 1 && line.size() >= 3 && (unsigned char)line[0] == 0xEF) line = line.substr(3);
        if (inBlockComment) {
            size_t e = line.find("*/");
            if (e == std::string::npos) continue;
            line = line.substr(e + 2);
            inBlockComment = false;
        }
        size_t bc = line.find("/*");
        if (bc != std::string::npos) {
            size_t e = line.find("*/", bc + 2);
            if (e == std::string::npos) {
                line = line.substr(0, bc);
                inBlockComment = true;
            } else {
                line = line.substr(0, bc) + line.substr(e + 2);
            }
        }
        line = trim(stripComment(line));
        if (line.empty()) continue;

        if (!seenHeader) {
            if (line != "SiiNunit") fail(lineNo, "esperado 'SiiNunit'");
            seenHeader = true;
            continue;
        }
        if (depth == 0) {
            if (line != "{") fail(lineNo, "esperado '{' apos SiiNunit");
            depth = 1;
            continue;
        }
        if (depth == 1) {
            if (line == "}") { depth = 0; continue; }
            // Cabecalho de unidade: "tipo : nome {" (o '{' pode vir na linha seguinte).
            bool hasBrace = line.back() == '{';
            std::string hdr = trim(hasBrace ? line.substr(0, line.size() - 1) : line);
            size_t colon = hdr.find(':');
            if (colon == std::string::npos) fail(lineNo, "cabecalho de unidade invalido: " + line);
            cur = Unit{};
            cur.type = trim(hdr.substr(0, colon));
            cur.name = trim(hdr.substr(colon + 1));
            if (!hasBrace) {
                std::string next;
                do {
                    if (!std::getline(in, next)) fail(lineNo, "fim inesperado do arquivo");
                    ++lineNo;
                    next = trim(stripComment(next));
                } while (next.empty());
                if (next != "{") fail(lineNo, "esperado '{' apos cabecalho de unidade");
            }
            depth = 2;
            continue;
        }
        // depth == 2: atributos
        if (line == "}") {
            if (!cur.name.empty()) doc.byName[cur.name] = doc.units.size();
            doc.units.push_back(std::move(cur));
            cur = Unit{};
            depth = 1;
            continue;
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos) fail(lineNo, "atributo sem ':' -> " + line);
        std::string key = trim(line.substr(0, colon));
        std::string val = unquote(trim(line.substr(colon + 1)));

        size_t br = key.find('[');
        if (br != std::string::npos && key.back() == ']') {
            std::string base = key.substr(0, br);
            std::string idx = key.substr(br + 1, key.size() - br - 2);
            auto& arr = cur.arrays[base];
            if (idx.empty()) {
                arr.push_back(val);
            } else {
                size_t n = std::stoul(idx);
                if (arr.size() <= n) arr.resize(n + 1);
                arr[n] = val;
            }
            if (cur.values.count(base) == 0 && std::find(cur.order.begin(), cur.order.end(), base) == cur.order.end())
                cur.order.push_back(base);
        } else {
            // "trucks: 3" declara o tamanho do array; guardamos tambem como valor simples.
            if (cur.values.count(key) == 0 && cur.arrays.count(key) == 0) cur.order.push_back(key);
            cur.values[key] = val;
        }
    }
    if (depth != 0) fail(lineNo, "arquivo terminou com bloco aberto");
    return doc;
}

}  // namespace sii
