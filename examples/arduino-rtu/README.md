# Arduino Modbus RTU (transport-only) example

A Modbus RTU **master** on Arduino, using libmodbus built with
`-DMODBUS_TRANSPORT_ONLY`. libmodbus contributes only the Modbus framing (unit
address + CRC); all I/O is done by a small UART transport
([src/main.cpp](src/main.cpp)) driving an Arduino `HardwareSerial`.

This is the natural way to run Modbus RTU on boards whose serial API is **not**
POSIX termios — ESP32, ESP8266, SAMD, Teensy, RP2040, etc. — where the normal
libmodbus serial backend cannot be used.

> This example is provided as a reference; build and flash it on your own board.
> 32-bit boards are recommended; the classic AVR Uno is tight on flash/RAM and
> lacks some of the headers the framing relies on.

## Why a build flag (and not just config.h)

`modbus.c` and `modbus-data.c` do `#include <config.h>`, but `modbus-tcp.c` and
`modbus-rtu.c` receive feature macros from the compiler command line. So
`MODBUS_TRANSPORT_ONLY` must be a **build flag** to reach all four sources — see
`build_flags` in [platformio.ini](platformio.ini). PlatformIO makes this easy;
that's why it is the recommended path.

## Project layout (PlatformIO)

```
arduino-rtu/
  platformio.ini            # sets board + build_flags = -DMODBUS_TRANSPORT_ONLY
  src/
    main.cpp                # the UART transport + demo (provided)
  lib/
    libmodbus/
      src/
        modbus.c  modbus-data.c  modbus-tcp.c  modbus-rtu.c
        modbus.h  modbus-version.h  modbus-transport.h
        modbus-rtu.h  modbus-rtu-private.h
        modbus-tcp.h  modbus-tcp-private.h
        modbus-private.h
        config.h            # from this folder (transport-only)
```

## Setup

1. Create the project and copy `platformio.ini`, `src/main.cpp` and `config.h`
   from this folder.
2. Populate `lib/libmodbus/src/` with the libmodbus sources and headers listed
   above (from the repository `src/` directory), plus this folder's `config.h`.
3. `modbus-version.h` is generated. Get it either by running
   `./autogen.sh && ./configure` once in a checkout (it appears at
   `src/modbus-version.h`) or, on Windows, `cscript src/win32/configure.js`. Copy
   it into `lib/libmodbus/src/`.
4. Set your board in `platformio.ini` (the example targets `esp32dev`) and adjust
   the pins at the top of `main.cpp`:
   - `RS485_SERIAL` — the UART wired to your RS485 transceiver.
   - `RS485_DE_PIN` — the driver-enable pin, or `-1` for auto-direction modules.
   - `SLAVE_ID` / `REG_ADDRESS` / `REG_COUNT` — the slave and registers to read.
5. `pio run -t upload && pio device monitor`.

## Wiring

```
ESP32 RS485_SERIAL TX --> RS485 transceiver DI
ESP32 RS485_SERIAL RX <-- RS485 transceiver RO
ESP32 RS485_DE_PIN    --> transceiver DE (+ /RE, tied together)   [if used]
transceiver A/B       <-> the RTU bus (to your slave device)
GND common
```

To test without a physical slave, connect the transceiver A/B to a USB–RS485
dongle on a PC and run any Modbus RTU **slave** simulator on unit id `SLAVE_ID`
serving a few holding registers.

## How the transport maps to Arduino

| modbus_transport_t | Arduino |
|---|---|
| `send`   | `Serial.write()` + `flush()` (with optional DE toggling) |
| `recv`   | `Serial.read()` until the requested byte count |
| `select` | poll `Serial.available()` until data or timeout |
| `connect`/`close` | no-ops (the port is opened in `setup()`) |
| `flush`/`free` | left NULL (optional) |

If no transport were registered, `modbus_connect()`/read/receive would return
`-1` with `errno == ENOTSUP` — a transport-only build performs no I/O of its own.

## Arduino IDE (instead of PlatformIO)

The classic Arduino IDE has no clean per-project `-D`. Easiest options:
- Use PlatformIO (recommended), or
- On cores that support it, add a `build_opt.h` next to the sketch containing
  `-DMODBUS_TRANSPORT_ONLY`, or
- Add `#define MODBUS_TRANSPORT_ONLY 1` at the very top of *every* libmodbus
  `.c` file (tedious but works).
