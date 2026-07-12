/*
 * Minimal config.h for a transport-only Arduino build of libmodbus.
 *
 * Copy this file into the libmodbus source directory of your project (next to
 * modbus.c), because modbus.c and modbus-data.c do `#include <config.h>`.
 *
 * In a transport-only build every HAVE_* feature macro can stay undefined: the
 * socket/serial code they gate is compiled out, and libmodbus supplies its own
 * strlcpy() when HAVE_STRLCPY is absent. MODBUS_TRANSPORT_ONLY itself is passed
 * as a build flag (see platformio.ini) so it also reaches modbus-tcp.c and
 * modbus-rtu.c; defining it here as well is harmless and documents intent.
 */
#ifndef MODBUS_ARDUINO_CONFIG_H
#define MODBUS_ARDUINO_CONFIG_H

#ifndef MODBUS_TRANSPORT_ONLY
#define MODBUS_TRANSPORT_ONLY 1
#endif

#endif /* MODBUS_ARDUINO_CONFIG_H */
