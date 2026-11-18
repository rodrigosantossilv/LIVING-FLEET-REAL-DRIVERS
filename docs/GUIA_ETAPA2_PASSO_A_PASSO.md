# Guia passo a passo – Etapa 2: ler a sua frota

Objetivo: gerar o arquivo `living_fleet_log.txt` com os seus motoristas, caminhões e entregas, e me enviar.
Tempo estimado: 10 a 15 minutos.

> **Segurança:** o `fleet_reader.exe` só **lê** o save. Ele nunca altera, apaga ou grava nada nos arquivos do jogo.

---

## Passo 1 – Fazer backup dos seus saves (recomendado)

1. Feche o Euro Truck Simulator 2.
2. Abra o **Explorador de Arquivos** (atalho: tecla `Windows` + `E`).
3. Vá em **Documentos** → **Euro Truck Simulator 2**.
   - Se não encontrar, tente **OneDrive** → **Documentos** → **Euro Truck Simulator 2**.
4. Copie as pastas **`profiles`** e **`steam_profiles`** (a que existir) e cole numa pasta segura, por exemplo `Área de Trabalho\Backup ETS2`.

---

## Passo 2 – Fazer o jogo salvar em formato de texto

Normalmente o ETS2 salva num formato codificado que não dá para ler. Vamos mudar isso numa configuração.

1. **Com o jogo fechado**, abra a pasta **Documentos\Euro Truck Simulator 2** (a mesma do Passo 1).
2. Clique com o botão direito no arquivo **`config.cfg`** → **Abrir com** → **Bloco de Notas**.
3. Aperte `Ctrl` + `F` e procure por: `g_save_format`
4. Você vai encontrar uma linha assim:
   ```
   uset g_save_format "0"
   ```
   O número pode ser outro. Troque o número entre aspas por **2**:
   ```
   uset g_save_format "2"
   ```
5. Salve o arquivo (`Ctrl` + `S`) e feche o Bloco de Notas.

> Se a linha `g_save_format` não existir, adicione no final do arquivo: `uset g_save_format "2"`

**Por que com o jogo fechado?** O jogo regrava o `config.cfg` quando fecha, e a sua mudança seria perdida.

---

## Passo 3 – Preparar a situação no jogo

1. Abra o ETS2 e carregue o seu perfil normalmente.
2. Para o teste ser útil, o ideal é ter:
   - pelo menos **1 motorista contratado**;
   - com **um caminhão atribuído** a ele;
   - e, se possível, **fazendo uma entrega** (veja em **Gerenciador de motoristas**: status "em serviço").
3. Se puder, deixe **2 ou mais motoristas**, com caminhões diferentes e personalizados. Assim testamos mais casos de uma vez.

---

## Passo 4 – Salvar o jogo

1. No jogo, aperte **Esc**.
2. Clique em **Salvar e Carregar** → **Salvar jogo**.
3. Digite um nome fácil, por exemplo **`livingfleet`**, e confirme.
4. Pode continuar jogando ou sair do jogo. O programa lê o arquivo salvo, e o jogo pode ficar aberto.

> **Dica:** o save em texto fica maior que o normal. Isso é esperado.

---

## Passo 5 – Baixar o `fleet_reader.exe`

1. Baixe o `fleet_reader.exe` (enviado no chat ou na página do projeto no GitHub).
2. Crie uma pasta, por exemplo **`Área de Trabalho\LivingFleet`**, e coloque o `.exe` dentro dela.

---

## Passo 6 – Executar o programa

**Opção A (mais fácil): dois cliques**

1. Dê **dois cliques** no `fleet_reader.exe`.
2. Ele procura **sozinho o save mais recente** e mostra o resultado numa janela preta.

**Opção B: escolher o save manualmente** (use se a Opção A pegar o save errado)

1. Vá até o save que você criou:
   `Documentos\Euro Truck Simulator 2\profiles\<código do perfil>\save\<nome do save>\`
   - O "código do perfil" é uma pasta com um nome estranho, tipo `4D61726961`. É o seu perfil.
   - Dentro de `save`, procure a pasta com o nome que você deu (ou a mais recente).
2. Dentro dela há o arquivo **`game.sii`**.
3. **Arraste o `game.sii` e solte em cima do `fleet_reader.exe`.**

### Aviso do Windows "O Windows protegeu o computador"

Esse aviso aparece porque o programa é novo e não tem assinatura digital paga. Para continuar:
1. Clique em **Mais informações**.
2. Clique em **Executar assim mesmo**.

Se o antivírus bloquear, me avise. Você também pode compilar o programa você mesmo a partir do código-fonte no GitHub.

---

## Passo 7 – Ler o resultado

A janela preta mostra o que foi encontrado. No final aparece:

```
Log salvo em: C:\Users\...\LivingFleet\living_fleet_log.txt
Pressione ENTER para sair...
```

Aperte **ENTER** para fechar. O arquivo **`living_fleet_log.txt`** fica **na mesma pasta do `fleet_reader.exe`**.

Para conferir rapidamente, abra o arquivo com o Bloco de Notas. Você deve ver algo como:

```
### Motorista 1: driver.carlos
  Garagem: garage.berlin (slot 0)
  [Caminhao via 'assigned_truck'] _nameless.xxx
     Marca/modelo: mercedes.axor
     Pecas/acessorios (35):
       - ...
  [Trabalho via 'driver_job'] ...
     source_company: ...
     target_company: ...
```

---

## Passo 8 – Me enviar os arquivos

Aqui no chat:
1. **Obrigatório:** anexe o **`living_fleet_log.txt`**.
2. **Muito útil:** anexe também o **`game.sii`** do Passo 6, opção B. Se ele for grande demais para enviar, compacte antes: botão direito → **Enviar para** → **Pasta compactada (zip)**.
3. **Opcional:** um print da tela **Meus motoristas** do jogo. Assim eu comparo o que o jogo mostra com o que o programa leu.

> O `game.sii` contém só dados do jogo (perfil, dinheiro, caminhões, motoristas). Não tem senha nem dados da sua conta Steam.

---

## Problemas comuns

| Mensagem ou problema | O que fazer |
|---|---|
| `O save esta em formato binario/criptografado` | O Passo 2 não funcionou. Confira se o jogo estava **fechado** ao editar o `config.cfg` e se a linha ficou `uset g_save_format "2"`. Depois **salve o jogo de novo** (Passo 4). |
| `Nenhum game.sii encontrado` | Os seus documentos podem estar em outro lugar. Use a **Opção B** do Passo 6 (arrastar o arquivo). |
| A janela abre e fecha na hora | Me avise e mande um print. |
| O programa leu um save antigo | Use a **Opção B** e arraste o `game.sii` do save certo. |
| `Nenhuma unidade 'driver_ai' encontrada` | Não é erro seu: o formato pode ter mudado na 1.61. **Me envie o log e o `game.sii`** que eu ajusto o programa. |
| `Erro ao ler o save: SII linha ...` | Me envie o log/print e o `game.sii`. O programa precisa de ajuste. |

---

## Depois do teste (opcional)

Pode deixar o `g_save_format "2"`, que não causa problemas no jogo. Os saves só ficam um pouco maiores. Se quiser voltar ao normal, repita o Passo 2 colocando o número que estava antes.
