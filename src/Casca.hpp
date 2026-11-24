#pragma once

// A casca: um módulo do Rasgo Modular (um `Signal`, em blocos) dentro do
// VCV Rack 2 (amostra a amostra). Piloto de 23/11/2026; o plano está em
// `RASGO_MODULAR/dossies/ESTUDO_port_vcv_cardinal.md`.
//
// - Blocos de 16 amostras: a casca junta as entradas, roda o DSP e entrega
//   as saídas do bloco anterior (16 amostras de atraso, 0,33 ms a 48 kHz).
// - Tensões (estudo §2): áudio e CV ±1 ↔ ±5 V; gate/trigger 0–1 ↔ 0–10 V;
//   1 V/oct igual nos dois (0 V = dó central).
// - Parâmetros, portas e posições vêm do próprio módulo (`parameters()`,
//   descritores, `panel()`): nada copiado à mão.
// - O painel é provisório (fundo liso e rótulos): a aparência passa pelo
//   autor antes de qualquer publicação.

#include "plugin.hpp"

#include "core/SignalGraph.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace rasgo_vcv {

using rasgo::modular::AudioBlock;
using rasgo::modular::PortDescriptor;
using rasgo::modular::PortKind;
using rasgo::modular::Signal;
using WK = rasgo::modular::Widget::Kind;   // o `Widget` do Rack tem o mesmo nome

constexpr int kBloco = 16;

// quantos volts valem 1,0 do Rasgo nesta porta
inline float voltsPorUnidade(const PortDescriptor& d) {
    if (d.unit == "v/oct") return 1.0f;
    if (d.unit == "gate" || d.unit == "trig") return 10.0f;
    return 5.0f;
}

// o LEARN em inglês (dica de cada knob e jack) e o dossiê de cada módulo
struct TextoLearn { const char* tipo; const char* bind; const char* texto; };
inline const char* learnEn(const std::string& tipo, const std::string& bind) {
    static const TextoLearn tabela[] = {
#include "learn_en.inc"
    };
    for (const auto& e : tabela)
        if (tipo == e.tipo && bind == e.bind) return e.texto;
    return nullptr;
}
inline std::string dossieDe(const std::string& tipo) {
#include "dossies.inc"
    return {};
}
// a origem de cada ideia, quando o módulo vem de uma pessoa ou de um modelo
// conhecido (linhas escritas a partir dos dossiês; o detalhe está neles)
inline const char* origemDe(const std::string& tipo) {
    if (tipo == "RUNGLER") return "Based on the Rungler of Rob Hordijk's Benjolin";
    if (tipo == "GENDYN") return "Dynamic stochastic synthesis (GENDYN), after Iannis Xenakis";
    if (tipo == "PREDATOR") return "Lotka-Volterra predator-prey model (after the RASGO Aquorbium)";
    if (tipo == "SWARM") return "Kuramoto model of coupled oscillators";
    if (tipo == "INTERFERENCE") return "Young's interference fringes and moire";
    if (tipo == "REFRACT") return "Snell's law of refraction";
    if (tipo == "ECLIPSE") return "Synodic and draconic months (after the RASGO Asterismus)";
    if (tipo == "QUANTIZER") return "Sieve after Iannis Xenakis (Formalized Music)";
    return nullptr;
}

// a família de cada tipo (gerada do catálogo do Rasgo Modular)
inline std::string familiaDe(const std::string& tipo) {
#include "familias.inc"
    return {};
}

template <class D>
struct Casca : rack::engine::Module {
    // Polifonia (autor, 24/11): com um cabo de várias vozes na entrada, uma
    // cópia do DSP por voz, até 16. A família SPACE (memória longa: SPACE,
    // HALL, LOOPER, MEMORY, SAMPLER…) fica mono. Todas as cópias nascem no
    // construtor: nada é alocado no áudio.
    static constexpr int kMaxVozes = 16;
    D dsp;                                     // a voz 0 (e o modelo do painel)
    std::vector<std::unique_ptr<D>> extras;    // as vozes 1…
    int maxVozes = 1, nVozes = 1;
    float taxa = 0.0f;
    int pos = 0;
    struct Buf { std::vector<AudioBlock> entra, sai, pronto; std::vector<const AudioBlock*> ptrs; };
    std::vector<Buf> buf;
    std::vector<float> escalaIn, escalaOut;
    std::vector<std::string> ids;
    // escala logarítmica (como o app: os tempos do ENVELOPE): o knob guarda
    // 0–1 e o DSP recebe mín·(máx/mín)^v; o Rack mostra o valor em segundos
    std::vector<char> logp;
    std::vector<float> pmin, pratio;

    // o visor (voz 0), em anéis lidos pela interface. Modos (autor, 24/11:
    // "visores próprios"): 0 traço da saída · 1 XY (saídas x/y: a trajetória)
    // · 2 pistas (até 4 saídas de gatilho/gate)
    static constexpr int kVisor = 512;
    int visorModo = 0;
    float visor[kVisor] = {};
    float visorY[kVisor] = {};
    float pistas[4][kVisor] = {};
    float pistaMax[4] = {0, 0, 0, 0};   // o máximo entre duas leituras (gatilho curto não some)
    int nPistas = 0, saidaPista[4] = {0, 0, 0, 0};
    int visorW = 0, visorPasso = 1, visorConta = 0, visorSaida = 0, saidaY = 0;
    bool visorLento = false;

    // uma luz por slider (a luz laranja do slider na variante B)
    std::vector<int> luzParam;

    D& voz(int v) { return v == 0 ? dsp : *extras[static_cast<std::size_t>(v - 1)]; }

    Casca() {
        const auto& ps = dsp.parameters();
        for (const auto& w : dsp.panel().widgets)
            if (w.kind == WK::Slider)
                for (std::size_t i = 0; i < ps.size(); ++i)
                    if (ps[i].descriptor.id == w.bind) luzParam.push_back(static_cast<int>(i));
        config(static_cast<int>(ps.size()), static_cast<int>(dsp.inputCount()),
               static_cast<int>(dsp.outputCount()), static_cast<int>(luzParam.size()));
        for (std::size_t i = 0; i < ps.size(); ++i) {
            const auto& pd = ps[i].descriptor;
            const bool lg = dsp.type() == "ENVELOPE" && pd.minimum > 0.0f
                && (pd.id == "attack" || pd.id == "decay" || pd.id == "release");
            if (lg) {
                const float r = pd.maximum / pd.minimum;
                configParam(static_cast<int>(i), 0.0f, 1.0f,
                            std::log(pd.defaultValue / pd.minimum) / std::log(r),
                            pd.id, pd.unit.empty() ? "" : " " + pd.unit, r, pd.minimum);
            } else {
                configParam(static_cast<int>(i), pd.minimum, pd.maximum, pd.defaultValue,
                            pd.id, pd.unit.empty() ? "" : " " + pd.unit);
            }
            ids.push_back(pd.id);
            if (const char* tx = learnEn(dsp.type(), pd.id)) paramQuantities[i]->description = tx;
            logp.push_back(lg ? 1 : 0);
            pmin.push_back(pd.minimum);
            pratio.push_back(lg ? pd.maximum / pd.minimum : 1.0f);
        }
        for (std::size_t i = 0; i < dsp.inputCount(); ++i) {
            configInput(static_cast<int>(i), dsp.inputDescriptor(i).name);
            if (const char* tx = learnEn(dsp.type(), "in:" + dsp.inputDescriptor(i).name)) inputInfos[i]->description = tx;
            escalaIn.push_back(voltsPorUnidade(dsp.inputDescriptor(i)));
        }
        int sx = -1, sy = -1;
        for (std::size_t i = 0; i < dsp.outputCount(); ++i) {
            const auto& od = dsp.outputDescriptor(i);
            configOutput(static_cast<int>(i), od.name);
            if (const char* tx = learnEn(dsp.type(), "out:" + od.name)) outputInfos[i]->description = tx;
            escalaOut.push_back(voltsPorUnidade(od));
            const std::string& nm = od.name;
            if ((dsp.type() == "ENVELOPE" && nm == "env") || (dsp.type() == "RUNGLER" && nm == "rung"))
                visorSaida = static_cast<int>(i);
            if (sx < 0 && (nm == "x" || nm == "x1" || nm == "X")) sx = static_cast<int>(i);
            if (sy < 0 && (nm == "y" || nm == "y1" || nm == "Y")) sy = static_cast<int>(i);
            if ((od.unit == "trig" || od.unit == "gate") && nPistas < 4) saidaPista[nPistas++] = static_cast<int>(i);
        }
        const std::string f = familiaDe(dsp.type());
        if (sx >= 0 && sy >= 0) { visorModo = 1; visorSaida = sx; saidaY = sy; }
        else if (nPistas >= 3) visorModo = 2;
        // os módulos de controle (e o RUNGLER) mostram o contorno no tempo
        visorLento = visorModo == 2 || dsp.type() == "RUNGLER" || f == "MODULATE" || f == "TIME"
                  || f == "DECISION" || f == "ROUTE";
        if (visorModo == 1) visorLento = false;   // a trajetória: amostra a amostra (passo abaixo)
        maxVozes = f == "SPACE" ? 1 : kMaxVozes;
        for (int v = 1; v < maxVozes; ++v) extras.push_back(std::make_unique<D>());
        buf.resize(static_cast<std::size_t>(maxVozes));
    }

