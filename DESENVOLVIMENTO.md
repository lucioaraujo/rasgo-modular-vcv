# Rasgo Modular para o VCV Rack

Estado (23/11/2026): **local, não publicado.** Começou como piloto de seis
módulos ("pode baixar o kit vcv e começar o piloto") e passou ao catálogo
inteiro ("todos os módulos do rasgo modular para o vcv"): **100 módulos**,
todos menos os cinco que são o encaixe com o aplicativo (SIGNAL-IN, POLY-IN,
NOTE-OUT, MIXER, MASTER — o Rack tem os seus). `tools/gerar_plugin.py` lê o
catálogo, os guias e o LEARN do Rasgo Modular e gera a lista; cada módulo
entra pela casca genérica. Plano e decisões em
`RASGO_MODULAR/dossies/ESTUDO_port_vcv_cardinal.md` (§8, o piloto).

## Como funciona

`src/Casca.hpp` embrulha qualquer módulo do Rasgo (um `Signal`, que processa
em blocos) num módulo do Rack (amostra a amostra): blocos de 16 amostras,
tensões ±5 V para áudio e CV, 0–10 V para gate/trigger, 1 V/oct igual nos
dois. Parâmetros, portas e posições vêm do próprio módulo, então um módulo
novo custa uma linha em `src/plugin.cpp`. O painel ainda é provisório
(fundo liso e rótulos): a aparência passa pelo autor.

## Visual (decidido pelo autor em 23/11/2026)

"Gostei mais do prata": as peças do VCV (knob, jack, slider, botão de trava)
com a identidade RASGO — o fundo e as cores do app, a faixa da família em
degradê (cheia à esquerda, transparente à direita), reflexo prata nos knobs,
arruela e círculo interno do jack no mesmo cinza, traço dos visores na cor da
família, títulos em Nunito Bold, rótulos em DejaVu Sans (as duas do Rack) e
o wordmark RASGO na base. Os botões de trava acendem uma luz laranja no
centro quando ligados (autor: "não sei quando o botão está apertado"). É o padrão de `src/Casca.hpp` (`RASGO_ESTILO 3`,
`RASGO_DEGRADE 1`); as outras variantes ficam como opção de build
(`RASGO_FLAGS=-DRASGO_ESTILO=1` = peças desenhadas como no app; `2` = B com
laranja). Imagens em `docs/`.

**Licença das peças:** os gráficos da Component Library do VCV são CC BY-NC
4.0 (© VCV). O plugin usa as que o próprio Rack instala, sem copiá-las; é o
uso de todo plugin gratuito. Vender o plugin exigiria a licença comercial da
VCV. O app do Rasgo não usa nada disso (desenha as suas peças).

## Informação de cada módulo

- **Dica no mouse** em cada knob e jack: o texto do LEARN em inglês (1400
  textos; o autor os ofereceu também sob GPL em 23/11/2026, `LICENSING.md`).
- **User manual** (botão direito): a página do módulo no guia do site.
- **Créditos** (botão direito): "Rasgo Modular — Lúcio de Araújo", a origem
  da ideia quando há uma pessoa ou um modelo (RUNGLER: Rob Hordijk; GENDYN e o
  crivo do QUANTIZER: Xenakis; Lotka-Volterra, Kuramoto…), e os links do
  site, do dossiê do módulo (fontes e créditos) e da família RASGO.
- **Parafusos** prata: no topo só à direita (o título fica à esquerda); o
  título diminui se não couber (COMPARADOR).
- **Wordmark RASGO** na base, sempre no tamanho cheio e na mesma altura nos
  100 módulos (autor, 24/11). No centro, entre os dois parafusos de baixo;
  quando um controle de baixo esbarraria, os controles sobem um pouco (até
  3 %) e, se ainda não couber, o wordmark vai para a esquerda e fica só o
  parafuso da direita (CHAOS, SUPPLY, TAP, COMPARADOR, RELATE, SUM).
- **Knobs** em três tamanhos: o grande onde o app declara (CUT do FILTER e
  do LADDER), o médio onde há 15 mm livres em volta, o pequeno nos outros.

## Construir

Com o SDK gratuito da VCV (Rack SDK 2.x) em `RACK_DIR` (sem o
`jq` instalado, passe o nome e a versão):

```sh
make -j2 RACK_DIR=/caminho/do/Rack-SDK SLUG=RasgoModular VERSION=2.0.0
```

Teste sem janela (a casca: altura, tensões, atraso, LADDER):

```sh
SDK=$RACK_DIR
g++ -std=gnu++17 -O2 -I$SDK/include -I$SDK/dep/include -Isrc -Isrc/rasgo \
    tests/teste_casca.cpp -L$SDK -lRack -o /tmp/teste_casca
LD_LIBRARY_PATH=$SDK /tmp/teste_casca
```

## Próximo

1. Abrir no VCV Rack Free (gratuito) e ouvir; imagens dos painéis para o
   autor.
2. Aparência dos painéis (decisão do autor), nome do plugin, repositório
   próprio.
3. Os outros módulos em lotes; visores; publicação na VCV Library e
   proposta ao Cardinal.
