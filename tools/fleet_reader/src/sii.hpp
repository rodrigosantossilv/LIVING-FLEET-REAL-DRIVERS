// Living Fleet - leitor de arquivos SII em texto (formato "SiiNunit").
// Somente leitura: este codigo nunca escreve no save do jogador.
#pragma once
#include <map>
#include <string>
#include <vector>

namespace sii {

struct Unit {
    std::string type;  // ex.: "driver_ai", "vehicle", "job_info"
    std::string name;  // ex.: "_nameless.1a2.b3c4" ou "driver.carlos"
    // Atributos simples: chave -> valor (texto bruto, aspas removidas).
    std::map<std::string, std::string> values;
    // Arrays: "trucks: 3" + "trucks[0]: ..." ou "trucks[]: ...".
    std::map<std::string, std::vector<std::string>> arrays;
    // Ordem original das chaves, para o log mostrar como no arquivo.
    std::vector<std::string> order;

    const std::string* get(const std::string& key) const;
    const std::vector<std::string>* getArray(const std::string& key) const;
};

struct Document {
    std::vector<Unit> units;
    std::map<std::string, size_t> byName;  // nome da unidade -> indice

    const Unit* find(const std::string& name) const;
    std::vector<const Unit*> ofType(const std::string& type) const;
};

enum class FileKind { Text, BinaryOrEncrypted, Unknown };

// Detecta pelo cabecalho se o save esta em texto ou em formato binario/criptografado.
FileKind detectKind(const std::string& data);

// Faz o parse do conteudo de texto. Lanca std::runtime_error com o numero da linha em caso de erro.
Document parse(const std::string& text);

}  // namespace sii