    void preparar(const float sr) {
        taxa = sr;
        // o visor lento cobre ~1,4 s; o de áudio, ~10 ms; a trajetória, ~0,5 s
        visorPasso = visorLento ? std::max(1, static_cast<int>(sr * 1.4f / kVisor))
                   : (visorModo == 1 ? std::max(1, static_cast<int>(sr * 0.5f / kVisor)) : 1);
        for (int v = 0; v < maxVozes; ++v) {
            voz(v).prepare(sr, kBloco);
            auto& b = buf[static_cast<std::size_t>(v)];
            b.entra.assign(dsp.inputCount(), AudioBlock(sr, 2, kBloco));
            b.sai.assign(dsp.outputCount(), AudioBlock(sr, 2, kBloco));
            b.pronto.assign(dsp.outputCount(), AudioBlock(sr, 2, kBloco));
            b.ptrs.assign(dsp.inputCount(), nullptr);
        }
        pos = 0;
    }

    void process(const ProcessArgs& args) override {
        if (args.sampleRate != taxa) preparar(args.sampleRate);
        // quantas vozes: o maior número de canais nas entradas (mono = 1)
        if (pos == 0) {
            int n = 1;
            for (std::size_t i = 0; i < escalaIn.size(); ++i) n = std::max(n, inputs[i].getChannels());
            nVozes = std::min(n, maxVozes);
            for (std::size_t o = 0; o < escalaOut.size(); ++o) outputs[o].setChannels(nVozes);
        }
        const std::size_t ps = static_cast<std::size_t>(pos);
        for (int v = 0; v < nVozes; ++v) {
            auto& b = buf[static_cast<std::size_t>(v)];
            for (std::size_t i = 0; i < b.entra.size(); ++i) {
                const float x = inputs[i].getPolyVoltage(v) / escalaIn[i];
                b.entra[i].at(0, ps) = x;
                b.entra[i].at(1, ps) = x;
            }
            for (std::size_t o = 0; o < b.pronto.size(); ++o)
                outputs[o].setVoltage(b.pronto[o].at(0, ps) * escalaOut[o], v);
        }
        if (visorModo == 2)
            for (int k = 0; k < nPistas; ++k)
                pistaMax[k] = std::max(pistaMax[k], buf[0].pronto[static_cast<std::size_t>(saidaPista[k])].at(0, ps));
        if (++visorConta >= visorPasso) {   // o visor mostra a voz 0
            visorConta = 0;
            const auto& pr = buf[0].pronto;
            if (visorSaida < static_cast<int>(pr.size())) visor[visorW] = pr[static_cast<std::size_t>(visorSaida)].at(0, ps);
            if (visorModo == 1 && saidaY < static_cast<int>(pr.size())) visorY[visorW] = pr[static_cast<std::size_t>(saidaY)].at(0, ps);
            if (visorModo == 2)
                for (int k = 0; k < nPistas; ++k) { pistas[k][visorW] = pistaMax[k]; pistaMax[k] = 0.0f; }
            visorW = (visorW + 1) % kVisor;
        }
        if (++pos < kBloco) return;
        pos = 0;
        for (std::size_t k = 0; k < luzParam.size(); ++k)
            if (auto* q = paramQuantities[static_cast<std::size_t>(luzParam[k])])
                lights[k].setBrightness(0.25f + 0.75f * q->getScaledValue());
        for (int v = 0; v < nVozes; ++v) {
            D& d = voz(v);
            for (std::size_t p = 0; p < ids.size(); ++p)
                d.setParameter(ids[p], logp[p] ? pmin[p] * std::pow(pratio[p], params[p].getValue())
                                               : params[p].getValue());
            auto& b = buf[static_cast<std::size_t>(v)];
            for (std::size_t i = 0; i < b.ptrs.size(); ++i)
                b.ptrs[i] = inputs[i].isConnected() ? &b.entra[i] : nullptr;
            d.process(b.ptrs, b.sai);
            b.pronto.swap(b.sai);
        }
    }
};

// ---- o painel ----------------------------------------------------------
// RASGO_ESTILO (proposta ao autor, 23/11/2026):
//   1 = A, fiel ao app: knobs, jacks, sliders e interruptores desenhados como
//       no Rasgo Modular;
//   2 = B, identidade RASGO com as peças padrão do VCV.
// Nos dois: o fundo e as cores do app, a faixa da família no topo, os
// trilhos, título, rótulos no cinza do app afastados dos jacks, o visor com
// moldura e o nome RASGO no rodapé.
// Escolha do autor (23/11/2026): "gostei mais do prata" — B prata (3), com a
// faixa em degradê e o reflexo nos knobs. As outras ficam como opção de build.
#ifndef RASGO_ESTILO
#define RASGO_ESTILO 3
#endif
#ifndef RASGO_DEGRADE
#define RASGO_DEGRADE 1
#endif

namespace cor {
inline NVGcolor superficie() { return nvgRGB(0x26, 0x2b, 0x36); }
inline NVGcolor recesso() { return nvgRGB(0x0e, 0x10, 0x15); }
inline NVGcolor linha() { return nvgRGB(0x48, 0x50, 0x60); }
inline NVGcolor textoPrim() { return nvgRGB(0xf0, 0xf3, 0xf8); }
inline NVGcolor textoSec() { return nvgRGB(0xbe, 0xc8, 0xd8); }
inline NVGcolor acento() { return nvgRGB(0xff, 0x9d, 0x4c); }
// a cor da família (a mesma do app: tons apagados, `familyColour`)
inline NVGcolor familia(const std::string& f) {
    float h = -1.0f;
    if (f == "SOURCE") h = 90.0f; else if (f == "TRANSFORM") h = 285.0f;
    else if (f == "MODULATE") h = 325.0f; else if (f == "TIME") h = 150.0f;
    else if (f == "DECISION") h = 52.0f; else if (f == "ROUTE") h = 195.0f;
    else if (f == "SPACE") h = 245.0f; else if (f == "OUT") h = 0.0f;
    if (h < 0.0f) return linha();
    return nvgHSL(h / 360.0f, f == "OUT" ? 0.12f : 0.34f, 0.56f);
}
}  // namespace cor


