#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include "../src/fleet.hpp"

#define CHECK(c) do { if (!(c)) { std::cerr << "FALHOU: " #c " (linha " << __LINE__ << ")\n"; return 1; } } while (0)

int main(int, char** argv) {
    std::ifstream f{std::string(argv[1])};
    std::stringstream ss; ss << f.rdbuf();
    std::string data = ss.str();
    CHECK(sii::detectKind(data) == sii::FileKind::Text);
    CHECK(sii::detectKind("ScsC\x01\x02") == sii::FileKind::BinaryOrEncrypted);
    auto doc = sii::parse(data);
    CHECK(doc.units.size() == 10);
    auto rep = fleet::extract(doc);
    CHECK(rep.playerTruck == "_nameless.v0");
    CHECK(rep.drivers.size() == 1);
    const auto& d = rep.drivers[0];
    CHECK(d.unitName == "driver.carlos");
    CHECK(d.garage == "garage.berlin" && d.garageSlot == 0);
    CHECK(d.trucks.size() == 1 && d.trucks[0].first == "assigned_truck");
    CHECK(d.trucks[0].second.brandModel == "mercedes.axor");
    CHECK(d.trucks[0].second.accessories.size() == 3);
    CHECK(d.trucks[0].second.licensePlate == "B LF 2026|germany");
    CHECK(d.jobs.size() == 1 && d.jobs[0].second.fields.size() == 6);
    std::string txt = fleet::formatReport(doc, rep, "teste");
    CHECK(txt.find("target_company: company.volatile.stokes.berlin") != std::string::npos);
    bool threw = false;
    try { sii::parse("SiiNunit\n{\nx : y {\n semdoispontos\n}\n}\n"); } catch (const std::exception&) { threw = true; }
    CHECK(threw);
    std::cout << "Todos os testes passaram\n";
    return 0;
}
