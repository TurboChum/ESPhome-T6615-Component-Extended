# ESPHome T6615 Extended Component

An extended [ESPHome](https://esphome.io) external component for the **Amphenol Telaire T6615** dual-beam NDIR CO₂ sensor.

The stock ESPHome `t6615` integration reads CO₂ and nothing else. This component exposes the **full UART protocol** — elevation, calibration, diagnostics, self-test, idle mode and device info — while keeping the clean non-blocking design of the original so it stays a drop-in replacement.

---

## What's improved over the stock integration

| | Stock `t6615` | This component |
|---|:---:|:---:|
| CO₂ reading | ✅ | ✅ |
| Elevation read / set | — | ✅ |
| Serial number, firmware version & date | — | ✅ |
| Status flags (error / warmup / calibrating) | — | ✅ |
| Idle mode (lamp off) | — | ✅ |
| Warm reset | — | ✅ |
| Hardware self-test (PGA + DSP) | — | ✅ |
| Single-point calibration (with safety interlock) | — | ✅ |
| ABC logic control (for T6613) | — | ✅ |
| Marks `unavailable` when sensor stops responding | — | ✅ |

All entities are **optional** — configure only the ones you want.

---

## Hardware notes

- **Power:** the T6615 runs on **5 V** and draws very little (~30 mA), so the 5 V line from a USB-to-serial cable powers it comfortably — the Telaire eval kit ships with exactly such a cable for power and comms. When wiring to an ESP board, just feed the sensor 5 V (not 3.3 V).
- **UART:** 19200 baud, 8 data bits, no parity, 1 stop bit (8N1). This is **5 V TTL UART**, not RS-232.
  - **Level shifting:** the ESP32 is a 3.3 V part and the sensor is 5 V, so the UART lines need translating between the two. A simple resistive divider (tried 1 kΩ / 2 kΩ) was **not** reliable in testing — use a proper level shifter. An **ADUM1201** (digital isolator) worked great and saves a lot of headaches.
- **Self-calibration:** the T6615 has a *sealed reference channel* and recalibrates itself internally about every 24 hours. It does **not** use ABC (Automatic Baseline Correction).

### T6615 vs T6613

ABC logic is a feature of the **T6613** (an unreferenced single-beam sensor that drifts and needs baseline correction). The **T6615** is dual-beam with a sealed reference, so ABC has no effect on it. The `abc_logic` switch is included only for T6613 compatibility and a possible future upstream PR — leave it out on a T6615.

---

## Installation

Add the component as an `external_components` source. Because the component lives in a `components/` subfolder, point ESPHome at the repository and let it resolve `t6615`:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/TurboChum/ESPhome-T6615-Component-Extended
      ref: DEV
    refresh: 5min
    components: [t6615]
```

> **Tip:** `refresh: 5min` makes ESPHome pull the latest commit periodically. Once you're happy with a version, you can pin a specific commit with `ref: <sha>` and set `refresh: never`.

---

## Usage

Minimal configuration — just CO₂:

```yaml
uart:
  id: uart_t6615
  rx_pin: GPIO16   # ESP RX  <- sensor TX
  tx_pin: GPIO17   # ESP TX  -> sensor RX
  baud_rate: 19200

t6615:
  id: my_t6615
  uart_id: uart_t6615
  update_interval: 60s

sensor:
  - platform: t6615
    t6615_id: my_t6615
    co2:
      name: "CO2"
```

Full configuration with every entity:

```yaml
sensor:
  - platform: t6615
    t6615_id: my_t6615
    co2:
      name: "CO2"
    elevation_reading:
      name: "Elevation (stored)"

text_sensor:
  - platform: t6615
    t6615_id: my_t6615
    serial_number:
      name: "Serial Number"
    firmware_version:
      name: "Firmware Version"
    firmware_date:
      name: "Firmware Date"
    selftest_result:
      name: "Self-test Result"

binary_sensor:
  - platform: t6615
    t6615_id: my_t6615
    error_flag:
      name: "Error"
    warmup_flag:
      name: "Warming Up"
    calibrating_flag:
      name: "Calibrating"
    selftest_running:
      name: "Self-test Running"

switch:
  - platform: t6615
    t6615_id: my_t6615
    idle_mode:
      name: "Idle Mode"
    calibration_armed:
      name: "Calibration Armed"
    # abc_logic:            # T6613 only — omit on a T6615
    #   name: "ABC Logic"

button:
  - platform: t6615
    t6615_id: my_t6615
    warm_reset:
      name: "Warm Reset"
    self_test:
      name: "Run Self-Test"
    trigger_calibration:
      name: "Trigger Single-Point Calibration"

number:
  - platform: t6615
    t6615_id: my_t6615
    elevation:
      name: "Elevation"
    calibration_ppm_target:
      name: "Calibration PPM Target"
```

A complete working example is in [`example.yaml`](example.yaml).

---

## Entity reference

### Sensors
| Entity | Description |
|---|---|
| `co2` | CO₂ concentration in ppm |
| `elevation_reading` | Elevation (ft) currently stored in the sensor |

### Text sensors *(read once on boot)*
| Entity | Description |
|---|---|
| `serial_number` | Factory serial number |
| `firmware_version` | Firmware build (COMPILE_SUBVOL), e.g. `A15` |
| `firmware_date` | Firmware date (COMPILE_DATE), e.g. `110713` |
| `selftest_result` | Last self-test result, e.g. `PGA:PASS DSP:12/12` |

### Binary sensors *(from the status byte)*
| Entity | Description |
|---|---|
| `error_flag` | Sensor is in an error condition |
| `warmup_flag` | Sensor is warming up |
| `calibrating_flag` | Single-point calibration in progress |
| `selftest_running` | Self-test in progress |

### Switches
| Entity | Description |
|---|---|
| `idle_mode` | Turn the IR lamp off and stop sampling |
| `calibration_armed` | Safety interlock for calibration (see below) |
| `abc_logic` | ABC on/off — **T6613 only**, no effect on T6615 |

### Buttons
| Entity | Description |
|---|---|
| `warm_reset` | Restart the sensor (re-runs warm-up) |
| `self_test` | Run the internal hardware self-test (~32 s) |
| `trigger_calibration` | Start single-point calibration (guarded) |

### Numbers
| Entity | Range | Description |
|---|---|---|
| `elevation` | 0–5000 ft | Elevation used for pressure compensation |
| `calibration_ppm_target` | 400–2000 ppm | Target ppm for single-point calibration |

---

## ⚠️ About single-point calibration

The T6615 self-calibrates against its sealed reference channel, so **most users never need single-point calibration.** Use it only if you have a certified reference gas and the reading has genuinely drifted.

To prevent accidental calibration, the `trigger_calibration` button is gated:

1. Set `calibration_ppm_target` to your reference gas concentration.
2. Turn the **`calibration_armed`** switch ON. It auto-disarms after **5 minutes**.
3. With reference gas flowing, press **`trigger_calibration`**.

Calibration is also blocked while the sensor is warming up or in an error state. Without a reference gas at the target concentration, calibration will skew your readings — handle with care.

---

## License

[GPL-3.0](LICENSE). Based on the upstream ESPHome `t6615` component.