inline std::shared_ptr<rack::window::Font> fonte(bool negrito) {
    return APP->window->loadFont(rack::asset::system(
        negrito ? "res/fonts/Nunito-Bold.ttf" : "res/fonts/DejaVuSans.ttf"));
}

struct PainelRasgo : rack::widget::Widget {
    rasgo::modular::Panel painel;
    std::string titulo, familia;
    std::vector<float> diam;   // diâmetro real (px) de cada knob criado; 0 = o do app
    float logoX = -1.0f, logoTopo = 0.0f, logoW = 0.0f;   // o wordmark (tamanho cheio)
    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        const float W = box.size.x, H = box.size.y;
        nvgBeginPath(vg); nvgRect(vg, 0, 0, W, H); nvgFillColor(vg, cor::superficie()); nvgFill(vg);
        nvgBeginPath(vg); nvgRect(vg, 0.5f, 0.5f, W - 1, H - 1);
        nvgStrokeColor(vg, cor::linha()); nvgStrokeWidth(vg, 1.0f); nvgStroke(vg);
        // a faixa da família e o trilho de baixo
        nvgBeginPath(vg); nvgRect(vg, 1, 1, W - 2, 4);
#if RASGO_DEGRADE
        // a faixa em degradê: a cor cheia à esquerda, transparente à direita
        {
            NVGcolor c0 = cor::familia(familia), c1 = c0;
            c1.a = 0.0f;
            nvgFillPaint(vg, nvgLinearGradient(vg, 1, 0, W - 1, 0, c0, c1));
        }
#else
        nvgFillColor(vg, cor::familia(familia));
#endif
        nvgFill(vg);
        nvgBeginPath(vg); nvgMoveTo(vg, 3, H - 4.0f); nvgLineTo(vg, W - 3, H - 4.0f);
        nvgStrokeColor(vg, cor::linha()); nvgStroke(vg);
        // o wordmark RASGO na base (o SVG da família, `res/rasgo_wordmark.svg`)
        if (auto svg = APP->window->loadSvg(rack::asset::plugin(pluginInstance, "res/rasgo_wordmark.svg"))) {
            if (svg->handle) {
                if (logoX >= 0.0f) {
                    const float s = logoW / svg->handle->width;
                    nvgSave(vg);
                    nvgTranslate(vg, logoX, logoTopo);
                    nvgScale(vg, s, s);
                    rack::window::svgDraw(vg, svg->handle);
                    nvgRestore(vg);
                }
            }
        }
        auto fb = fonte(true), fr = fonte(false);
        if (!fb || !fr) return;
        nvgFontFaceId(vg, fb->handle);
        nvgFontSize(vg, 12.0f);
        nvgFillColor(vg, cor::textoPrim());
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        {
            // o título não passa do parafuso da direita: se não couber
            // (COMPARADOR em 8 HP), a fonte diminui só o necessário
            const float x0 = rack::mm2px(2.2f);
            const float livre = W - 2.0f * rack::app::RACK_GRID_WIDTH - x0 - 2.0f;
            const float largura = nvgTextBounds(vg, 0, 0, titulo.c_str(), nullptr, nullptr);
            if (largura > livre) nvgFontSize(vg, std::max(8.0f, 12.0f * livre / largura));
            nvgText(vg, x0, 6.0f, titulo.c_str(), nullptr);
        }
        nvgFontFaceId(vg, fr->handle);
        nvgFontSize(vg, 8.5f);
        nvgFillColor(vg, cor::textoSec());
        for (const auto& w : painel.widgets) {
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
            switch (w.kind) {
            case WK::Knob: {
                const float d = w.span > 1.0f ? w.span : 8.0f;
                const std::size_t k = static_cast<std::size_t>(&w - painel.widgets.data());
                const float cy = rack::mm2px(w.y + d * 0.5f);
                const float meio = k < diam.size() && diam[k] > 0.0f ? diam[k] * 0.5f : rack::mm2px(d * 0.5f);
                nvgText(vg, rack::mm2px(w.x + d * 0.5f), cy - meio - rack::mm2px(0.8f), w.label.c_str(), nullptr);
                break; }
            case WK::Jack:
                nvgText(vg, rack::mm2px(w.x), rack::mm2px(w.y - 4.6f), w.label.c_str(), nullptr);
                break;
            case WK::Slider:
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                nvgText(vg, rack::mm2px(w.x + 3.0f), rack::mm2px(w.y + 31.0f), w.label.c_str(), nullptr);
                break;
            case WK::Toggle:
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                // o interruptor do VCV (CKSS) é mais alto que o quadrado do Rasgo
                nvgText(vg, rack::mm2px(w.x + (RASGO_ESTILO == 1 ? 2.0f : 2.2f)),
                        rack::mm2px(w.y + (RASGO_ESTILO == 1 ? 5.2f : 6.4f)), w.label.c_str(), nullptr);
                break;
            case WK::Display: {
                const float x = rack::mm2px(w.x), y = rack::mm2px(w.y);
                const float dw = rack::mm2px(w.span > 1.0f ? w.span : 16.0f), dh = rack::mm2px(w.height);
                nvgBeginPath(vg); nvgRect(vg, x, y, dw, dh); nvgFillColor(vg, cor::recesso()); nvgFill(vg);
                nvgStrokeColor(vg, cor::linha()); nvgStroke(vg);
                nvgFillColor(vg, cor::textoSec());
                break; }
            case WK::Label:
                break;
            }
        }
    }
};

// o reflexo: um contorno prata, claro no alto à esquerda e sumindo embaixo
// à direita (como luz batendo de cima)
inline void reflexo(NVGcontext* vg, float r, float raio, float largura) {
    NVGcolor claro = nvgRGBA(0xd8, 0xdd, 0xe6, 0xd0), some = nvgRGBA(0xd8, 0xdd, 0xe6, 0x00);
    nvgBeginPath(vg);
    nvgCircle(vg, r, r, raio);
    nvgStrokePaint(vg, nvgLinearGradient(vg, r - raio, r - raio, r + raio * 0.6f, r + raio * 0.6f, claro, some));
    nvgStrokeWidth(vg, largura);
    nvgStroke(vg);
}

