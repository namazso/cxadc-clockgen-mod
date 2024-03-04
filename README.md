# CXADC ClockGen Modified

This is a modification of the original CXADC ClockGen at https://gitlab.com/wolfre/cxadc-clock-generator-audio-adc

## Why?

- Cost reduction: The main board PCB and the Si5351A is not necessary for this mod.
- Alternative use cases: Nearly any combination of the [Domesday Duplicator](https://github.com/simoninns/DomesdayDuplicator), [CXADC](https://github.com/happycube/cxadc-linux3) cards, and this build works.
- Simplicity: Some niche use cases were cut.

## How?

There are several changes that make this possible:

- The PCM1802 is configured in 512 division mode. This allows us to directly use a 40 MHz clock with it, while producing 78125 Hz sample rate recordings.
- USB configuration was edited to allow enough buffer for the higher sample rate recording, as well as remove alternative clocks.
- The Pi Pico is underclocked to 120 MHz. This allows a simple divisor for getting 40 MHz, without fractionals which introduce jitter.
- The RP2040 has a feature where you can output divided clocks on a certain set of GPIO pins. Using this, we output 40 MHz on GPIO 21.

## Shared build guide for any setup

### Materials needed

- PCM1802 board (€4) [AliExpress](https://www.aliexpress.com/w/wholesale-pcm1802.html)
- Raspberry Pi Pico (€2) [AliExpress](https://www.aliexpress.com/w/wholesale-rp2040-pico.html)
- Wires
- Soldering setup (iron, solder, flux)

### Steps

1. **Enable 512 fs master mode by bridging MODE0**

Unfortunately the most common PCM1802 board [has a bug](https://www.pjrc.com/pcm1802-breakout-board-needs-hack/) where the + side of the bridgable connections is not actually connected to 3V3. To fix this, you will need to add a cable.

![IMG_001231](https://github.com/namazso/cxadc-clockgen-mod/assets/8676443/ecd69d1a-ecab-4ba3-b07a-5015ee40c5d5)
![IMG_001232](https://github.com/namazso/cxadc-clockgen-mod/assets/8676443/46dab739-5289-42eb-94c7-754f9fd214c5)

2. **Connect the PCM1802 board to the Pi Pico**

- 5V to VBUS
- GND to GND (any)
- PDW to GPIO17
- DOUT to GPIO10
- BCK to GPIO11
- LRCK to GPIO12

3. **Flash the firmware on the Pi Pico**

Build and flash the contents of the firmware folder. Alternatively, you can use the [prebuilt version](https://github.com/namazso/cxadc-clockgen-mod/releases/latest/download/firmware.uf2) to skip the building step.

## Use as a cheaper CXADC Clock Generator + audio ADC

1. Connect PCM1802's SCK to GPIO21
2. Build the necessary amount of clock generator adapters [as in the original](https://gitlab.com/wolfre/cxadc-clock-generator-audio-adc/-/tree/main/build-guide?ref_type=heads#building-pcb-1-vt610ex-clock-generator-insert)
3. Connect their GND to GND and their clock input to GPIO21

How you make the connection is up to you, but cutting an SMA cable may be a very easy solution if using the SMA version of the adapter boards.

## Use as an externally clocked audio ADC with a Domesday Duplicator

1. Connect GNDs, connect PCM1802's SCK to the Domesday Duplicator's pin 40

## ~~Use as an externally clocked audio ADC with a [MISRC](https://github.com/Stefan-Olt/MISRC/)~~

~~1. Connect the SMA clock output of the MISRC to SCK and GND of the PCM1802~~

Use the AUX pins and [pcm_extract](https://github.com/namazso/pcm_extract/) instead.
