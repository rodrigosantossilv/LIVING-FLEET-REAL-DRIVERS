// Living Fleet - extracao da frota (motoristas, caminhoes, trabalhos) a partir de um game.sii.
#pragma once
#include <string>
#include <vector>

#include "sii.hpp"

namespace fleet {

struct Accessory {
    std::string unitType;  // ex.: vehicle_paint_job_accessory
    std::string dataPath;  // ex.: /def/vehicle/truck/mercedes.actros/chassis/4x2.sii
};

struct Truck {
    std::string unitName;
    std::string brandModel;  // extraido do data_path (ex.: "mercedes.actros")
    std::string licensePlate;
    std::string odometer;
    std::vector<Accessory> accessories;
};

struct Job {
    std::string unitName;
    std::vector<std::pair<std::string, std::string>> fields;  // todos os campos, como no save
};

struct Driver {
    std::string unitName;  // ID do motorista no save (o nome exibido vem dos arquivos def do jogo)
    std::string garage;    // garagem onde esta alocado (se encontrada)
    int garageSlot = -1;
    std::vector<std::pair<std::string, std::string>> keyFields;  // campos conhecidos/relevantes
    std::vector<std::pair<std::string, Truck>> trucks;           // atributo -> caminhao referenciado
    std::vector<std::pair<std::string, std::string>> trailers;   // atributo -> unidade do reboque
    std::vector<std::pair<std::string, Job>> jobs;               // atributo -> trabalho referenciado
    std::string garageSlotTruck;  // caminhao no mesmo slot da garagem (hipotese de associacao)
};

struct Report {
    std::vector<Driver> drivers;
    std::vector<std::string> warnings;
    std::string playerTruck;
    size_t unitCount = 0;
};

Truck readTruck(const sii::Document& doc, const std::string& unitName);
Report extract(const sii::Document& doc);

// Gera o texto do log. "rawDriverDump" inclui todos os atributos do 1o motorista,
// util para descobrirmos a estrutura real do save na versao 1.61.
std::string formatReport(const sii::Document& doc, const Report& r, const std::string& sourcePath);

}  // namespace fleet
