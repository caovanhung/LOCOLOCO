# Temperature usermod

Based on the excellent `QuinLED_Dig_Uno_Temp_MQTT` usermod by srg74 and 400killer!  
Reads an attached DS18B20 temperature sensor (as available on the QuinLED Dig-Uno)  
Temperature is displayed in both the Info section of the web UI as well as published to the `/temperature` MQTT topic, if enabled.  
May be expanded with support for different sensor types in the future.

If temperature sensor is not detected during boot, this usermod will be disabled.

Maintained by @blazoncek

## Installation

Add `Temperature` to `custom_usermods` in your platformio_override.ini.

Example **platformio_override.ini**:

```ini
[env:usermod_temperature_esp32dev]
extends = env:esp32dev
custom_usermods = ${env:esp32dev.custom_usermods} 
  Temperature
```

### Define Your Options

* `USERMOD_DALLASTEMPERATURE_MEASUREMENT_INTERVAL` - number of milliseconds between measurements, defaults to 60000 ms (60s)

All parameters can be configured at runtime via the Usermods settings page, including pin, temperature in degrees Celsius or Fahrenheit and measurement interval.

## Project link

* [QuinLED-Dig-Uno](https://quinled.info/2018/09/15/quinled-dig-uno/) - Project link
* [Srg74-WLED-Wemos-shield](https://github.com/srg74/WLED-wemos-shield) - another great DIY WLED board

## Change Log

2020-09-12 
* Changed to use async non-blocking implementation
* Do not report erroneous low temperatures to MQTT
* Disable plugin if temperature sensor not detected
* Report the number of seconds until the first read in the info screen instead of sensor error

2021-04
* Adaptation for runtime configuration.

2023-05
* Rewrite to conform to newer recommendations.
* Recommended @blazoncek fork of OneWire for ESP32 to avoid Sensor error

2024-09
* Update OneWire to version 2.3.8, which includes stickbreaker's and garyd9's ESP32 fixes:
  blazoncek's fork is no longer needed