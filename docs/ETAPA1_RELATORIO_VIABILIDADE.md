# Living Fleet – Real Drivers
## Etapa 1 – Relatório de viabilidade técnica

**Versão-alvo:** ETS2 1.61.1.9 · Windows 10/11 64 bits
**Data:** 18/11/2026
**Status:** investigação concluída · protótipo da Etapa 2 entregue · **nada foi testado ainda dentro do jogo**

---

## 0. Resumo

| Pergunta | Resposta curta | Confiança |
|---|---|---|
| Dá para saber quem são os motoristas, os caminhões e as entregas? | **Sim**, pelo save (`game.sii`). | Alta. Os nomes exatos dos campos na 1.61 precisam ser confirmados com o seu save |
| Dá para saber a configuração visual completa do caminhão de cada motorista? | **Sim.** O save guarda cada peça (chassi, cabine, motor, pintura, rodas, acessórios) como `data_path`. | Alta |
| O jogo tem uma posição real do motorista no mapa? | **Não.** O jogo não simula o motorista fisicamente, só o tempo e o lucro da entrega. | Alta |
| O SDK oficial consegue criar ou controlar veículos? | **Não.** O SDK só lê telemetria do caminhão do jogador e cria dispositivos de entrada virtuais. | Alta |
| É possível colocar um caminhão físico no mapa? | **Provavelmente sim, só com engenharia reversa.** Terceiros já fizeram coisas parecidas, mas não existe API pública para isso. | Média. Só vai ser comprovado na Etapa 4 |
| Fazer esse caminhão dirigir sozinho pelas estradas? | **Possível em teoria**, aproveitando a IA do tráfego. É o maior risco do projeto. | Baixa a média |

**Conclusão:** o projeto **é viável em princípio**, mas a parte central (Etapas 4 e 5) depende de engenharia reversa do executável do ETS2. Ninguém documentou isso publicamente para o que você quer. Por isso, a Etapa 4 é um **ponto de decisão**: se não conseguirmos fazer um caminhão aparecer fisicamente no jogo, as etapas seguintes não fazem sentido do jeito que você descreveu.

---

## 1. A – SDK oficial de telemetria

O SCS SDK (versão 1.14 do SDK) é uma DLL nativa carregada da pasta `bin/win_x64/plugins/` do jogo.

**O que ele lê:**
- Caminhão **do jogador**: posição no mundo (x, y, z e orientação), velocidade, RPM, marcha, combustível, desgaste, luzes, etc.
- Reboque(s) do jogador, trabalho atual **do jogador** (carga, origem, destino, receita, prazo) e a navegação do jogador (distância e tempo restantes, limite de velocidade).
- Eventos de jogo: trabalho entregue, trabalho cancelado, multa, pedágio, balsa e trem.
- Configuração do caminhão do jogador (marca, modelo, placa).

**O que ele também faz:**
- Desde a 1.14, cria **dispositivos de entrada virtuais**, ou seja, controles falsos que agem sobre o caminhão **do jogador**.

**O que ele NÃO faz (limitações confirmadas pela API):**
- Não lê nada sobre os motoristas contratados, o tráfego ou outros veículos.
- **Não cria, move nem controla nenhum veículo além do caminhão do jogador.**
- Não desenha nada na tela.

**Para que serve no Living Fleet:**
1. É a **forma oficial de carregar nosso código dentro do processo do jogo**. Um plugin do SDK é uma DLL que o jogo carrega por conta própria, e esse é o ponto de entrada para as etapas seguintes.
2. Fornece a **posição real do jogador**, necessária para os encontros e o comboio.
3. Avisa quando o jogo está pausado e quando a entrega do jogador termina.

> Observação: o site oficial da documentação (modding.scssoft.com) está bloqueado na rede desta sessão. As informações acima vêm dos cabeçalhos do SDK 1.14 publicados em espelhos e da documentação de projetos que usam o SDK. Antes da Etapa 4 vou conferir tudo nos cabeçalhos oficiais.

---

## 2. B – Arquivo de save (`game.sii`)

**Formato:** a partir de certa versão, o jogo grava o save em binário/criptografado (cabeçalhos `ScsC` ou `BSII`). Para obter texto legível:
- No console do jogo, digite `g_save_format 2` e salve o jogo; ou
- Em `config.cfg`, defina `uset g_save_format "2"`.

