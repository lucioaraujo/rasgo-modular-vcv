# Rasgo Modular for VCV Rack

The modules of [Rasgo Modular](https://lucioaraujo.github.io/rasgo-modular/) —
a modular synthesizer by Lúcio de Araújo, part of the
[RASGO](https://rasgoinstruments.arquiviagem.net/) family — as **VCV Rack 2**
modules: **100 modules** in eight families (SOURCE, TRANSFORM, MODULATE,
TIME, DECISION, ROUTE, SPACE, OUT), from oscillators and filters to physics,
optics, ecology and Xenakis.

Each module runs the same DSP as the Rasgo Modular application. Rasgo
Modular itself is a complete instrument — seeds, the VARIA hand, cables as
processes — and lives in its own application; here its modules join any VCV
Rack patch.

- **Tooltips** on every knob and jack explain what it does.
- **Right-click → User manual** opens the module's guide on the website.
- **Right-click → credits**: author, sources and the dossier of each module.
- **Polyphony** up to 16 voices (the SPACE family is mono).
- Voltages follow VCV Rack: audio and CV ±5 V, gates 0–10 V, 1 V/oct with
  0 V = C4.

Five modules are the application's own connectors and are not included,
because VCV Rack has its own: SIGNAL-IN, POLY-IN, NOTE-OUT, MIXER, MASTER.

## Build

With the free VCV Rack SDK 2:

```sh
make RACK_DIR=/path/to/Rack-SDK SLUG=RasgoModular VERSION=2.0.0
make RACK_DIR=/path/to/Rack-SDK SLUG=RasgoModular VERSION=2.0.0 install
```

(`SLUG` and `VERSION` are only needed when `jq` is not installed.)

## License

GPL-3.0-or-later (`LICENSE.txt`). The DSP comes from Rasgo Modular, whose
`src/dsp/` and `src/core/` are offered under AGPL-3.0-or-later **or**
GPL-3.0-or-later; the English LEARN texts are offered under the same terms
(see Rasgo Modular's `LICENSING.md`). The knobs, jacks, sliders and screws
are VCV Rack's Component Library, provided by Rack itself.

---

# Rasgo Modular para o VCV Rack

Os módulos do [Rasgo Modular](https://lucioaraujo.github.io/rasgo-modular/),
sintetizador modular de Lúcio de Araújo, da família
[RASGO](https://rasgoinstruments.arquiviagem.net/), como módulos do **VCV
Rack 2**: **100 módulos** em oito famílias. Cada um roda o mesmo DSP do
aplicativo Rasgo Modular; o instrumento completo (seeds, a mão do VARIA, os
cabos como processo) continua no aplicativo. Dicas em cada knob e jack,
manual e créditos no botão direito, polifonia até 16 vozes. Licença
GPL-3.0-or-later.

Contact: rasgo.instruments@gmail.com
