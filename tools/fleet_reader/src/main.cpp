// Living Fleet - fleet_reader
// Uso: fleet_reader.exe [caminho\game.sii] [caminho\log.txt]
// Sem argumentos: procura o game.sii mais recente em Documentos\Euro Truck Simulator 2.
// O save nunca e modificado: o arquivo e aberto apenas para leitura.
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "fleet.hpp"
#include "sii.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

static fs::path findNewestSave() {
    std::vector<fs::path> roots;
#ifdef _WIN32
    if (const char* up = std::getenv("USERPROFILE")) {
        roots.push_back(fs::path(up) / "Documents" / "Euro Truck Simulator 2");
        roots.push_back(fs::path(up) / "OneDrive" / "Documents" / "Euro Truck Simulator 2");
        roots.push_back(fs::path(up) / "OneDrive" / "Documentos" / "Euro Truck Simulator 2");
    }
#else
    if (const char* home = std::getenv("HOME"))
        roots.push_back(fs::path(home) / ".local/share/Euro Truck Simulator 2");
#endif
    fs::path best;
    fs::file_time_type bestTime{};
    for (const auto& root : roots) {
        for (const char* profiles : {"profiles", "steam_profiles"}) {
            fs::path p = root / profiles;
            std::error_code ec;
            if (!fs::is_directory(p, ec)) continue;
            for (auto it = fs::recursive_directory_iterator(p, ec); it != fs::recursive_directory_iterator();
                 it.increment(ec)) {
                if (ec) break;
                if (it->path().filename() != "game.sii") continue;
                auto t = fs::last_write_time(it->path(), ec);
                if (!ec && (best.empty() || t > bestTime)) { best = it->path(); bestTime = t; }
            }
        }
    }
    return best;
}

// Pasta onde esta o fleet_reader.exe (o log e salvo ali, mesmo ao arrastar o save sobre o .exe).
static fs::path exeDir(const char* argv0) {
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) return fs::path(buf).parent_path();
#endif
    std::error_code ec;
    fs::path p = fs::absolute(fs::u8path(argv0), ec);
    return ec ? fs::current_path() : p.parent_path();
}

static int finish(int code) {
#ifdef _WIN32
    // Executado com duplo clique: mantem a janela aberta para o usuario ler.
    std::cout << "\nPressione ENTER para sair...";
    std::cin.get();
#endif
    return code;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    fs::path input = argc > 1 ? fs::u8path(argv[1]) : findNewestSave();
    if (input.empty()) {
        std::cerr << "Nenhum game.sii encontrado. Arraste o arquivo game.sii sobre o fleet_reader.exe\n"
                     "ou rode: fleet_reader.exe \"C:\\caminho\\game.sii\"\n";
        return finish(1);
    }
    fs::path output = argc > 2 ? fs::u8path(argv[2]) : exeDir(argv[0]) / "living_fleet_log.txt";

    std::ifstream f(input, std::ios::binary);
    if (!f) {
        std::cerr << "Nao foi possivel abrir: " << input.u8string() << "\n";
        return finish(1);
    }
    std::stringstream ss;
    ss << f.rdbuf();
    std::string data = ss.str();

    switch (sii::detectKind(data)) {
        case sii::FileKind::BinaryOrEncrypted:
            std::cerr << "O save esta em formato binario/criptografado.\n"
                         "No jogo: abra o console e digite  g_save_format 2  e depois salve o jogo\n"
                         "(ou edite config.cfg: uset g_save_format \"2\"). Entao rode novamente.\n";
            return finish(2);
        case sii::FileKind::Unknown:
            std::cerr << "Arquivo nao reconhecido como save SII: " << input.u8string() << "\n";
            return finish(2);
        case sii::FileKind::Text: break;
    }

    try {
        sii::Document doc = sii::parse(data);
        fleet::Report rep = fleet::extract(doc);
        std::string text = fleet::formatReport(doc, rep, input.u8string());
        std::ofstream out(output, std::ios::binary);
        out << text;
        std::cout << text << "\nLog salvo em: " << output.u8string() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Erro ao ler o save: " << e.what() << "\n";
        return finish(3);
    }
    return finish(0);
}
