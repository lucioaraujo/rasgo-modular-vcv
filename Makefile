# Piloto do Rasgo Modular para o VCV Rack 2 (23/11/2026).
# RACK_DIR: o SDK gratuito da VCV (Rack SDK 2), fora do repositório.
RACK_DIR ?= ../Rack-SDK

# -iquote, not -I: the Rasgo DSP has dsp/Filter.hpp, and on case-insensitive
# file systems (macOS, Windows) the SDK's <dsp/filter.hpp> would find it.
FLAGS += -iquote src/rasgo $(RASGO_FLAGS)
SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk

# O DSP do Rasgo é C++17 (há precedente no Cardinal: Befaco, SurgeXT); a
# última opção vale sobre o -std=c++11 do SDK.
CXXFLAGS += -std=gnu++17
