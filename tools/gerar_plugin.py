#!/usr/bin/env python3
"""Gera o plugin VCV a partir do Rasgo Modular (cópia fixada, sem dependência
de build da pasta irmã — RASGO/AGENTS.md §4).

- copia src/core (o necessário) e src/dsp inteiros de RASGO_MODULAR (o commit
  atual) para src/rasgo/, com sha256 em PROVENANCE.sha256;
- lê o catálogo (apps/panel/ModuleCatalog.hpp): tipo → classe e família;
- escreve src/modulos.inc (os modelos) e src/familias.inc (tipo → família);
- reescreve a lista "modules" do plugin.json, com a descrição em inglês da
  primeira frase do guia do site (website/guia/en/TIPO.md).
Ficam de fora os módulos que são o encaixe com o aplicativo (o Rack tem os
seus): SIGNAL-IN, POLY-IN, NOTE-OUT, MIXER, MASTER.
Uso: python3 tools/gerar_plugin.py
"""
import hashlib, json, pathlib, re, shutil, subprocess

raiz = pathlib.Path(__file__).resolve().parent.parent
mod = raiz.parent / "RASGO_MODULAR"
FORA = {"SIGNAL-IN", "POLY-IN", "NOTE-OUT", "MIXER", "MASTER"}
TAGS = {"SOURCE": ["Oscillator"], "TRANSFORM": ["Effect"], "MODULATE": ["Envelope generator", "LFO"],
        "TIME": ["Clock generator", "Sequencer"], "DECISION": ["Random", "Logic"], "ROUTE": ["Utility"],
        "SPACE": ["Effect", "Delay"], "OUT": ["Visual"]}

cat = (mod / "apps/panel/ModuleCatalog.hpp").read_text(encoding="utf-8")
classe = dict(re.findall(r'if \(t == "([A-Z0-9-]+)"\)\s+return std::make_unique<(\w+)>', cat))
# o nome com apelido (AUDIO-IN, COMPARADOR…), com ou sem comentário na linha
for a, c in re.findall(r'if \(t == "([A-Z0-9-]+)" \|\| t == "[A-Z0-9-]+"\)[^\n]*\n?\s*return std::make_unique<(\w+)>', cat):
    classe.setdefault(a, c)
bloco = cat[cat.index("seedModuleCatalog()"):]
bloco = bloco[:bloco.index("return c;")]
familia = {}
ordem = []
for fam, lista in re.findall(r'\{"([A-Z]+)",\s*\{([^}]*)\}\}', bloco):
    for t in re.findall(r'"([A-Z0-9-]+)"', lista):
        familia[t] = fam
        ordem.append(t)
tipos = [t for t in ordem if t not in FORA]
falta = [t for t in tipos if t not in classe]
assert not falta, f"sem classe no catálogo: {falta}"

# cópia fixada do DSP
dst = raiz / "src" / "rasgo"
shutil.rmtree(dst, ignore_errors=True)
(dst / "core").mkdir(parents=True)
(dst / "dsp").mkdir()
for f in ["SignalGraph.hpp", "Graph.hpp", "Panel.hpp", "Denormals.hpp", "RtFloat.hpp"]:
    shutil.copy2(mod / "src/core" / f, dst / "core" / f)
for f in sorted((mod / "src/dsp").glob("*.hpp")):
    shutil.copy2(f, dst / "dsp" / f.name)
linhas = []
for f in sorted(dst.rglob("*.hpp")):
    linhas.append(f"{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.relative_to(dst)}")
(raiz / "PROVENANCE.sha256").write_text("\n".join(linhas) + "\n")
commit = subprocess.run(["git", "-C", str(mod), "rev-parse", "--short", "HEAD"],
                        capture_output=True, text=True).stdout.strip()

# o cabeçalho de cada classe: o include do catálogo que a declara
incs = re.findall(r'#include "(dsp/\w+\.hpp)"', cat)
decl = {}
for inc in incs:
    txt = (mod / "src" / inc).read_text(encoding="utf-8")
    for c in re.findall(r'class (\w+) final\b', txt):
        decl[c] = inc