Também existem decodificadores de código aberto (SII_Decrypt e seus forks em Pascal, C# e Rust). Se for necessário, podem ser integrados mais tarde para você não precisar mudar a configuração.

**Estrutura (hipótese baseada em saves de versões anteriores, a confirmar com o seu save 1.61):**

```
player : _nameless.X       { trucks[], drivers[], assigned_truck ... }
garage : garage.berlin     { vehicles[i] <-> drivers[i] (vagas paralelas), status, productivity }
driver_ai : driver.carlos  { state, hometown, current_city, on_duty_timer, experience_points,
                             training_policy, driver_job -> job_info, assigned_truck -> vehicle ... }
job_info : _nameless.Y     { cargo, source_company, target_company, planned_distance_km, start_time ... }
vehicle : _nameless.Z      { accessories[] -> vehicle_*_accessory { data_path }, license_plate, odometer }
```

**Como ligar motorista → caminhão → entrega:**
1. Pelas referências diretas da unidade `driver_ai` (por exemplo `assigned_truck` e `driver_job`).
2. Pela vaga da garagem (`garage.drivers[i]` ↔ `garage.vehicles[i]`).

O protótipo da Etapa 2 tenta os dois caminhos e mostra no log qual deles encontrou cada relação.

**Configuração visual do caminhão:** cada peça é uma unidade `vehicle_accessory`, `vehicle_paint_job_accessory`, `vehicle_addon_accessory`, `vehicle_wheel_accessory`, etc., com o `data_path` da definição usada. **Isso é tudo o que o jogo precisa para montar o caminhão exatamente como você personalizou.** É o mesmo tipo de dado que o Convoy envia para mostrar o caminhão de outro jogador.

**Limitações do save:**
- O save é uma **foto do momento**: só é atualizado quando o jogo salva (autosave, salvar manual, sono). Para dados ao vivo, a mesma informação terá que ser lida da memória do jogo (Etapa 3 em diante).
- O nome exibido do motorista ("Carlos Henrique") não fica no save. O save guarda um ID (ex.: `driver.carlos`), e o nome está nos arquivos `def` do jogo.

---

## 3. Sobre "posição real" do motorista (importante)

Você pediu "não quero posições inventadas". É preciso deixar claro:

**O ETS2 não tem uma posição física para o motorista contratado.** Internamente, a entrega dele é só "começou no tempo T, distância D, termina no tempo T+X". Não existe um ponto no mapa onde ele "está".

A posição mais fiel possível é, portanto, uma **posição derivada dos dados do próprio jogo**:
- origem e destino reais da entrega;
- rota calculada pelo **mesmo sistema de navegação do jogo** (o GPS), não uma linha reta;
- avanço na rota = (tempo atual do jogo − início) ÷ duração prevista.

O resultado não é aleatório nem inventado: é determinístico e coerente com o que o jogo registra. Mas é uma **projeção**, e o log e a interface vão sempre identificá-la assim ("posição projetada"), separada da posição física do caminhão depois que ele existir no mundo.

---

## 4. C – Sistema de veículos do ETS2

**API pública para criar e controlar veículos:** **não existe.** Nem o SDK, nem os mods `.scs`, nem o console oferecem isso.

**Evidências de que o motor do jogo consegue fazer isso (por meios não oficiais):**

| Projeto | O que comprova | Código aberto? |
|---|---|---|
| **Convoy oficial** (desde a 1.41, ampliado na 1.50) | O motor desenha caminhões "remotos" com a personalização completa de outro jogador | Não (é do próprio jogo) |
| **TruckersMP** | Há anos mostra caminhões de outros jogadores injetando código no jogo | Não |
| **ETS2LA plugin** (fork do *ts-extra-utilities*) | Uma DLL lê da memória do jogo os ~40 veículos de tráfego mais próximos (posição, rotação, tamanho), os veículos estacionados, os semáforos e **a rota de navegação** | **Sim (MIT)** |
| **ts-extra-utilities** (dariowouters) | Uma DLL carregada como plugin do SDK **modifica objetos do mundo** (rodas direcionais e desengate de reboques), só no single-player e com DirectX 11 | Sim |

Os dois projetos de código aberto mostram a técnica que precisaremos: plugin do SDK + **busca de assinaturas** no executável + leitura e escrita de estruturas internas do jogo.

**Duas abordagens para a Etapa 4:**

**C1 – Assumir um veículo do tráfego de IA (recomendada para testar primeiro)**
- O jogo já cria caminhões de IA perto do jogador, e eles já respeitam faixas, cruzamentos, distância e colisões.
- A ideia é encontrar (ou provocar) a criação de um caminhão de tráfego e **trocar o modelo e as peças dele** pelos do caminhão do motorista.
- Vantagem: a IA de direção vem pronta, o que cobre boa parte do item 5 dos requisitos.
- Riscos: a IA de tráfego anda sem destino e desaparece longe do jogador. Precisaremos controlar a rota e impedir que ele suma, e os veículos de tráfego podem não aceitar todas as peças de um caminhão de jogador.

**C2 – Criar um veículo "remoto" como o do Convoy**
- Reaproveitar o caminho de código que o jogo usa para desenhar outros jogadores, que já aceita a personalização completa.
- Vantagem: o visual fica idêntico ao seu caminhão.
- Risco: um veículo remoto **não tem IA**, porque só replica uma posição recebida pela rede. Nós teríamos que dirigir o caminhão: calcular a trajetória pelas faixas da rota, curvas, velocidade e distância dos outros carros. É muito mais trabalho.

Uma combinação também é possível: aparência via C2 e trajetória calculada a partir das faixas que o tráfego de IA usa.

---

## 5. D – Arquitetura recomendada

```
┌──────────────────────────── processo do ETS2 ─────────────────────────────┐
│  living_fleet.dll  (plugin nativo C++, carregado pelo SDK oficial)        │
│   ├─ Telemetria (SDK)        → posição do jogador, pausa, eventos         │
│   ├─ Leitor de save/memória  → motoristas, caminhões, entregas            │
│   ├─ Projeção de rota        → onde cada motorista deveria estar          │
│   ├─ Ganchos de engenharia reversa (assinaturas, nunca endereços fixos)   │
│   │    → criar/alterar veículo, ler rota, comandar IA                     │
│   └─ Interface no jogo (Dear ImGui sobre DirectX 11)                      │
└───────────────────────────────────────────────────────────────────────────┘
          ▲ reaproveita
tools/fleet_reader  (C++, o mesmo leitor SII, executável separado para testes)
```

| Escolha | Por quê |
|---|---|
| **C++** | Plugins do SDK são DLLs nativas; ganchos na memória exigem código nativo no mesmo processo; os projetos de referência (ts-extra-utilities, ETS2LA plugin) também são C++. |
| **Leitor SII em C++ separado** | O mesmo código é usado no `fleet_reader.exe` (testável sem abrir o jogo) e depois dentro da DLL. |
| **Sem aplicativo C# por enquanto** | Não há necessidade: uma janela externa não ajuda a colocar o caminhão no mapa. Pode ser reavaliado para a interface depois. |
| **Mod `.scs` complementar** | Só se for necessário, por exemplo para definir uma placa com o nome do motorista ou um ícone. Não é necessário para o núcleo. |
| **Interface com ImGui sobre DirectX 11** | É a forma usada por outros plugins. Só funciona com DX11, que é o padrão do jogo. |

---

## 6. Riscos e regras de segurança

| Risco | Medida |
|---|---|
| **Cada atualização do ETS2 pode quebrar o plugin** | Usar busca por assinaturas em vez de endereços fixos; verificar a versão do jogo na inicialização e **desativar tudo** se ela não for reconhecida. |
| **Travamentos do jogo** | Toda escrita na memória é protegida e pode ser desligada por configuração; testes começam só lendo dados. |
| **Corromper o save** | **O Living Fleet nunca escreve no save.** O `fleet_reader` só abre o arquivo para leitura. |
| **Multiplayer (Convoy / TruckersMP)** | O plugin detecta o modo multiplayer e **não ativa nada**. É exclusivo para single-player. |
| **Contas e conquistas** | Sem efeito em contas. Mesmo assim, recomendo testar num **perfil separado** ou com cópia de segurança dos saves. |
| **Termos de uso** | Não verifiquei o EULA da SCS sobre injeção de código. Projetos como os citados existem há anos, mas a decisão de usar é sua. |
| **Divergência no comboio** | No comboio, o caminhão físico pode andar mais rápido ou mais devagar do que a previsão do jogo. A entrega oficial continua terminando quando o jogo decidir. Detalhes na seção 7. |

---

## 7. Sincronização com o jogo original

- **O jogo continua sendo a fonte da verdade.** O Living Fleet não cria motoristas, caminhões nem entregas; só **mostra** o que o jogo já está fazendo. Assim não há duplicação nem impacto na economia.
- O caminhão físico é uma **representação** da entrega em andamento. Ele só existe quando o motorista está em entrega e a posição projetada está dentro da área carregada perto do jogador.
- **Limitação conhecida:** o jogo decide quando a entrega termina com base no tempo. Se durante o comboio o caminhão físico atrasar (por exemplo, porque você parou num posto), ele estará "atrás" da previsão. Opções: (a) ajustar a velocidade dentro de uma margem; (b) ao sair do campo de visão, voltar para a posição projetada; (c) com mais engenharia reversa, alterar o tempo da entrega no jogo, o que é mais arriscado e fica para avaliar depois.
- **Garagens (Etapa 8):** o jogo já mostra veículos estacionados (o ETS2LA lê essa lista). Ainda é preciso investigar se dá para controlar quais caminhões aparecem estacionados nas suas garagens.

---

## 8. Plano por etapas com pontos de decisão

| Etapa | Entrega | Como provamos | Decisão |
|---|---|---|---|
| 1. Investigação | Este relatório | — | ✅ concluída |
| 2. Leitura da frota | `fleet_reader.exe` + log | **Você roda no seu save 1.61** e me manda o log | Ajustar os campos à estrutura real |
| 3. Localização | Plugin DLL lê a posição do jogador (SDK) + projeção da posição de cada motorista na rota | Log ao vivo: "Carlos: posição projetada X km de você" | — |
| **4a.** | Plugin lê a lista de veículos de tráfego da memória (técnica do ETS2LA) | Log com veículos próximos batendo com o que você vê na tela | Se falhar: rever a abordagem |
| **4b.** | Alterar a aparência de **um** veículo de tráfego | Vídeo/print do caminhão alterado no jogo | — |
| **4c.** | Caminhão com a configuração completa do motorista no mapa | **Caminhão visível no jogo** | **Decisão principal do projeto** |
| 5–9 | Movimentação, encontros, comboio, garagens, testes | Cada uma provada dentro do jogo | — |

---

## 9. O que eu consigo e não consigo fazer daqui

- **Consigo:** escrever o código, compilar o `.exe`/`.dll` para Windows, pesquisar, analisar o seu save e os logs.
- **Não consigo:** rodar o ETS2. Estou num servidor Linux sem o jogo. **Todo teste dentro do jogo será feito por você.** Na engenharia reversa (Etapa 4), você também vai precisar rodar ferramentas como x64dbg, Cheat Engine ou Ghidra e me mandar os resultados, ou me enviar uma cópia do `eurotrucks2.exe` para análise estática.

---

## Fontes

- [Documentação do SDK de telemetria (SCS)](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry) · [Changelog do SDK](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry/Changelog)
- [Cabeçalhos do SDK 1.14 (espelho)](https://rust-digger.code-maven.com/src/scs-sdk-rs-0.2.2/sdk/include/scssdk_telemetry_event.h)
- [SII_Decrypt](https://github.com/Tommylans/SII_Decrypt) · [decrypt_truck (Rust)](https://docs.rs/crate/decrypt_truck/1.3.6)
- [ETS2LA plugin (GitLab, MIT)](https://gitlab.com/ETS2LA/ets2la_plugin) · [Desenvolvimento de plugins ETS2LA](https://ets2la.com/developers/plugin_development)
- [ts-extra-utilities](https://github.com/dariowouters/ts-extra-utilities)
- [Convoy (modo de jogo)](https://trucksimulator.wiki.gg/wiki/Convoy_(game_mode)) · [Beta do Convoy 1.41](https://forum.scssoft.com/viewtopic.php?p=1560701) · [Patch 1.50](https://trucksimulator.wiki.gg/wiki/Patch_Notes/Euro_Truck_Simulator_2/Update_1.50)
- [TruckersMP](https://trucksimulator.wiki.gg/wiki/TruckersMP)
