#include "fleet.hpp"

#include <algorithm>
#include <set>
#include <sstream>

namespace fleet {

namespace {

// Campos de driver_ai que acreditamos ser relevantes. A lista e uma hipotese baseada em
// saves de versoes anteriores; o log tambem mostra TODOS os campos para confirmarmos na 1.61.
const char* kDriverKeys[] = {"state",          "hometown",           "current_city",    "on_duty_timer",
                             "experience_points", "training_policy",  "adr",             "long_dist",
                             "heavy",          "fragile",            "urgent",          "mechanical",
                             "extra_pay",      "driver_job",         "assigned_truck",  "adopted_truck",
                             "assigned_trailer", "adopted_trailer",  "old_hometown",    "profit_log"};

std::string brandModelFromPath(const std::string& p) {
    const std::string marker = "/def/vehicle/truck/";
    size_t i = p.find(marker);
    if (i == std::string::npos) return "";
    i += marker.size();
    size_t e = p.find('/', i);
    return p.substr(i, e == std::string::npos ? std::string::npos : e - i);
}

bool isNullRef(const std::string& v) { return v.empty() || v == "null"; }

}  // namespace

Truck readTruck(const sii::Document& doc, const std::string& unitName) {
    Truck t;
    t.unitName = unitName;
    const sii::Unit* u = doc.find(unitName);
    if (!u) return t;
    if (auto v = u->get("license_plate")) t.licensePlate = *v;
    if (auto v = u->get("odometer")) t.odometer = *v;
    if (auto acc = u->getArray("accessories")) {
        for (const auto& ref : *acc) {
            const sii::Unit* a = doc.find(ref);
            if (!a) continue;
            Accessory x;
            x.unitType = a->type;
            if (auto dp = a->get("data_path")) x.dataPath = *dp;
            if (t.brandModel.empty()) t.brandModel = brandModelFromPath(x.dataPath);
            t.accessories.push_back(x);
        }
    }
    return t;
}

static Job readJob(const sii::Document& doc, const std::string& unitName) {
    Job j;
    j.unitName = unitName;
    if (const sii::Unit* u = doc.find(unitName))
        for (const auto& k : u->order)
            if (auto v = u->get(k)) j.fields.emplace_back(k, *v);
    return j;
}

Report extract(const sii::Document& doc) {
    Report r;
    r.unitCount = doc.units.size();

    // Garagens: slots paralelos drivers[i] <-> vehicles[i] (hipotese a confirmar).
    struct Slot { std::string garage; int index; std::string vehicle; };
    std::map<std::string, Slot> slotOf;
    for (const sii::Unit* g : doc.ofType("garage")) {
        const auto* ds = g->getArray("drivers");
        const auto* vs = g->getArray("vehicles");
        if (!ds) continue;
        for (size_t i = 0; i < ds->size(); ++i) {
            if (isNullRef((*ds)[i])) continue;
            std::string veh = (vs && i < vs->size()) ? (*vs)[i] : "";
            slotOf[(*ds)[i]] = Slot{g->name, (int)i, isNullRef(veh) ? "" : veh};
        }
    }

    if (auto players = doc.ofType("player"); !players.empty())
        if (auto v = players[0]->get("assigned_truck")) r.playerTruck = *v;

    // Motoristas: unidades do tipo driver_ai.
    std::set<std::string> seen;
    for (const sii::Unit* d : doc.ofType("driver_ai")) {
        Driver drv;
        drv.unitName = d->name;
        seen.insert(d->name);
        for (const char* k : kDriverKeys)
            if (auto v = d->get(k)) drv.keyFields.emplace_back(k, *v);

        // Segue toda referencia do motorista e classifica pelo tipo da unidade apontada.
        for (const auto& [key, val] : d->values) {
            if (isNullRef(val)) continue;
            const sii::Unit* ref = doc.find(val);
            if (!ref) continue;
            if (ref->type == "vehicle") drv.trucks.emplace_back(key, readTruck(doc, val));
            else if (ref->type == "trailer") drv.trailers.emplace_back(key, val);
            else if (ref->type == "job_info") drv.jobs.emplace_back(key, readJob(doc, val));
        }

        if (auto it = slotOf.find(d->name); it != slotOf.end()) {
            drv.garage = it->second.garage;
            drv.garageSlot = it->second.index;
            drv.garageSlotTruck = it->second.vehicle;
        }
        r.drivers.push_back(std::move(drv));
    }

    // Motoristas listados em garagens mas sem unidade driver_ai propria.
    for (const auto& [name, slot] : slotOf)
        if (!seen.count(name))
            r.warnings.push_back("Motorista '" + name + "' aparece em " + slot.garage +
                                 " mas nao tem unidade driver_ai correspondente.");

    if (r.drivers.empty())
        r.warnings.push_back("Nenhuma unidade 'driver_ai' encontrada. O formato do save pode ter mudado "
                             "na 1.61 - envie o log para analisarmos.");
    return r;
}

std::string formatReport(const sii::Document& doc, const Report& r, const std::string& sourcePath) {
    std::ostringstream o;
    o << "==============================================\n"
      << " LIVING FLEET - Leitura da frota (Etapa 2)\n"
      << "==============================================\n"
      << "Arquivo lido : " << sourcePath << "\n"
      << "Unidades SII : " << r.unitCount << "\n"
      << "Motoristas   : " << r.drivers.size() << "\n"
      << "Caminhao do jogador: " << (r.playerTruck.empty() ? "(nao encontrado)" : r.playerTruck) << "\n"
      << "Obs.: o save NAO foi modificado. Este programa so le o arquivo.\n\n";

    // Contagem por tipo de unidade: ajuda a mapear o formato da 1.61.
    std::map<std::string, int> typeCount;
    for (const auto& u : doc.units) typeCount[u.type]++;
    o << "--- Tipos de unidade no save ---\n";
    for (const auto& [t, n] : typeCount) o << "  " << t << ": " << n << "\n";
    o << "\n";

    int idx = 1;
    for (const auto& d : r.drivers) {
        o << "### Motorista " << idx++ << ": " << d.unitName << "\n";
        if (!d.garage.empty())
            o << "  Garagem: " << d.garage << " (slot " << d.garageSlot << ")\n";
        for (const auto& [k, v] : d.keyFields) o << "  " << k << ": " << v << "\n";

        auto printTruck = [&](const std::string& label, const Truck& t) {
            o << "  [Caminhao via '" << label << "'] " << t.unitName << "\n"
              << "     Marca/modelo: " << (t.brandModel.empty() ? "?" : t.brandModel) << "\n"
              << "     Placa: " << (t.licensePlate.empty() ? "?" : t.licensePlate) << "\n"
              << "     Odometro: " << (t.odometer.empty() ? "?" : t.odometer) << "\n"
              << "     Pecas/acessorios (" << t.accessories.size() << "):\n";
            for (const auto& a : t.accessories) o << "       - " << a.unitType << "  " << a.dataPath << "\n";
        };
        for (const auto& [k, t] : d.trucks) printTruck(k, t);
        if (!d.garageSlotTruck.empty()) {
            bool already = std::any_of(d.trucks.begin(), d.trucks.end(),
                                       [&](const auto& p) { return p.second.unitName == d.garageSlotTruck; });
            if (!already) printTruck("garagem.vehicles[" + std::to_string(d.garageSlot) + "]",
                                     readTruck(doc, d.garageSlotTruck));
        }
        if (d.trucks.empty() && d.garageSlotTruck.empty()) o << "  (nenhum caminhao associado encontrado)\n";
        for (const auto& [k, t] : d.trailers) o << "  [Reboque via '" << k << "'] " << t << "\n";
        for (const auto& [k, j] : d.jobs) {
            o << "  [Trabalho via '" << k << "'] " << j.unitName << "\n";
            for (const auto& [fk, fv] : j.fields) o << "     " << fk << ": " << fv << "\n";
        }
        if (d.jobs.empty()) o << "  (sem trabalho ativo referenciado)\n";
        o << "\n";
    }

    if (!r.warnings.empty()) {
        o << "--- Avisos ---\n";
        for (const auto& w : r.warnings) o << "  ! " << w << "\n";
        o << "\n";
    }

    // Dump bruto do primeiro motorista: para mapear todos os campos reais da 1.61.
    if (auto ds = doc.ofType("driver_ai"); !ds.empty()) {
        const sii::Unit* d = ds[0];
        o << "--- Estrutura bruta do 1o motorista (para analise) ---\n"
          << d->type << " : " << d->name << "\n";
        for (const auto& k : d->order) {
            if (auto v = d->get(k)) o << "  " << k << ": " << *v << "\n";
            if (auto a = d->getArray(k))
                for (size_t i = 0; i < a->size(); ++i) o << "  " << k << "[" << i << "]: " << (*a)[i] << "\n";
        }
    }
    return o.str();
}

}  // namespace fleet
