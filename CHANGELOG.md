# Rasgo Modular for VCV Rack — changelog

## 2.1.0 — unreleased

- Panels follow the redesign of Rasgo Modular (Nov 26–28): each module has
  its own form tied to its concept — a pair of large knobs, a large central
  "hero" knob, or vertical sliders — with the jacks at the bottom.
- Output area: the outputs sit on a lighter plate, with a green/red light
  beside each output (positive/negative voltage, brightness by level).
- The main control of each module uses a white knob (light body, dark
  pointer), as in the application.
- The accent colour is silver (the orange is gone), including the lit latch
  buttons.
- Displays: phosphor oscilloscope on SCOPE; the cycle drawn as a circle on
  POLYGON, ORBIT, EPICYCLE and SEASON.
- VCA4: new CHAIN input (audio added to the mix) and MXCV input (CV of the
  mix gain).
- Shorter control names where they were longer than five letters (CYCLE,
  DERIV, PASSO, RUBAT, BRILH, FALAN, PROB); tooltips updated.

## 2.0.0 — 2026-11-24 (first public version)

- 100 modules of Rasgo Modular as VCV Rack 2 modules, each one wrapping the
  original DSP (a fixed copy, with checksums in `PROVENANCE.sha256`).
- The Rasgo look: family colour stripe, silver knobs with a reflection in
  three sizes, displays in the family colour, latch buttons that light up,
  the RASGO wordmark on every panel.
- English tooltips on every knob and jack (the LEARN texts of Rasgo
  Modular), a "User manual" link to each module's guide, credits in the
  context menu.
- Polyphony up to 16 voices (one DSP copy per voice); the long-memory SPACE
  family stays mono.
