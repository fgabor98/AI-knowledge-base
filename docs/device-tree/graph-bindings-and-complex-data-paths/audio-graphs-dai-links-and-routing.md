---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Audio Graphs, DAI Links, And Routing

Embedded audio contains several overlapping graphs: physical digital audio interfaces between components, ASoC DAI links that form a sound card, and DAPM routes inside and around codecs. They solve different problems.

## Components And Machine Topology

ASoC separates CPU/platform DAIs, codecs, and the machine-level description that connects them. An audio graph card can express DAI relationships through graph ports when the participating bindings support it.

```dts
sound {
        compatible = "audio-graph-card";
        label = "Board Audio";
        dais = <&cpu_dai_port>;
};

&i2s0 {
        cpu_dai_port: port {
                cpu_endpoint: endpoint {
                        remote-endpoint = <&codec_endpoint>;
                        dai-format = "i2s";
                        bitclock-master;
                        frame-master;
                };
        };
};

codec@1a {
        compatible = "vendor,audio-codec";
        reg = <0x1a>;

        port {
                codec_endpoint: endpoint {
                        remote-endpoint = <&cpu_endpoint>;
                };
        };
};
```

This is illustrative: exact master/format property placement has evolved across simple-card and audio-graph bindings. Use the schemas and examples for the target kernel rather than translating mechanically.

## DAI Link Contract

For each CPU-to-codec link, prove:

- serial format: I²S, left/right justified, DSP A/B, AC97, or binding-defined mode
- bit-clock and frame-clock mastership
- clock inversion and polarity
- time-division multiplexing slots, masks, width, and channel mapping
- system/master clock source and allowed rates
- sample formats and rates supported by both endpoints

Mastership is directional. Exactly one capable component should drive each clock. Two masters cause contention; zero masters leaves a static bus.

## Graph Edges Versus DAPM Routes

The endpoint link connects component DAIs. DAPM describes audio signal paths and power dependencies within components and across board widgets such as microphones, headphone jacks, and amplifiers. A graph endpoint does not mean every mixer route is active.

Properties such as `audio-routing` commonly list sink/source name pairs whose strings must match codec widgets and board widgets exactly. A card can register successfully while a misspelled DAPM route produces silence.

## Multi-Link And DPCM Systems

Complex SoCs can have front-end PCM links, DSP processing, and back-end physical DAIs. DPCM models dynamic routes between them. A simple one-edge hardware graph cannot capture all runtime DSP routing policy.

Use the binding intended for the platform architecture. Do not force a graph-card binding onto hardware that needs a machine driver for jack detection, clock arbitration, amplifiers, or nontrivial DSP topology.

## Clock And Power Sequencing

Audio clocks can be shared across links and families (44.1 kHz versus 48 kHz multiples). Review PLL parent/rate conflicts, codec MCLK requirements, and whether concurrent streams can coexist.

DAPM powers only widgets on active routes, while runtime PM controls component devices. External amplifiers may need supply, mute, enable, and pop-suppression sequencing. DT describes their wiring and constraints; the ASoC machine/component logic performs timed transitions.

## Runtime Diagnosis

```sh
aplay -l
arecord -l
cat /proc/asound/cards
cat /sys/kernel/debug/asoc/cards 2>/dev/null
find /sys/kernel/debug/asoc -maxdepth 3 -type f 2>/dev/null
dmesg | grep -Ei 'asoc|audio|codec|dai|sound'
```

Use `amixer`, `alsactl`, and controlled playback/capture. Observe BCLK, frame clock, MCLK, and data with a scope or logic analyzer. Separate card registration, PCM constraints, DAPM routing, clocking, amplifier state, and analog path.

## Authoritative References

- [Linux ASoC overview](https://docs.kernel.org/sound/soc/overview.html)
- [Linux ASoC machine-driver documentation](https://docs.kernel.org/sound/soc/machine.html)
- [Linux Dynamic PCM documentation](https://docs.kernel.org/sound/soc/dpcm.html)
- [Linux audio graph card schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/sound/audio-graph-card.yaml)

## Continue

Proceed to [Multi-Endpoint Topologies, Crossbars, And Shared Resources](multi-endpoint-topologies-crossbars-and-shared-resources.md).
