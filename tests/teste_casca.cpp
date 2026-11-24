// Teste da casca sem janela (23/11/2026): o OSC e o LADDER do Rasgo pela
// casca do VCV Rack — altura, tensões (±5 V) e o atraso de um bloco.
#include "../src/Casca.hpp"
#include "dsp/Ladder.hpp"
#include "dsp/Oscillator.hpp"
#include <cmath>
#include <cstdio>

rack::plugin::Plugin* pluginInstance = nullptr;
using namespace rasgo_vcv;
namespace rm = rasgo::modular;

static int falhas = 0;
static void check(bool c, const char* e) { if (!c) { std::printf("FALHOU: %s\n", e); ++falhas; } }

int main() {
    rack::engine::Module::ProcessArgs a;
    a.sampleRate = 48000.0f; a.sampleTime = 1.0f / 48000.0f; a.frame = 0;

    Casca<rm::Oscillator> osc;
    int fr = -1;
    for (std::size_t i = 0; i < osc.ids.size(); ++i) if (osc.ids[i] == "freq") fr = static_cast<int>(i);
    check(fr >= 0, "OSC tem o parâmetro freq");
    osc.params[fr].setValue(440.0f);
    int cruz = 0; float pico = 0.0f, ant = 0.0f; int primeiro = -1;
    for (int n = 0; n < 48000; ++n) {
        osc.process(a);
        const float v = osc.outputs[0].getVoltage();   // SIN
        if (primeiro < 0 && v != 0.0f) primeiro = n;
        if (n > 4800) { if (ant <= 0.0f && v > 0.0f) ++cruz; pico = std::max(pico, std::fabs(v)); }
        ant = v;
    }
    std::printf("OSC 440 Hz: %d ciclos em 0,9 s, pico %.2f V, primeira amostra em %d\n", cruz, pico, primeiro);
    check(std::abs(cruz - 396) <= 3, "altura: 440 Hz");
    check(pico > 4.0f && pico < 5.6f, "nível: ±5 V");
    check(primeiro >= kBloco && primeiro < 2 * kBloco, "atraso de um bloco (16 amostras)");

    // 1 V/oct: +1 V na entrada dobra a frequência
    Casca<rm::Oscillator> o2;
    o2.params[fr].setValue(220.0f);
    int pin = -1;
    for (std::size_t i = 0; i < o2.inputs.size(); ++i) if (o2.dsp.inputDescriptor(i).name == "pitch") pin = static_cast<int>(i);
    check(pin >= 0, "OSC tem a entrada pitch");
    o2.inputs[pin].channels = 1; o2.inputs[pin].setVoltage(1.0f);   // (o cabo é que liga a porta)
    cruz = 0; ant = 0.0f;
    for (int n = 0; n < 48000; ++n) { o2.process(a); const float v = o2.outputs[0].getVoltage(); if (n > 4800 && ant <= 0.0f && v > 0.0f) ++cruz; ant = v; }
    std::printf("OSC 220 Hz + 1 V: %d ciclos em 0,9 s\n", cruz);
    check(std::abs(cruz - 396) <= 4, "1 V/oct: 220 Hz + 1 V = 440 Hz");

    // LADDER: ruído ±5 V entra, sai filtrado e finito
    Casca<rm::Ladder> lad;
    lad.inputs[0].channels = 1;
    std::uint32_t s = 1; float energia = 0.0f; bool finito = true;
    for (int n = 0; n < 48000; ++n) {
        s = s * 1664525u + 1013904223u;
        lad.inputs[0].setVoltage(((s >> 8) / 16777216.0f * 2.0f - 1.0f) * 5.0f);
        lad.process(a);
        const float v = lad.outputs[0].getVoltage();
        if (!std::isfinite(v)) finito = false;
        energia += v * v;
    }
    std::printf("LADDER: rms %.2f V\n", std::sqrt(energia / 48000.0f));
    check(finito && energia > 0.0f, "LADDER soa e fica finito");

    // polifonia: 4 vozes de altura (0, 1, 2, 3 V) num OSC de 110 Hz → 4 canais
    Casca<rm::Oscillator> po;
    po.params[fr].setValue(110.0f);
    po.inputs[pin].channels = 4;
    po.outputs[0].channels = 1;   // a saída com cabo (sem cabo o Rack mantém 0 canais)
    for (int c = 0; c < 4; ++c) po.inputs[pin].setVoltage(static_cast<float>(c), c);
    int cz[4] = {0, 0, 0, 0}; float an[4] = {0, 0, 0, 0};
    for (int n = 0; n < 48000; ++n) {
        po.process(a);
        for (int c = 0; c < 4; ++c) { const float v = po.outputs[0].getVoltage(c); if (n > 4800 && an[c] <= 0.0f && v > 0.0f) ++cz[c]; an[c] = v; }
    }
    std::printf("polifonia: %d canais; ciclos em 0,9 s: %d %d %d %d\n", po.outputs[0].getChannels(), cz[0], cz[1], cz[2], cz[3]);
    check(po.outputs[0].getChannels() == 4, "4 vozes na entrada → 4 canais na saída");
    check(std::abs(cz[0] - 99) <= 2 && std::abs(cz[1] - 198) <= 3 && std::abs(cz[2] - 396) <= 4 && std::abs(cz[3] - 792) <= 6,
          "cada voz na sua altura (110, 220, 440, 880 Hz)");

    std::printf(falhas ? "%d falha(s)\n" : "casca: tudo certo\n", falhas);
    return falhas ? 1 : 0;
}