// ---- as peças do estilo A (desenhadas como no app) ---------------------
struct KnobRasgo : rack::app::Knob {
    KnobRasgo() {
        box.size = rack::mm2px(rack::math::Vec(8.0f, 8.0f));
        minAngle = -0.75f * static_cast<float>(M_PI);
        maxAngle = 0.75f * static_cast<float>(M_PI);
    }
    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        const float r = box.size.x * 0.5f;
        nvgBeginPath(vg); nvgCircle(vg, r, r, r - 0.5f);
        nvgFillColor(vg, cor::recesso()); nvgFill(vg);
        nvgStrokeColor(vg, cor::linha()); nvgStrokeWidth(vg, 1.0f); nvgStroke(vg);
        reflexo(vg, r, r - 1.0f, 1.4f);
        float v = 0.5f;
        if (auto* q = getParamQuantity()) v = q->getScaledValue();
        const float a = minAngle + (maxAngle - minAngle) * v;
        nvgBeginPath(vg); nvgMoveTo(vg, r, r);
        nvgLineTo(vg, r + std::sin(a) * r * 0.8f, r - std::cos(a) * r * 0.8f);
        nvgStrokeColor(vg, cor::acento()); nvgStrokeWidth(vg, 2.2f); nvgLineCap(vg, NVG_ROUND); nvgStroke(vg);
    }
};
struct SliderRasgo : rack::app::SliderKnob {
    SliderRasgo() { box.size = rack::mm2px(rack::math::Vec(6.0f, 30.0f)); speed = 2.0f; }
    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        nvgBeginPath(vg); nvgRect(vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(vg, cor::recesso()); nvgFill(vg);
        nvgStrokeColor(vg, cor::linha()); nvgStroke(vg);
        float v = 0.0f;
        if (auto* q = getParamQuantity()) v = q->getScaledValue();
        const float h = (box.size.y - 2.0f) * v;
        nvgBeginPath(vg); nvgRect(vg, 1, box.size.y - 1 - h, box.size.x - 2, h);
        nvgFillColor(vg, cor::acento()); nvgFill(vg);
        // a pega: um traço laranja no nível, visível mesmo com o slider no fundo
        const float y = std::min(box.size.y - 3.0f, box.size.y - 1.0f - h);
        nvgBeginPath(vg); nvgRect(vg, -1.0f, y - 1.0f, box.size.x + 2.0f, 3.0f);
        nvgFillColor(vg, cor::acento()); nvgFill(vg);
    }
};
struct ChaveRasgo : rack::app::Switch {
    ChaveRasgo() { box.size = rack::mm2px(rack::math::Vec(4.0f, 4.0f)); }
    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        float v = 0.0f;
        if (auto* q = getParamQuantity()) v = q->getValue();
        nvgBeginPath(vg); nvgRect(vg, 0.5f, 0.5f, box.size.x - 1, box.size.y - 1);
        nvgFillColor(vg, v > 0.5f ? cor::acento() : cor::recesso()); nvgFill(vg);
        nvgStrokeColor(vg, cor::linha()); nvgStroke(vg);
    }
};
struct JackRasgo : rack::app::PortWidget {
    JackRasgo() { box.size = rack::mm2px(rack::math::Vec(8.4f, 8.4f)); }
    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        const float r = box.size.x * 0.5f;
        nvgBeginPath(vg); nvgCircle(vg, r, r, r * 0.72f);
        nvgFillColor(vg, cor::recesso()); nvgFill(vg);
        nvgStrokeColor(vg, cor::linha()); nvgStrokeWidth(vg, 1.6f); nvgStroke(vg);
        nvgBeginPath(vg); nvgCircle(vg, r, r, r * 0.28f);
        nvgFillColor(vg, nvgRGB(0, 0, 0)); nvgFill(vg);
    }
};

// variante B: as peças do VCV, com o laranja do Rasgo
struct KnobVcvLaranja : rack::componentlibrary::RoundSmallBlackKnob {
    void draw(const DrawArgs& args) override {
        rack::componentlibrary::RoundSmallBlackKnob::draw(args);
        reflexo(args.vg, box.size.x * 0.5f, box.size.x * 0.5f - 1.0f, 1.4f);
        float v = 0.5f;
        if (auto* q = getParamQuantity()) v = q->getScaledValue();
        const float a = minAngle + (maxAngle - minAngle) * v;
        const float r = box.size.x * 0.5f;
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, r + std::sin(a) * r * 0.25f, r - std::cos(a) * r * 0.25f);
        nvgLineTo(args.vg, r + std::sin(a) * r * 0.78f, r - std::cos(a) * r * 0.78f);
        nvgStrokeColor(args.vg, cor::acento()); nvgStrokeWidth(args.vg, 2.2f);
        nvgLineCap(args.vg, NVG_ROUND); nvgStroke(args.vg);
    }
};
// o botão de trava do VCV com uma luz no centro quando ligado (autor, 23/11:
// "não sei quando o botão está apertado ou não"): o laranja de "ligado" do app
struct LatchLuz : rack::componentlibrary::VCVLatch {
    void draw(const DrawArgs& args) override {
        rack::componentlibrary::VCVLatch::draw(args);
        auto* q = getParamQuantity();
        if (!q || q->getValue() < 0.5f) return;
        const float r = box.size.x * 0.5f;
        NVGcontext* vg = args.vg;
        NVGcolor c0 = cor::acento(), c1 = cor::acento();
        c1.a = 0.0f;
        nvgBeginPath(vg); nvgCircle(vg, r, r, r * 0.85f);
        nvgFillPaint(vg, nvgRadialGradient(vg, r, r, r * 0.25f, r * 0.85f, c0, c1));
        nvgFill(vg);
        nvgBeginPath(vg); nvgCircle(vg, r, r, r * 0.32f);
        nvgFillColor(vg, cor::acento()); nvgFill(vg);
    }
};

// o knob do VCV só com o reflexo (B prata), em qualquer tamanho
template <class K>
struct KnobReflexo : K {
    void draw(const typename K::DrawArgs& args) override {
        K::draw(args);
        reflexo(args.vg, this->box.size.x * 0.5f, this->box.size.x * 0.5f - 1.0f, 1.4f);
    }
};
using KnobVcvReflexo = KnobReflexo<rack::componentlibrary::RoundSmallBlackKnob>;
using KnobVcvMedio = KnobReflexo<rack::componentlibrary::RoundBlackKnob>;
using KnobVcvGrande = KnobReflexo<rack::componentlibrary::RoundLargeBlackKnob>;

// O tamanho de cada knob (23/11/2026, autor: "adaptar tamanhos diferentes de
// knobs… ver se há espaço"): o knob que o app já declara maior (CUT do
// FILTER e do LADDER) vai no grande; um knob sem nenhum controle a menos de
// 15 mm (e longe da borda) vai no médio; os outros, no pequeno.
inline int tamanhoKnob(const rasgo::modular::Panel& p, const rasgo::modular::Widget& w) {
    if (w.span > 11.0f) return 2;
    const float d = w.span > 1.0f ? w.span : 8.0f;
    const float cx = w.x + d * 0.5f, cy = w.y + d * 0.5f;
    const float W = static_cast<float>(p.hp) * 5.08f;
    if (cx < 7.0f || cx > W - 7.0f) return 0;
    for (const auto& o : p.widgets) {
        if (&o == &w || o.kind == WK::Label) continue;
        float ox = o.x, oy = o.y;
        if (o.kind == WK::Knob) { const float od = o.span > 1.0f ? o.span : 8.0f; ox += od * 0.5f; oy += od * 0.5f; }
        else if (o.kind == WK::Slider) { ox += 3.0f; oy += 15.0f; }
        else if (o.kind == WK::Toggle) { ox += 2.0f; oy += 2.0f; }
        else if (o.kind == WK::Display) {   // a distância ao retângulo do visor
            const float dw = o.span > 1.0f ? o.span : 16.0f;
            ox = std::clamp(cx, o.x, o.x + dw); oy = std::clamp(cy, o.y, o.y + o.height);
        }
        if (std::hypot(ox - cx, oy - cy) < 15.0f) return 0;
    }
    return 1;
}
// o slider do VCV com a pega laranja (sempre, não só com a luz)
struct SliderVcvLaranja : rack::componentlibrary::VCVSlider {
    void draw(const DrawArgs& args) override {
        rack::componentlibrary::VCVSlider::draw(args);
        if (!handle) return;
        const rack::math::Rect h = handle->box;
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, h.pos.x + 1.2f, h.pos.y + 1.2f, h.size.x - 2.4f, h.size.y - 2.4f, 1.0f);
        nvgFillColor(args.vg, cor::acento()); nvgFill(args.vg);
    }
};
// o jack do VCV com a arruela um pouco mais cinza (um véu cinza sobre o prata)
struct JackVcvCinza : rack::componentlibrary::PJ301MPort {
    void draw(const DrawArgs& args) override {
        rack::componentlibrary::PJ301MPort::draw(args);
        const float r = box.size.x * 0.5f;
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, r, r, r * 0.98f);
        nvgPathWinding(args.vg, NVG_SOLID);
        // até a borda do furo: a arruela e o círculo interno no mesmo cinza
        nvgCircle(args.vg, r, r, r * 0.30f);
        nvgPathWinding(args.vg, NVG_HOLE);
        nvgFillColor(args.vg, nvgRGBA(0x48, 0x50, 0x60, 0x50));
        nvgFill(args.vg);
    }
};

