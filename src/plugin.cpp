// O Rasgo Modular no VCV Rack 2: os módulos do catálogo, cada um pela casca
// genérica (`Casca.hpp`). A lista vem de `tools/gerar_plugin.py`.
#include "plugin.hpp"
#include "Casca.hpp"
#include "modulos.inc"

using namespace rasgo_vcv;
namespace rm = rasgo::modular;

rack::plugin::Plugin* pluginInstance;

void init(rack::plugin::Plugin* p) {
    pluginInstance = p;
#define RASGO_MODELO(Classe, slug) p->addModel(rack::createModel<Casca<rm::Classe>, CascaWidget<rm::Classe>>(slug));
    RASGO_MODELOS(RASGO_MODELO)
#undef RASGO_MODELO
}
