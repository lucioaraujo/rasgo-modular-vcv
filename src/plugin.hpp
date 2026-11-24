#pragma once
// The SDK's rack.hpp includes dsp/midi.hpp before dsp/filter.hpp, and midi.hpp
// uses ExponentialFilter; Apple clang and GCC 15 reject that. Including the
// filter first keeps every compiler happy.
#include <dsp/filter.hpp>
#include <rack.hpp>

extern rack::plugin::Plugin* pluginInstance;