#if RASGO_ESTILO == 1
using PecaKnob = KnobRasgo;
using PecaSlider = SliderRasgo;
using PecaChave = ChaveRasgo;
using PecaJack = JackRasgo;
#elif RASGO_ESTILO == 2   // B laranja
using PecaKnob = KnobVcvLaranja;
using PecaSlider = SliderVcvLaranja;
using PecaChave = LatchLuz;
using PecaJack = JackVcvCinza;
#else                     // B prata (3): as peças do VCV, reflexo no knob, arruela mais cinza
using PecaKnob = KnobVcvReflexo;
using PecaSlider = rack::componentlibrary::VCVSlider;
using PecaChave = LatchLuz;
using PecaJack = JackVcvCinza;
#endif

// o visor: o traço da saída do módulo (o visor padrão do app). Áudio: a
// onda presa no cruzamento por zero, normalizada pelo pico; contorno
// (ENVELOPE, RUNGLER): ~1,4 s, de baixo para cima.
template <class D>
struct VisorRasgo : rack::widget::Widget {
    Casca<D>* m = nullptr;
    std::string rotulo;
    NVGcolor traco = cor::acento();   // a cor da família (a da faixa)
    void draw(const DrawArgs& args) override {
        NVGcontext* vg = args.vg;
        const float w = box.size.x, h = box.size.y;
        nvgBeginPath(vg); nvgMoveTo(vg, 2, h * 0.5f); nvgLineTo(vg, w - 2, h * 0.5f);
        nvgStrokeColor(vg, cor::linha()); nvgStrokeWidth(vg, 1.0f); nvgStroke(vg);
        if (m && m->visorModo == 1) {   // XY: a trajetória dos últimos ~0,5 s
            constexpr int N = Casca<D>::kVisor;
            const int w0 = m->visorW;
            float pk = 1e-6f;
            for (int i = 0; i < N; ++i) pk = std::max({pk, std::fabs(m->visor[i]), std::fabs(m->visorY[i])});
            if (pk > 0.002f) {
                const float cxv = w * 0.5f, cyv = h * 0.5f, r = std::min(w, h) * 0.5f - 3.0f;
                nvgBeginPath(vg);
                for (int i = 0; i < N; ++i) {
                    const int k = (w0 + i) % N;
                    const float x = cxv + m->visor[k] / pk * r, y = cyv - m->visorY[k] / pk * r;
                    if (i == 0) nvgMoveTo(vg, x, y); else nvgLineTo(vg, x, y);
                }
                nvgStrokeColor(vg, traco); nvgStrokeWidth(vg, 1.1f); nvgStroke(vg);
                const int k = (w0 + N - 1) % N;   // o ponto atual
                nvgBeginPath(vg); nvgCircle(vg, cxv + m->visor[k] / pk * r, cyv - m->visorY[k] / pk * r, 2.2f);
                nvgFillColor(vg, traco); nvgFill(vg);
            }
        } else if (m && m->visorModo == 2) {   // pistas: um pulso aceso por gatilho, no tempo
            constexpr int N = Casca<D>::kVisor;
            const int w0 = m->visorW, np = m->nPistas;
            const float alt = (h - 4.0f) / static_cast<float>(np);
            nvgFillColor(vg, traco);
            for (int l = 0; l < np; ++l) {
                const float y0 = 2.0f + alt * static_cast<float>(l);
                for (int i = 0; i < N; ++i)
                    if (m->pistas[l][(w0 + i) % N] >= 0.5f) {
                        const float x = 2.0f + (w - 4.0f) * static_cast<float>(i) / static_cast<float>(N);
                        nvgBeginPath(vg); nvgRect(vg, x, y0 + 1.0f, std::max(1.0f, (w - 4.0f) / N), alt - 2.0f); nvgFill(vg);
                    }
            }
        } else if (m) {
            constexpr int N = Casca<D>::kVisor;
            float v[N];
            const int w0 = m->visorW;
            for (int i = 0; i < N; ++i) v[i] = m->visor[(w0 + i) % N];
            int ini = 0, n = N;
            if (!m->visorLento) {   // um disparo: o último cruzamento com metade do anel depois
                n = N / 2;
                for (int i = N / 2 - 1; i > 0; --i)
                    if (v[i - 1] < 0.0f && v[i] >= 0.0f) { ini = i; break; }
            }
            float lo = 1e9f, hi = -1e9f;
            for (int i = ini; i < ini + n; ++i) { lo = std::min(lo, v[i]); hi = std::max(hi, v[i]); }
            // abaixo de ~-54 dB (0,002 na escala do Rasgo) não há sinal: só a
            // linha reta. Sem isto o visor ampliava restos minúsculos (sem
            // cabo) até a altura toda, como se houvesse sinal (autor, 24/11)
            const float pico = std::max(std::fabs(lo), std::fabs(hi));
            const bool temSinal = m->visorLento ? (hi - lo) > 0.002f : pico > 0.002f;
            if (temSinal && hi > lo + 1e-6f) {
                nvgBeginPath(vg);
                for (int i = 0; i < n; ++i) {
                    const float x = 2.0f + (w - 4.0f) * static_cast<float>(i) / static_cast<float>(n - 1);
                    float y;
                    if (m->visorLento) y = h - 3.0f - (v[ini + i] - std::min(lo, 0.0f)) / (hi - std::min(lo, 0.0f)) * (h - 6.0f);
                    else { const float pk = std::max(std::fabs(lo), std::fabs(hi)); y = h * 0.5f - v[ini + i] / pk * (h * 0.5f - 3.0f); }
                    if (i == 0) nvgMoveTo(vg, x, y); else nvgLineTo(vg, x, y);
                }
                nvgStrokeColor(vg, traco); nvgStrokeWidth(vg, 1.3f); nvgStroke(vg);
            }
        }
        auto f = fonte(false);
        if (f && !rotulo.empty()) {
            nvgFontFaceId(vg, f->handle); nvgFontSize(vg, 8.0f);
            nvgFillColor(vg, cor::textoSec());
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
            nvgText(vg, 3.0f, h - 2.0f, rotulo.c_str(), nullptr);
        }
    }
};

