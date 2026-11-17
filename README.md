Need help? Join the [Discord](https://discord.gg/dUSSRezJXC) or open an issue.

# Comfoair Q 350 ESPHome bridge

This software let you use a ESP32 + CAN Transceiver to interact with the Comfoair Q 350 unit.

It does expose all known informations of your Comfoair unit via ESPHOME and let you control the air flow via Home Assistant as well.

It does allow you to integrate the unit on Home Assistant as depicted below:
![Comfoair Q 350 Home Assistant](docs/homeassistant.png?raw=true "Comfoair Q 350 Home Assistant")

## Custom PCB

Inside the [docs/pcb folder](docs/pcb) you can find both gerber file and BOM. Files are provided as is. It's up to you to check data and schematic correctness.

This repository is meant to be working with the custom pcb above. But you can always buy components separately such as [Waveshare SN65HVD230](https://www.banggood.com/Waveshare-SN65HVD230-CAN-Bus-Module-Communication-CAN-Bus-Transceiver-Development-Board-p-1693712.html?rmmds=myorder&cur_warehouse=CN) and a voltage regulator as well as an ESP32 of your choice and it will work.

The pcb will look like this once soldered and mounted.

![PCB](docs/pic.jpg?raw=true "Comfoair Q 350 3D Print")

## DIY

Prerequisites for custom build:

* `ESP32` -> [link](https://amzn.to/3pe0XVP)
* `DC-DC converter` -> [link](https://amzn.to/39ar22v)
* `RJ45 Female` -> [link](https://amzn.to/3sNx3tH)
* `Can Transceiver` -> [Waveshare SN65HVD230](https://www.banggood.com/Waveshare-SN65HVD230-CAN-Bus-Module-Communication-CAN-Bus-Transceiver-Development-Board-p-1693712.html?rmmds=myorder&cur_warehouse=CN)
+ Some ethernet cable


Here a simple schematic. made by @mat3u

```
---------------+        +---------------+                 +-------------+
(oran/red)12V  o--------o IN+      OUT+ o-----------------o VIN         |
               |        |   [LM2596S]   |                 |             |
(brown)   GND  o--------o IN-      OUT- o-----------------o GND         |
               |        +---------------+                 |             |
    [RJ45]     |                                          |   [ESP32]   |
  [ComfoAir]   |                                          |             |
               |        +-------------------------+       |             |
(w/blue) CAN_L o--------o CAN_L               3v3 o-------o 3v3         |
(blue)   CAN_H o--------o CAN_H               GND o-------o GND         |
---------------+        |                         |       |             |
                        |   [SN65HVD230]   CAN TX o-------o 25          |
                        |                  CAN RX o-------o 21          |
                        +-------------------------+       +-------------+
``````

## esphome conf

To include this repo use the external_components configuration like so:

```yaml
# ...
external_components:
  - source: github://vekexasia/comfoair-esp32
    components: [ comfoair ]
api:
  custom_services: true # <--- This is needed

comfoair:

# ...
```


# Home Assistant Services

The component exposes 4 services.

- comfoair_send_command
- comfoair_send_hex
- comfoair_update_all
- comfoair_req_update_service

**NOTE**: the prefix could differ depending on your esphome device name.

**comfoair_send_command**: allows one "command" parameter. The value of the param can be one of the following pre-bundled commands:

- ventilation_level_0
- ventilation_level_1
- ventilation_level_2
- ventilation_level_3
- boost_10_min
- boost_20_min
- boost_30_min
- boost_60_min
- boost_end
- auto
- manual
- bypass_activate_1h
- bypass_deactivate_1h
- bypass_auto
- ventilation_supply_only
- ventilation_supply_only_reset
- ventilation_extract_only
- ventilation_extract_only_reset
- temp_profile_normal
- temp_profile_cool
- temp_profile_warm

**comfoair_send_hex**: sends a command given as hex bytes, but only if the bytes are exactly those of one of the commands above. Anything else is refused and logged. Some commands can damage the unit, so arbitrary bytes are never sent.
```
service: esphome.comfoair_send_hex
data:
  hexSequence: "8415070100000000100e000000"
```
^^ the above is equivalent to ventilation_extract_only (which has an inner timer of 1h)

**comfoair_update_all**: this service will request an update of all the values from the Comfoair unit. This is useful in case you want to force an update of all the values.

**comfoair_req_update_service**: this service will request an update of a specific value from the Comfoair unit. This is useful in case you want to force an update of a specific value. You're required to pass the `PDOID` of the service to update

## Unit settings

Some of the unit's own settings are read with RMI and shown in Home Assistant. The ones that are safe to change can be changed from there, and appear under Configuration on the device page.

| Entity | Type | Notes |
|---|---|---|
| rmot_heating_threshold, rmot_cooling_threshold | number | Outdoor running mean below which it is the heating season, above which the cooling season |
| comfort_temperature_warm, _normal, _cool | number | Comfort temperature of each temperature profile |
| temperature_profile_mode | select | adaptive or fixed |
| passive_temperature_control | select | off, auto only or on |
| passive_temperature_control_speed | select | slow, medium or fast |
| humidity_comfort_control, humidity_protection | select | off, auto only or on |
| filter_warning_period, filter_lifetime | number | Days |
| airflow_away, _low, _medium, _high | sensor | Set when the unit is commissioned, read only |
| bathroom_switch_on_delay, bathroom_switch_off_delay | sensor | Set when the unit is commissioned, read only |
| hardware_version, firmware_version | text sensor | |

How it works.

- Only the properties listed in `RMI_PROPERTIES` in `rmi.h` are ever read ("get property") or written ("set property"), and only the ones marked writable are written. Other RMI commands can damage the unit and are never sent.
- A value is only written within the range the unit reports for it, and a number's range in Home Assistant is set to the unit's. Anything else is refused and logged, and the control goes back to the unit's value. After a write the value is read back, so Home Assistant shows what the unit really holds.
- Settings are read at boot and then every 10 minutes, so changes made on the unit or in the app show up. At boot the connection to Home Assistant waits until they are read, at most 15 s, so Home Assistant gets the real ranges.
- A setting the unit doesn't know is logged once and not asked again, and one whose range can't be read is shown but not written. The same configuration works on every unit and firmware.

## Status values

As well as the fans, temperatures and energy, the component shows these values the unit sends.

| Entity | Notes |
|---|---|
| alarm | On while the unit has a warning, e.g. the filter warning |
| application_mode | normal, or e.g. filter_wizard while the filters are changed |
| next_profile_change | Seconds left on a temperature profile timer |
| preheater_energy_ytd, preheater_energy_since_start, preheater_power_current | Preheater energy (kWh) and power (W) |
| sensor_ventilation_mode | disabled, active or overruling |
| fan_speed_modulated | 0 to 300, between the levels while sensor ventilation adjusts the speed |
| frost_protection_unbalance | % |
| ventilation_constraints | Why the unit doesn't give the requested airflow, or none |

`ventilation_constraints` lists one or more of resistance, resistance_guard (air resistance limits the fans), noise_guard (fan speed held down to limit noise), preheater_negative, preheater_positive, frost_protection, frost_protection_unbalance, bypass, temperature_comfort, humidity_comfort, humidity_protection, analog_input_1 to 4, analog_preset, rf_sensor_flow_preset, rf_sensor_flow_proportional, hood, comfocool and co2_zone_1 to 8. It shows other if only bits with no known meaning are set.

## Climate Entity

The component will expose a climate entity with heat,cool,auto modes and all the fan speeds.

Furthermore current indoor humidity and temperature as well as current target temperature will be shon as in the picture below.

![climate.png](docs/climate.png)


The climate entity can also be used to set the fan speed, and the mode (heat, cool, auto).

## Global filters

Since this component posts data as soon as it receives it, sometimes it might be useful to avoid spamming home assistant with updates.

To avoid this, you can set the `global_filters`. This accepts the same filters as the `filters` parameter in the `sensor` component.

For example, let's say you want to throttle the sensors for 10s.

```yaml

comfoair:
  global_filters:
    - throttle: 10s
```

Obviously, you can always set the filter on a per-sensor basis.

```yaml
comfoair:
  supply_fan_speed:
    name: supply_fan_speed
    disabled_by_default: false
    force_update: false
    unit_of_measurement: rpm
    accuracy_decimals: 0
    filters:
      - or:
          - throttle: 300s
          - delta: 100.0
```

**NOTE**: when setting `global_filters` that **does** takes precedence over the `filters` set on a per-sensor basis.
## Credits

A lot of this repo was inspired by the reverse engineering [here](https://github.com/marco-hoyer/zcan/issues/1).
If you'd like to know more how the unit communicates, head over

* [here](https://github.com/michaelarnauts/aiocomfoconnect/blob/master/docs/PROTOCOL-RMI.md)
* [and here](https://github.com/michaelarnauts/aiocomfoconnect/blob/master/docs/PROTOCOL-PDO.md)

