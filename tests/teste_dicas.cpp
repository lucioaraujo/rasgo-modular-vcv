// As dicas do LEARN em inglês chegam aos knobs e jacks (23/11/2026).
#include "../src/Casca.hpp"
#include "dsp/Oscillator.hpp"
#include "dsp/Rungler.hpp"
#include <cstdio>
rack::plugin::Plugin* pluginInstance = nullptr;
using namespace rasgo_vcv;
int main() {
    Casca<rasgo::modular::Oscillator> o;
    Casca<rasgo::modular::Rungler> r;
    int comDica = 0, total = 0;
    for (auto* q : o.paramQuantities) { ++total; if (!q->description.empty()) ++comDica; }
    for (auto* i : o.inputInfos) { ++total; if (!i->description.empty()) ++comDica; }
    std::printf("OSC: %d de %d com dica\n", comDica, total);
    std::printf("FREQ: %s\n", o.paramQuantities[0]->description.c_str());
    std::printf("RUNGLER: %s\n", origemDe("RUNGLER"));
    return comDica > total / 2 ? 0 : 1;
}