// Layout normalizado (autor, 24/11: "deixe-os no mesmo padrão de espaçamentos
// e layout"): as fileiras do app são mantidas (os agrupamentos do autor), mas
// cada uma é recolocada num passo padrão, centralizada; as fileiras descem num
// passo padrão (knobs, jacks, sliders), e o HP passa a ser o que a fileira mais
// longa pede. O visor fica no topo, na largura do painel. Em mm.
// (descartado em 24/11: "não precisa uniformizar a organização… cada módulo
// pode ter seu estilo pessoal"; fica como opção de build, desligado)
#ifndef RASGO_LAYOUT
#define RASGO_LAYOUT 0
#endif
inline void normalizarLayout(rasgo::modular::Panel& p) {
    using rasgo::modular::Widget;
    constexpr float kPassoKnob = 12.7f, kPassoJack = 10.16f, kMargem = 3.0f;
    constexpr float kFilaKnob = 15.5f, kFilaJack = 11.5f, kFilaSlider = 38.0f, kFilaGrande = 19.0f;
    std::vector<Widget> visores, controles;
    for (const auto& w : p.widgets) {
        if (w.kind == WK::Display) visores.push_back(w);
        else if (w.kind != WK::Label) controles.push_back(w);
    }
    // o centro vertical de cada controle (para agrupar em fileiras)
    auto cy = [](const Widget& w) {
        if (w.kind == WK::Knob) return w.y + (w.span > 1.0f ? w.span : 8.0f) * 0.5f;
        if (w.kind == WK::Slider) return w.y + 15.0f;
        if (w.kind == WK::Toggle) return w.y + 2.0f;
        return w.y;
    };
    auto cx = [](const Widget& w) {
        if (w.kind == WK::Knob) return w.x + (w.span > 1.0f ? w.span : 8.0f) * 0.5f;
        if (w.kind == WK::Slider) return w.x + 3.0f;
        if (w.kind == WK::Toggle) return w.x + 2.0f;
        return w.x;
    };
    std::sort(controles.begin(), controles.end(), [&](const Widget& a, const Widget& b) { return cy(a) < cy(b); });
    std::vector<std::vector<Widget>> filas;
    for (const auto& w : controles) {
        const bool jack = w.kind == WK::Jack;
        if (!filas.empty()) {
            const auto& f = filas.back();
            const bool fJack = f.front().kind == WK::Jack;
            if (std::fabs(cy(f.front()) - cy(w)) < 4.0f && fJack == jack) { filas.back().push_back(w); continue; }
        }
        filas.push_back({w});
    }
    float largura = 0.0f;
    for (auto& f : filas) {
        std::sort(f.begin(), f.end(), [&](const Widget& a, const Widget& b) { return cx(a) < cx(b); });
        const float passo = f.front().kind == WK::Jack ? kPassoJack : kPassoKnob;
        largura = std::max(largura, passo * static_cast<float>(f.size()));
    }
    if (!visores.empty()) largura = std::max(largura, 30.0f);
    const int hp = std::max(6, static_cast<int>(std::ceil((largura + 2.0f * kMargem) / 5.08f)));
    const float W = static_cast<float>(hp) * 5.08f;
    // a altura que as fileiras pedem; se não couber, o passo vertical encolhe
    float y = 7.5f;
    float alturaVisores = 0.0f;
    for (const auto& v : visores) alturaVisores += v.height + 3.0f;
    float pedida = 0.0f;
    for (const auto& f : filas) {
        bool grande = false, slider = false;
        for (const auto& w : f) { if (w.kind == WK::Slider) slider = true; if (w.kind == WK::Knob && w.span > 11.0f) grande = true; }
        pedida += slider ? kFilaSlider : (f.front().kind == WK::Jack ? kFilaJack : (grande ? kFilaGrande : kFilaKnob));
    }
    const float disponivel = 117.0f - (7.5f + alturaVisores + 4.0f);
    const float k = pedida > disponivel ? disponivel / pedida : 1.0f;
    std::vector<Widget> out;
    for (const auto& w : p.widgets) if (w.kind == WK::Label) out.push_back(w);
    y = 7.5f;
    for (auto v : visores) { v.x = 2.5f; v.span = W - 5.0f; v.y = y; y += v.height + 3.0f; out.push_back(v); }
    y += 4.0f;   // o rótulo da primeira fileira
    // folga que sobra (fileiras menores que o espaço): distribuída acima de cada fileira de jacks
    for (const auto& f : filas) {
        bool grande = false, slider = false;
        for (const auto& w : f) { if (w.kind == WK::Slider) slider = true; if (w.kind == WK::Knob && w.span > 11.0f) grande = true; }
        const bool jack = f.front().kind == WK::Jack;
        const float alto = (slider ? kFilaSlider : (jack ? kFilaJack : (grande ? kFilaGrande : kFilaKnob))) * k;
        const float passo = jack ? kPassoJack : kPassoKnob;
        const float x0 = (W - passo * static_cast<float>(f.size())) * 0.5f + passo * 0.5f;   // centro da 1ª célula
        for (std::size_t i = 0; i < f.size(); ++i) {
            Widget w = f[i];
            const float c = x0 + passo * static_cast<float>(i);
            if (w.kind == WK::Jack) { w.x = c; w.y = y + 3.5f; }
            else if (w.kind == WK::Knob) { const float d = w.span > 1.0f ? w.span : 8.0f; w.x = c - d * 0.5f; w.y = y; }
            else if (w.kind == WK::Slider) { w.x = c - 3.0f; w.y = y; }
            else { w.x = c - 2.0f; w.y = y + 2.0f; }
            out.push_back(w);
        }
        y += alto;
    }
    p.widgets = out;
    p.hp = hp;
}

// Aproxima as fileiras muito espaçadas (autor, 24/11: KEY, TAP, SWITCH…):
// numa fileira em que todos os vizinhos estão a mais de 18 mm, os elementos
// se juntam em volta do centro da própria fileira, no passo padrão (12,7 mm
// knobs, 10,16 mm jacks) ou no que os rótulos pedem. Mantém a ordem e o
// centro; as outras fileiras não mudam. `larg` como em `estreitar`.
inline void aproximarFileiras(rasgo::modular::Panel& p, const std::vector<float>& larg) {
    struct Item { std::size_t i; float cx, cy, meia; bool jack; };
    std::vector<Item> it;
    for (std::size_t i = 0; i < p.widgets.size(); ++i) {
        const auto& w = p.widgets[i];
        if (w.kind == WK::Label || w.kind == WK::Display) continue;
        float cx = w.x, cy = w.y;
        if (w.kind == WK::Knob) { const float d = w.span > 1.0f ? w.span : 8.0f; cx += d * 0.5f; cy += d * 0.5f; }
        else if (w.kind == WK::Slider) { cx += 3.0f; cy += 15.0f; }
        else if (w.kind == WK::Toggle) { cx += 2.0f; cy += 2.0f; }
        const float rot = 1.55f * static_cast<float>(w.label.size()) + 1.0f;
        it.push_back({i, cx, cy, 0.5f * std::max(larg[i], rot), w.kind == WK::Jack});
    }
    std::vector<char> feito(it.size(), 0);
    for (std::size_t a = 0; a < it.size(); ++a) {
        if (feito[a]) continue;
        std::vector<std::size_t> fila;
        for (std::size_t b = 0; b < it.size(); ++b)
            if (!feito[b] && std::fabs(it[b].cy - it[a].cy) < 4.0f) { fila.push_back(b); feito[b] = 1; }
        if (fila.size() < 2) continue;
        std::sort(fila.begin(), fila.end(), [&](std::size_t x, std::size_t y) { return it[x].cx < it[y].cx; });
        float menor = 1e9f;
        for (std::size_t k = 1; k < fila.size(); ++k) menor = std::min(menor, it[fila[k]].cx - it[fila[k - 1]].cx);
        if (menor <= 18.0f) continue;
        const float centro = 0.5f * (it[fila.front()].cx + it[fila.back()].cx);
        // as posições novas, no passo de cada par (o padrão ou o que os rótulos pedem)
        std::vector<float> pos(fila.size(), 0.0f);
        for (std::size_t k = 1; k < fila.size(); ++k) {
            const auto& A = it[fila[k - 1]]; const auto& B = it[fila[k]];
            const float padrao = (A.jack && B.jack) ? 10.16f : 12.7f;
            pos[k] = pos[k - 1] + std::max(padrao, A.meia + B.meia + 1.5f);
        }
        const float meio = 0.5f * pos.back();
        for (std::size_t k = 0; k < fila.size(); ++k) {
            auto& w = p.widgets[it[fila[k]].i];
            w.x += (centro - meio + pos[k]) - it[fila[k]].cx;
        }
    }
}

