# Proveniência — o DSP do Rasgo Modular no piloto VCV

- **Origem:** `RASGO_MODULAR` (repositório `lucioaraujo/rasgo-modular`),
  commit `3dce50c` (23/11/2026), copiado por `tools/gerar_plugin.py`.
- **Copiado (sem alteração), em `src/rasgo/`:** `core/SignalGraph.hpp`,
  `core/Graph.hpp`, `core/Panel.hpp`, `core/Denormals.hpp`, `core/RtFloat.hpp`
  e todos os `src/dsp/*.hpp`. Checksums em `PROVENANCE.sha256`.
- **Gerado do Rasgo Modular** (mesmo commit): `src/modulos.inc`,
  `src/familias.inc`, `src/dossies.inc` (catálogo e índice dos dossiês) e
  `src/learn_en.inc` (o LEARN em inglês, oferecido também sob GPL pelo autor
  em 23/11/2026).
- **Licença:** `src/dsp/` e `src/core/` do Rasgo Modular são oferecidos também
  sob GPL-3.0-or-later (`RASGO_MODULAR/LICENSING.md`, decisão do autor de
  20/11/2026); é a licença deste plugin, compatível com o VCV Rack (GPLv3+) e
  com o Cardinal. Texto em `LICENSE.txt`.
- **Autoria:** Lúcio de Araújo. Créditos de cada algoritmo nos dossiês do
  Rasgo Modular (RUNGLER: Rob Hordijk; GENDYN: Iannis Xenakis).
- **Regra da família:** cópia fixada, sem dependência de build da pasta irmã
  (`RASGO/AGENTS.md` §4). Para atualizar, copiar de um commit novo e
  registrar aqui.
