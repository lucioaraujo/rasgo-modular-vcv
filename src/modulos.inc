// GERADO por tools/gerar_plugin.py (Rasgo Modular ed68437). Não editar.
#include "dsp/Abacus.hpp"
#include "dsp/Accent.hpp"
#include "dsp/Additive.hpp"
#include "dsp/Apocrifo.hpp"
#include "dsp/Arp.hpp"
#include "dsp/Blefe.hpp"
#include "dsp/Bounce.hpp"
#include "dsp/Boxcar.hpp"
#include "dsp/Budget.hpp"
#include "dsp/Cell.hpp"
#include "dsp/Chaos.hpp"
#include "dsp/Chord.hpp"
#include "dsp/Coherence.hpp"
#include "dsp/Comparador.hpp"
#include "dsp/Control.hpp"
#include "dsp/Crush.hpp"
#include "dsp/CutUp.hpp"
#include "dsp/Decision.hpp"
#include "dsp/Drift.hpp"
#include "dsp/Drum.hpp"
#include "dsp/Dynamics.hpp"
#include "dsp/Eclipse.hpp"
#include "dsp/Elastic.hpp"
#include "dsp/Envelope.hpp"
#include "dsp/Epicycle.hpp"
#include "dsp/EuclidClock.hpp"
#include "dsp/Filter.hpp"
#include "dsp/Form.hpp"
#include "dsp/Formant.hpp"
#include "dsp/Fronteira.hpp"
#include "dsp/FunctionGenerator.hpp"
#include "dsp/Gendyn.hpp"
#include "dsp/Glide.hpp"
#include "dsp/Hall.hpp"
#include "dsp/Harmony.hpp"
#include "dsp/Interference.hpp"
#include "dsp/Interpret.hpp"
#include "dsp/Key.hpp"
#include "dsp/Ladder.hpp"
#include "dsp/Lens.hpp"
#include "dsp/Listen.hpp"
#include "dsp/Logic.hpp"
#include "dsp/Looper.hpp"
#include "dsp/Lpg.hpp"
#include "dsp/Magnet.hpp"
#include "dsp/Markov.hpp"
#include "dsp/Matrix.hpp"
#include "dsp/Matter.hpp"
#include "dsp/Memory.hpp"
#include "dsp/Mousse.hpp"
#include "dsp/Mult.hpp"
#include "dsp/Noise.hpp"
#include "dsp/Operator.hpp"
#include "dsp/Orbit.hpp"
#include "dsp/Oscillator.hpp"
#include "dsp/Parametric.hpp"
#include "dsp/Pendulum.hpp"
#include "dsp/Phrase.hpp"
#include "dsp/Planar.hpp"
#include "dsp/Pll.hpp"
#include "dsp/Polygon.hpp"
#include "dsp/Predator.hpp"
#include "dsp/Pressure.hpp"
#include "dsp/Prism.hpp"
#include "dsp/Pulsar.hpp"
#include "dsp/Quantizer.hpp"
#include "dsp/Refract.hpp"
#include "dsp/Relate.hpp"
#include "dsp/Resonator.hpp"
#include "dsp/Rungler.hpp"
#include "dsp/SampleHold.hpp"
#include "dsp/Sampler.hpp"
#include "dsp/Scope.hpp"
#include "dsp/Season.hpp"
#include "dsp/Shape.hpp"
#include "dsp/Shifter.hpp"
#include "dsp/Space.hpp"
#include "dsp/Spectra.hpp"
#include "dsp/Spring.hpp"
#include "dsp/Stages.hpp"
#include "dsp/StepSequencer.hpp"
#include "dsp/StringVoice.hpp"
#include "dsp/Sum.hpp"
#include "dsp/Supply.hpp"
#include "dsp/Swarm.hpp"
#include "dsp/Swirl.hpp"
#include "dsp/Switch.hpp"
#include "dsp/Tap.hpp"
#include "dsp/Tape.hpp"
#include "dsp/Tempo.hpp"
#include "dsp/TrigSeq.hpp"
#include "dsp/TuringLoop.hpp"
#include "dsp/Turntable.hpp"
#include "dsp/Vca.hpp"
#include "dsp/Vca4.hpp"
#include "dsp/Via.hpp"
#include "dsp/Vocoder.hpp"
#include "dsp/Volta.hpp"
#include "dsp/Wasp.hpp"
#include "dsp/Wavetable.hpp"
#define RASGO_MODELOS(X) \
    X(Oscillator, "OSC") \
    X(Wavetable, "WAVETABLE") \
    X(Additive, "ADDITIVE") \
    X(Operator, "OPERATOR") \
    X(Pulsar, "PULSAR") \
    X(Spectra, "SPECTRA") \
    X(Pll, "PLL") \
    X(Chord, "CHORD") \
    X(Noise, "NOISE") \
    X(Matter, "MATTER") \
    X(StringVoice, "STRING") \
    X(Drum, "DRUM") \
    X(Gendyn, "GENDYN") \
    X(Rungler, "RUNGLER") \
    X(Filter, "FILTER") \
    X(Ladder, "LADDER") \
    X(Formant, "FORMANT") \
    X(Vocoder, "VOCODER") \
    X(Resonator, "RESONATOR") \
    X(Wasp, "WASP") \
    X(Lpg, "LPG") \
    X(Vca, "VCA") \
    X(Vca4, "VCA4") \
    X(Shape, "SHAPE") \
    X(Shifter, "SHIFTER") \
    X(Crush, "CRUSH") \
    X(Parametric, "PARAMETRIC") \
    X(Glide, "GLIDE") \
    X(Control, "CONTROL") \
    X(Dynamics, "DYNAMICS") \
    X(Elastic, "ELASTIC") \
    X(Tape, "TAPE") \
    X(Envelope, "ENVELOPE") \
    X(FunctionGenerator, "FUNCTION") \
    X(Stages, "STAGES") \
    X(Drift, "DRIFT") \
    X(Chaos, "CHAOS") \
    X(SampleHold, "SH") \
    X(Bounce, "BOUNCE") \
    X(Mousse, "MOUSSE") \
    X(Pendulum, "PENDULUM") \
    X(Epicycle, "EPICYCLE") \
    X(Supply, "SUPPLY") \
    X(Magnet, "MAGNET") \
    X(Orbit, "ORBIT") \
    X(Pressure, "PRESSURE") \
    X(Refract, "REFRACT") \
    X(Predator, "PREDATOR") \
    X(Form, "FORM") \
    X(EuclidClock, "CLOCK") \
    X(Logic, "LOGIC") \
    X(TuringLoop, "TURING") \
    X(StepSequencer, "SEQUENCE") \
    X(TrigSeq, "TRIGSEQ") \
    X(Arp, "ARP") \
    X(Cell, "CELL") \
    X(Tap, "TAP") \
    X(Polygon, "POLYGON") \
    X(Budget, "BUDGET") \
    X(Interference, "INTERFERENCE") \
    X(Swarm, "SWARM") \
    X(Eclipse, "ECLIPSE") \
    X(Season, "SEASON") \
    X(Tempo, "TEMPO") \
    X(Volta, "VOLTA") \
    X(Quantizer, "QUANTIZER") \
    X(Harmony, "HARMONY") \
    X(Abacus, "ABACUS") \
    X(Decision, "DECISION") \
    X(Boxcar, "BOXCAR") \
    X(Markov, "MARKOV") \
    X(Coherence, "COHERENCE") \
    X(Key, "KEY") \
    X(Accent, "ACCENT") \
    X(Blefe, "BLEFE") \
    X(Fronteira, "FRONTEIRA") \
    X(Comparador, "VERSUS") \
    X(Interpret, "INTERPRET") \
    X(Phrase, "PHRASE") \
    X(Switch, "SWITCH") \
    X(Matrix, "MATRIX") \
    X(Mult, "MULT") \
    X(Planar, "PLANAR") \
    X(Via, "VIA") \
    X(Relate, "RELATE") \
    X(Summer, "SUM") \
    X(Space, "SPACE") \
    X(Hall, "HALL") \
    X(Looper, "LOOPER") \
    X(Swirl, "SWIRL") \
    X(Memory, "MEMORY") \
    X(Sampler, "SAMPLER") \
    X(Turntable, "TURNTABLE") \
    X(Spring, "SPRING") \
    X(Prism, "PRISM") \
    X(Apocrifo, "APOCRIFO") \
    X(CutUp, "CUTUP") \
    X(Lens, "LENS") \
    X(Scope, "SCOPE") \
    X(Listen, "LISTEN") \