// Estreita o painel sem encolher nada (autor, 24/11: "opte por módulos mais
// estreitos, sem encolher os objetos"; "cada módulo pode ter seu estilo
// pessoal, mas com certa padronização no conjunto"): a organização do app é
// mantida e as posições horizontais são apertadas por um fator único, até o
// limite em que dois vizinhos de uma mesma fileira (objeto ou rótulo) ficariam
// a menos de 1,5 mm. `larg` = largura de cada widget no Rack (mm), na ordem
// de `p.widgets`. Devolve o fator (1 = não estreitou).
inline float estreitar(rasgo::modular::Panel& p, const std::vector<float>& larg) {
    const float W0 = static_cast<float>(p.hp) * 5.08f;
    struct Item { std::size_t i; float cx, cy, meia; };
    std::vector<Item> it;
    for (std::size_t i = 0; i < p.widgets.size(); ++i) {
        const auto& w = p.widgets[i];
        if (w.kind == WK::Label || w.kind == WK::Display) continue;
        float cx = w.x, cy = w.y;
        if (w.kind == WK::Knob) { const float d = w.span > 1.0f ? w.span : 8.0f; cx += d * 0.5f; cy += d * 0.5f; }
        else if (w.kind == WK::Slider) { cx += 3.0f; cy += 15.0f; }
        else if (w.kind == WK::Toggle) { cx += 2.0f; cy += 2.0f; }
        // a meia-largura: o maior entre o objeto e o rótulo (~1,55 mm por letra)
        const float rot = 1.55f * static_cast<float>(w.label.size()) + 1.0f;
        it.push_back({i, cx, cy, 0.5f * std::max(larg[i], rot)});
    }
    if (it.size() < 2) return 1.0f;
    float s = 0.0f;   // o menor fator que ainda respeita todos os pares
    for (std::size_t a = 0; a < it.size(); ++a)
        for (std::size_t b = a + 1; b < it.size(); ++b) {
            if (std::fabs(it[a].cy - it[b].cy) > 6.0f) continue;      // fileiras diferentes
            const float dx = std::fabs(it[a].cx - it[b].cx);
            if (dx < 0.5f) continue;
            s = std::max(s, (it[a].meia + it[b].meia + 1.5f) / dx);
        }
    s = std::min(1.0f, std::max(s, 0.3f));
    float xmin = 1e9f, xmax = -1e9f;
    for (const auto& e : it) { xmin = std::min(xmin, e.cx - e.meia); xmax = std::max(xmax, e.cx + e.meia); }
    const float conteudo = (xmax - xmin) * s;
    // nunca mais estreito que o título e o wordmark pedem
    const int hp = std::max({6, static_cast<int>(std::ceil((conteudo + 6.0f) / 5.08f)), std::min(p.hp, 7)});
    if (hp >= p.hp) return 1.0f;
    const float W = static_cast<float>(hp) * 5.08f;
    const float desloc = (W - conteudo) * 0.5f;
    for (const auto& e : it) {
        auto& w = p.widgets[e.i];
        const float ncx = desloc + (e.cx - xmin) * s;
        w.x += ncx - e.cx;
    }
    for (auto& w : p.widgets)
        if (w.kind == WK::Display) { w.x = 2.5f; w.span = W - 5.0f; }
        else if (w.kind == WK::Label) w.x = std::min(w.x, W - 4.0f);
    (void)W0;
    p.hp = hp;
    return s;
}

// Sobe os controles (autor, 24/11: "suba os controles se for o caso") quando
// algum controle de baixo ocuparia a faixa do wordmark centralizado: o bloco
// abaixo do visor é comprimido para cima, mantendo as proporções. Em mm.
inline float subirControles(rasgo::modular::Panel& p) {
    const float W = static_cast<float>(p.hp) * 5.08f, H = 128.5f;
    const float lw = 22.0f, lh = lw * 30.75f / 258.74f;
    const float topo = H - 1.35f - 0.5f * (5.08f + lh);   // o topo do wordmark (mm)
    const float x0 = (W - lw) * 0.5f, x1 = x0 + lw, folga = 1.2f;
    auto fundo = [](const rasgo::modular::Widget& w) {
        if (w.kind == WK::Jack) return w.y + 4.3f;
        if (w.kind == WK::Knob) return w.y + (w.span > 1.0f ? w.span : 8.0f) + 0.7f;
        if (w.kind == WK::Slider) return w.y + 34.0f;
        if (w.kind == WK::Toggle) return w.y + 9.0f;
        return 0.0f;
    };
    auto meio = [](const rasgo::modular::Widget& w, float& a, float& b) {
        if (w.kind == WK::Jack) { a = w.x - 4.3f; b = w.x + 4.3f; }
        else if (w.kind == WK::Knob) { const float d = w.span > 1.0f ? w.span : 8.0f; a = w.x - 0.1f * d; b = w.x + 1.1f * d; }
        else { a = w.x - 1.0f; b = w.x + 7.0f; }
    };
    bool bate = false;
    float maxB = 0.0f, y0 = 1e9f;
    for (const auto& w : p.widgets) {
        if (w.kind == WK::Display || w.kind == WK::Label) continue;
        const float f = fundo(w);
        maxB = std::max(maxB, f);
        y0 = std::min(y0, (w.kind == WK::Jack ? w.y - 4.3f : w.y) - 4.5f);   // com o rótulo
        float a, b; meio(w, a, b);
        if (b > x0 && a < x1 && f > topo - folga) bate = true;
    }
    if (!bate || maxB <= y0) return 1.0f;
    const float k = std::min(1.0f, (topo - folga - y0) / (maxB - y0));
    for (auto& w : p.widgets)
        if (w.kind != WK::Display && w.kind != WK::Label) w.y = y0 + (w.y - y0) * k;
    return k;
}