with (raiz / "src" / "modulos.inc").open("w") as f:
    f.write(f"// GERADO por tools/gerar_plugin.py (Rasgo Modular {commit}). Não editar.\n")
    for inc in sorted({decl[classe[t]] for t in tipos}):
        f.write(f'#include "{inc}"\n')
    f.write("#define RASGO_MODELOS(X) \\\n")
    for t in tipos:
        f.write(f'    X({classe[t]}, "{t}") \\\n')
    f.write("\n")
with (raiz / "src" / "familias.inc").open("w") as f:
    f.write(f"// GERADO por tools/gerar_plugin.py (Rasgo Modular {commit}). Não editar.\n")
    for t in tipos:
        f.write(f'if (tipo == "{t}") return "{familia[t]}";\n')

def descricao(t):
    g = mod / "website/guia/en" / f"{t}.md"
    if not g.exists():
        return t
    txt = g.read_text(encoding="utf-8").split("\n")
    for l in txt[1:]:
        if l.strip():
            frase = re.split(r"(?<=[.])\s", l.strip())[0]
            return frase.rstrip(".")[:200]
    return t

# os textos do LEARN em inglês (autorizados sob GPL pelo autor, 23/11/2026):
# t["TIPO"]["bind"] = LearnEntry{"texto", ...} nos blocos learnTableEnMutable()
lw = (mod / "apps/panel/LearnWidgetI18n.hpp").read_text(encoding="utf-8")
learn = {}
for bloco_t in re.split(r"LearnTable& t = ", lw)[1:]:
    if not bloco_t.startswith("learnTableEnMutable()"):
        continue
    for tp, bind, txt in re.findall(r't\["([A-Z0-9-]+)"\]\["([^"]*)"\] = LearnEntry\{"((?:[^"\\]|\\.)*)"', bloco_t):
        learn[(tp, bind)] = txt.replace('\\"', '"')
with (raiz / "src" / "learn_en.inc").open("w", encoding="utf-8") as f:
    f.write(f"// GERADO por tools/gerar_plugin.py: o LEARN em inglês do Rasgo Modular {commit}\n")
    f.write("// (textos do autor, oferecidos também sob GPL-3.0-or-later em 23/11/2026).\n")
    n = 0
    for (tp, bind), txt in sorted(learn.items()):
        if tp in tipos:
            f.write(f'{{"{tp}", "{bind}", R"RASGO({txt})RASGO"}},\n'); n += 1
print(f"LEARN em inglês: {n} textos")

# o dossiê de cada módulo (créditos e fontes), pelo índice dos dossiês
idx = (mod / "dossies/00_indice.md").read_text(encoding="utf-8")
dossie = {}
for arq, tp in re.findall(r"\| *\d+ *\| *\[`([^`]+\.md)`\]\([^)]*\) *\| *`([A-Z0-9-]+)`", idx):
    dossie.setdefault(tp, arq)
with (raiz / "src" / "dossies.inc").open("w", encoding="utf-8") as f:
    f.write("// GERADO por tools/gerar_plugin.py: o dossiê (fontes e créditos) de cada módulo\n")
    for tp in tipos:
        if tp in dossie:
            f.write(f'if (tipo == "{tp}") return "{dossie[tp]}";\n')
print(f"dossiês: {sum(1 for t in tipos if t in dossie)} de {len(tipos)}")

pj = json.loads((raiz / "plugin.json").read_text(encoding="utf-8"))
SITE = "https://lucioaraujo.github.io/rasgo-modular/modulos-en.html#mod-"
pj["modules"] = [{"slug": t, "name": t, "description": descricao(t), "tags": TAGS[familia[t]],
                  "manualUrl": SITE + t} for t in tipos]
(raiz / "plugin.json").write_text(json.dumps(pj, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"{len(tipos)} módulos (de {len(ordem)}; fora: {sorted(FORA)}), DSP de {commit}")
