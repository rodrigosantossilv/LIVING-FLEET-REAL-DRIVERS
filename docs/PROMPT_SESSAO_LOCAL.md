Você está trabalhando no projeto **Living Fleet — Real Drivers**, um plugin experimental para Euro Truck Simulator 2 (Windows). Esta sessão roda no meu PC.

## Contexto

- Repositório: https://github.com/rodrigosantossilv/LIVING-FLEET-REAL-DRIVERS. Clone-o (ou faça `git pull`) numa pasta de trabalho fora da pasta do jogo, por exemplo `C:\Users\rodri\Projetos\LIVING-FLEET-REAL-DRIVERS`.
- Antes de tudo, leia `docs/ETAPA1_RELATORIO_VIABILIDADE.md` e `docs/GUIA_ETAPA2_PASSO_A_PASSO.md`.
- O leitor de save já existe em `tools/fleet_reader/` (C++17, CMake). Há um binário pronto em `download/fleet_reader.exe`.
- Pasta de dados do jogo neste PC (no OneDrive): `C:\Users\rodri\OneDrive\Documentos\Euro Truck Simulator 2` (`config.cfg`, `profiles\`, `steam_profiles\`).
- Versão do jogo no repositório: ETS2 1.61.1.9. **Confirme a versão instalada** (pelo `game.log.txt` na pasta de dados) antes de qualquer teste.
- Problema atual: o `fleet_reader` disse que o save está em formato binário/criptografado, mesmo depois de eu digitar `g_save_format 2` no console.

## Intenção

Permitir que eu escolha um motorista contratado e encontre no mapa o caminhão atribuído a ele, para acompanhá-lo em comboio durante uma entrega.

## Metas

- Selecionar um motorista que já exista no meu save.
- Vincular o motorista ao caminhão que o jogo atribuiu a ele.
- Preservar o modelo e os acessórios que puderem ser reconstruídos com os dados do save.
- Fazer a representação física aparecer perto do jogador e acompanhar uma entrega.
- Tratar o resultado como experimental até ser validado dentro do jogo.

## Fora do escopo inicial

- Adicionar caminhões genéricos ao tráfego.
- Fazer todos os motoristas aparecerem ao mesmo tempo.
- Multiplayer, Convoy oficial ou TruckersMP.
- Prometer rota autônoma completa antes de provar que dá para criar um veículo com o jogo rodando.

## Abordagem

Plugin experimental em C++ para Windows (DLL do SDK oficial da SCS, em `bin\win_x64\plugins` na pasta de instalação do jogo). O leitor de save identifica motorista, caminhão, peças e entrega. Dentro do jogo, o plugin obtém a posição do jogador pelo SDK oficial e investiga a entidade de "caminhão remoto" (a do Convoy) como ponto de partida para criar a representação do motorista. A solução final deve usar o caminhão atribuído ao motorista, nunca um caminhão aleatório do tráfego.

O SDK oficial não tem API para criar veículos. A criação da entidade exige pesquisar as estruturas internas do jogo, pode falhar e pode quebrar em atualizações. Uma entidade remota pode não ter colisão nem IA de trânsito. Isso deve ser **medido, não presumido**.

## Fases (cada uma é um ponto de decisão: pare e me mostre o resultado antes de seguir)

1. **Integridade dos dados**
   - Confira `g_save_format` no `config.cfg`. Se precisar mudar, peça para eu fechar o jogo antes, porque o jogo regrava esse arquivo ao fechar.
   - Encontre o `game.sii` mais recente, rode o `fleet_reader` e mostre o `living_fleet_log.txt`.
   - Se o save continuar codificado, proponha (sem implementar ainda) suporte a decodificação no próprio leitor, usando como referência o SII_Decrypt, que é de código aberto.
   - Ajuste o leitor à estrutura real da versão instalada.
   - **Sucesso:** o relatório bate com a tela "Meus motoristas" do jogo, que eu vou conferir.
2. **Carregamento do plugin**
   - Obtenha os cabeçalhos do SDK oficial da SCS.
   - Compile uma DLL mínima (Visual Studio 2022 / CMake, x64) que registre telemetria básica do jogador num arquivo de log.
   - Antes de copiar a DLL para a pasta do jogo, me diga exatamente o que vai copiar e para onde.
   - **Sucesso:** o jogo carrega a DLL sem erro (evidência no `game.log.txt`) e o log mostra a posição do jogador.
3. **Prova de entidade**
   - Investigue como criar uma entidade remota no single-player, sem usar um caminhão do tráfego no lugar.
   - Antes de qualquer engenharia reversa, documente a técnica, os riscos e as ferramentas (x64dbg, Ghidra, Cheat Engine).
   - **Sucesso:** um caminhão visível perto do jogador, com pelo menos o modelo e a configuração do motorista escolhido.
   - **Se falhar:** pare, apresente o resultado e as limitações, e não comece o comboio.
4. **Comboio**
   - Ligue a entidade ao estado e à rota do motorista e faça ela acompanhar o jogador ou seguir a entrega.
   - **Sucesso:** deslocamento visível na estrada, sem teletransporte e sem perder a entidade durante uma sessão de teste.

## Segurança e compatibilidade

- O leitor de save continua **somente leitura**.
- Não altere saves nem arquivos do jogo, e não deixe alterações permanentes na memória do jogo, sem a minha autorização explícita.
- O plugin é só para single-player, desliga com um ajuste simples e se recusa a ativar em multiplayer.
- Não teste no Convoy nem no TruckersMP.
- Confirme a versão exata do jogo e guarde os logs antes de cada teste.
- Faça commits no repositório a cada fase concluída.

## Pendências

- Conseguir o `living_fleet_log.txt` e, se precisar, o `game.sii` real, para confirmar o formato do save.
- Investigar a entidade interna do Convoy e seus limites.
- O movimento final pode exigir simular a rota, porque o jogo não guarda uma posição física para o motorista contratado.

**Comece agora pela Fase 1.**