template <class D>
struct CascaWidget : rack::app::ModuleWidget {
    std::string tipo;
    // os créditos no botão direito (o "User manual" do Rack já abre o guia)
    void appendContextMenu(rack::ui::Menu* menu) override {
        menu->addChild(new rack::ui::MenuSeparator);
        menu->addChild(rack::createMenuLabel("Rasgo Modular — Lúcio de Araújo"));
        if (const char* o = origemDe(tipo)) menu->addChild(rack::createMenuLabel(o));
        menu->addChild(rack::createMenuItem("Rasgo Modular (website)", "", [] {
            rack::system::openBrowser("https://lucioaraujo.github.io/rasgo-modular/");
        }));
        const std::string d = dossieDe(tipo);
        if (!d.empty())
            menu->addChild(rack::createMenuItem("Sources and credits (dossier)", "", [d] {
                rack::system::openBrowser("https://github.com/lucioaraujo/rasgo-modular/blob/main/dossies/" + d);
            }));
        menu->addChild(rack::createMenuItem("RASGO instruments", "", [] {
            rack::system::openBrowser("https://rasgoinstruments.arquiviagem.net/");
        }));
    }
    explicit CascaWidget(Casca<D>* m) {
        setModule(m);
        D modelo;   // o painel não depende do estado
        tipo = modelo.type();
        rasgo::modular::Panel p = modelo.panel();
#if RASGO_LAYOUT
        normalizarLayout(p);
#endif
        // o tamanho de cada knob sai do layout original (antes de estreitar)
        std::vector<int> tam(p.widgets.size(), 0);
        std::vector<float> larg(p.widgets.size(), 0.0f);
        for (std::size_t k = 0; k < p.widgets.size(); ++k) {
            const auto& w = p.widgets[k];
            if (w.kind == WK::Knob) {
#if RASGO_ESTILO == 3
                tam[k] = tamanhoKnob(p, w);
#endif
                larg[k] = tam[k] == 2 ? 12.7f : (tam[k] == 1 ? 10.0f : 8.0f);
            } else if (w.kind == WK::Jack) larg[k] = 8.4f;
            else if (w.kind == WK::Slider) larg[k] = 6.0f;
            else if (w.kind == WK::Toggle) larg[k] = 6.0f;
        }
        aproximarFileiras(p, larg);
        const float apertou = estreitar(p, larg);
        const float subiu = subirControles(p);
        auto* fundo = new PainelRasgo;
        fundo->painel = p;
        fundo->titulo = modelo.type();
        fundo->familia = familiaDe(modelo.type());
        fundo->diam.assign(p.widgets.size(), 0.0f);
        fundo->box.size = rack::math::Vec(p.hp * rack::app::RACK_GRID_WIDTH, rack::app::RACK_GRID_HEIGHT);
        setPanel(fundo);
        // os parafusos do Rack (prata): no topo só à direita — à esquerda
        // fica o título (autor, 23/11). Embaixo, o wordmark RASGO vai sempre
        // no tamanho cheio (autor: "não quero que diminua a logo… se for o
        // caso utilize somente um parafuso"): no centro com os dois
        // parafusos; se um controle de baixo atrapalha, encostado num lado,
        // sem o parafuso daquele lado.
        {
            using rack::app::RACK_GRID_WIDTH;
            using rack::app::RACK_GRID_HEIGHT;
            const float W = fundo->box.size.x, H = fundo->box.size.y;
            const float lw = rack::mm2px(22.0f), lh = lw * 30.75f / 258.74f;
            const float topo = H - 4.0f - 0.5f * (RACK_GRID_WIDTH + lh);
            const float pE = RACK_GRID_WIDTH + 15.0f, pD = W - 2 * RACK_GRID_WIDTH;   // fim do parafuso esq., início do dir.
            auto bate = [&](float xa, float xb) {
                for (const auto& w : p.widgets) {
                    float cx = rack::mm2px(w.x), meia = 0.0f, fundoW = 0.0f;
                    if (w.kind == WK::Jack) { meia = rack::mm2px(4.3f); fundoW = rack::mm2px(w.y) + meia; }
                    else if (w.kind == WK::Knob) { const float d = w.span > 1.0f ? w.span : 8.0f; cx = rack::mm2px(w.x + d * 0.5f); meia = rack::mm2px(d * 0.6f); fundoW = rack::mm2px(w.y + d) + 2.0f; }
                    else if (w.kind == WK::Toggle) { meia = rack::mm2px(3.0f); fundoW = rack::mm2px(w.y + 9.0f); }
                    else continue;
                    if (cx + meia > xa && cx - meia < xb && fundoW > topo - 2.0f) return true;
                }
                return false;
            };
            bool esq = true, dir = true;
            float x = (W - lw) * 0.5f;
            if (!(x >= pE + 3.0f && x + lw <= pD - 3.0f && !bate(x, x + lw))) {
                const float xe = 5.0f, xd = W - 5.0f - lw;
                if (xe + lw <= pD - 3.0f && !bate(xe, xe + lw)) { x = xe; esq = false; }
                else if (xd >= pE + 3.0f && !bate(xd, xd + lw)) { x = xd; dir = false; }
                else if (!bate(xe, xe + lw)) { x = xe; esq = false; dir = false; }
                else { x = (W - lw) * 0.5f; esq = dir = false; }   // último recurso: no centro, sem parafusos
            }
            fundo->logoX = x; fundo->logoTopo = topo; fundo->logoW = lw;
            addChild(rack::createWidget<rack::componentlibrary::ScrewSilver>(rack::math::Vec(pD, 0)));
            if (esq) addChild(rack::createWidget<rack::componentlibrary::ScrewSilver>(
                rack::math::Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            if (dir) addChild(rack::createWidget<rack::componentlibrary::ScrewSilver>(
                rack::math::Vec(pD, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            if (std::getenv("RASGO_LOGO_LOG"))
                std::fprintf(stderr, "[logo] %s %s k=%.3f hp=%d aperto=%.2f\n", modelo.type().c_str(),
                             esq && dir ? "centro" : esq ? "direita" : dir ? "esquerda" : "sem parafusos", subiu, p.hp, apertou);
        }
        const auto paramIndex = [&](const std::string& id) {
            const auto& ps = modelo.parameters();
            for (std::size_t i = 0; i < ps.size(); ++i)
                if (ps[i].descriptor.id == id) return static_cast<int>(i);
            return -1;
        };
        const auto portIndex = [&](const std::string& nome, bool saida) {
            const std::size_t n = saida ? modelo.outputCount() : modelo.inputCount();
            for (std::size_t i = 0; i < n; ++i)
                if ((saida ? modelo.outputDescriptor(i) : modelo.inputDescriptor(i)).name == nome)
                    return static_cast<int>(i);
            return -1;
        };
        int luz = 0;
        (void)luz;
        for (const auto& w : p.widgets) {
            if (w.kind == WK::Display) {
                auto* vis = new VisorRasgo<D>;
                vis->m = m;
                vis->rotulo = w.label;
                vis->traco = cor::familia(familiaDe(modelo.type()));
                vis->box.pos = rack::mm2px(rack::math::Vec(w.x, w.y));
                vis->box.size = rack::mm2px(rack::math::Vec(w.span > 1.0f ? w.span : 16.0f, w.height));
                addChild(vis);
                continue;
            }
            if (w.kind == WK::Jack) {
                const bool saida = w.bind.rfind("out:", 0) == 0;
                const int i = portIndex(w.bind.substr(saida ? 4 : 3), saida);
                if (i < 0) continue;
                const auto c = rack::mm2px(rack::math::Vec(w.x, w.y));
                if (saida) addOutput(rack::createOutputCentered<PecaJack>(c, m, i));
                else addInput(rack::createInputCentered<PecaJack>(c, m, i));
                continue;
            }
            const int i = paramIndex(w.bind);
            if (i < 0) continue;
            if (w.kind == WK::Knob) {
                const float d = w.span > 1.0f ? w.span : 8.0f;
                const auto c = rack::mm2px(rack::math::Vec(w.x + d * 0.5f, w.y + d * 0.5f));
                rack::app::ParamWidget* pw = nullptr;
#if RASGO_ESTILO == 3
                const int tk = tam[static_cast<std::size_t>(&w - p.widgets.data())];
                if (tk == 2) pw = rack::createParamCentered<KnobVcvGrande>(c, m, i);
                else if (tk == 1) pw = rack::createParamCentered<KnobVcvMedio>(c, m, i);
                else pw = rack::createParamCentered<PecaKnob>(c, m, i);
#else
                pw = rack::createParamCentered<PecaKnob>(c, m, i);
#endif
                addParam(pw);
                fundo->diam[static_cast<std::size_t>(&w - p.widgets.data())] = pw->box.size.x;
            } else if (w.kind == WK::Slider) {
                addParam(rack::createParam<PecaSlider>(rack::mm2px(rack::math::Vec(w.x, w.y)), m, i));
            } else if (w.kind == WK::Toggle) {
                addParam(rack::createParam<PecaChave>(rack::mm2px(rack::math::Vec(w.x, w.y)), m, i));
            }
        }
    }
};

}  // namespace rasgo_vcv
