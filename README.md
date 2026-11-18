# Living Fleet – Real Drivers (ETS2 1.61.1.9)

Mod/plugin para fazer os motoristas contratados do Euro Truck Simulator 2 existirem fisicamente no mapa.

- `docs/ETAPA1_RELATORIO_VIABILIDADE.md` – relatório de viabilidade (Etapa 1)
- `docs/GUIA_ETAPA2_PASSO_A_PASSO.md` – guia passo a passo para gerar o log da frota
- `tools/fleet_reader/` – Etapa 2: leitor somente leitura do `game.sii` que gera `living_fleet_log.txt`

## Usar o fleet_reader (Windows)
1. No jogo: console -> `g_save_format 2` -> salve o jogo.
2. Dê dois cliques em `fleet_reader.exe` (procura o save mais recente sozinho)
   ou arraste o `game.sii` sobre ele.
3. O arquivo `living_fleet_log.txt` aparece na mesma pasta.

## Compilar
Visual Studio 2022 (CMake) ou: `cmake -S tools/fleet_reader -B build && cmake --build build --config Release`
