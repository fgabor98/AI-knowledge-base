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

ASoC separates CPU DAIs, codecs, and the machine-level description that connects them. TI's AM62x SK uses `simple-audio-card`, which links DAIs with `sound-dai` phandles rather than graph endpoints. From `k3-am62x-sk-common.dtsi` (trimmed):

```dts
tlv320_mclk: clk-0 {
        #clock-cells = <0>;
        compatible = "fixed-clock";
        clock-frequency = <12288000>;
};

codec_audio: sound {
        compatible = "simple-audio-card";
        simple-audio-card,name = "AM62x-SKEVM";
        simple-audio-card,widgets =
                "Headphone",    "Headphone Jack",
                "Line",         "Line In",
                "Microphone",   "Microphone Jack";
        simple-audio-card,routing =
                "Headphone Jack",       "HPLOUT",
                "Headphone Jack",       "HPROUT",
                "LINE1L",               "Line In",
                "LINE1R",               "Line In",
                "MIC3R",                "Microphone Jack",
                "Microphone Jack",      "Mic Bias";
        simple-audio-card,format = "dsp_b";
        simple-audio-card,bitclock-master = <&sound_master>;
        simple-audio-card,frame-master = <&sound_master>;
        simple-audio-card,bitclock-inversion;

        simple-audio-card,cpu {
                sound-dai = <&mcasp1>;
        };

        sound_master: simple-audio-card,codec {
                sound-dai = <&tlv320aic3106>;
                clocks = <&tlv320_mclk>;
        };
};

&mcasp1 {
        #sound-dai-cells = <0>;
        op-mode = <0>;          /* MCASP_IIS_MODE */
        tdm-slots = <2>;

        serial-dir = <  /* 0: INACTIVE, 1: TX, 2: RX */
               1 0 2 0
               0 0 0 0
               0 0 0 0
               0 0 0 0
        >;
};
```

The codec, `tlv320aic3106: audio-codec@1b`, is an I²C child of `main_i2c1` with `#sound-dai-cells = <0>` and three supplies.

The graph-based alternative is `audio-graph-card`. Renesas's `ulcb-audio-graph-card.dtsi` lists DAI ports in `dais`, and each endpoint pairs a CPU port with a codec or HDMI port:

```dts
sound_card: sound {
        compatible = "audio-graph-card";
        label = "snd-ulcb";
        dais = <&snd_ulcb1 /* (A) CPU0 <-> ak4613 */
                &snd_ulcb2 /* (B) CPU1  -> HDMI   */
        >;
};

snd_ulcb1: port@0 {
        reg = <0>;
        rsnd_for_ak4613: endpoint {
                remote-endpoint = <&ak4613_endpoint>;
                bitclock-master;
                frame-master;
                playback = <&ssi0>, <&src0>, <&dvc0>;
                capture  = <&ssi1>, <&src1>, <&dvc1>;
        };
};
```

Both forms describe the same contract. The graph form helps when one device has several audio ports, such as an HDMI transmitter's `port@2` sound input; the SiI9022 binding likewise reserves `port@3` for sound input. Use the schemas and examples for the target kernel rather than translating mechanically between them.

## DAI Link Contract

For each CPU-to-codec link, prove the following. The AM62x SK values are in parentheses:

- serial format (`dsp_b`)
- bit-clock and frame-clock mastership (both driven by the codec, through `sound_master`)
- clock inversion and polarity (`bitclock-inversion`)
- TDM slots, masks, width, and channel mapping (two slots; McASP serializer 0 transmits and serializer 2 receives)
- system/master clock source and allowed rates (a fixed 12.288 MHz oscillator to the codec, a 48 kHz family rate)
- sample formats and rates supported by both endpoints

Mastership is directional. Exactly one capable component should drive each clock. Two masters cause contention; zero masters leaves a static bus.

## Graph Edges Versus DAPM Routes

The endpoint link connects component DAIs. DAPM describes audio signal paths and power dependencies within components and across board widgets such as microphones, headphone jacks, and amplifiers. A graph endpoint does not mean every mixer route is active.

Routing properties list sink/source name pairs whose strings must match codec widgets and board widgets exactly. In the SK card, `HPLOUT`, `HPROUT`, `LINE1L`, `LINE1R`, `MIC3R`, and `Mic Bias` are widget names defined in `sound/soc/codecs/tlv320aic3x.c`; `Headphone Jack`, `Line In`, and `Microphone Jack` are board widgets declared in `simple-audio-card,widgets`. A card can register successfully while a misspelled route produces silence.

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
