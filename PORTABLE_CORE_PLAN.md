# Portable core (run libmodbus on any OS)

Plan for making libmodbus buildable and runnable on a platform that has neither
BSD sockets nor winsock (RTOS, bare metal, custom IP stack), on top of the
pluggable transport (`src/modbus-transport.h`). This branch is based on
`pluggable-transport`.

## Why the transport alone isn't enough

The transport overrides the runtime I/O *channel* (send/recv/select/connect/
close/flush), but the library still has compile-time and residual runtime
dependencies on the host OS even when a transport is set:

1. **The backends compile against OS headers.** `modbus-tcp.c` is a hard
   `_WIN32` (winsock) vs POSIX (`sys/socket.h`, `netinet/*`, `arpa/inet.h`,
   `netdb.h`) split with no third path; `modbus-rtu.c` needs `termios.h` /
   `windows.h`. They are compiled and linked because a context is always created
   with `modbus_new_tcp()` / `modbus_new_rtu()`.
2. **The core receive path used `fd_set` / `struct timeval` unconditionally.**
   `_modbus_receive_msg()` declares `fd_set rset` and (before this branch) called
   `FD_ZERO(&rset)` even for a transport context.
3. **Error-recovery sleep is OS-specific.** `_sleep_response_timeout()` is
   `Sleep()` (Windows) vs `nanosleep()` (POSIX), a two-way `#ifdef` with no third
   option.

## Increments

### Increment 1 — core runtime decoupling (this branch, first commit)

Move `FD_ZERO`/`FD_SET` fully inside the `if (!ctx->transport)` branch of
`_modbus_receive_msg()` so a transport context performs **no** fd_set/select
operations at runtime. Behaviour on the default backend path is unchanged
(verified by the existing TCP/RTU tests); the transport test still passes. This
is the prerequisite for guarding the `fd_set` declaration behind a build option
in Increment 3.

### Increment 2 — transport-only constructors (this branch, second commit)

Add `modbus_new_tcp_transport()` and `modbus_new_rtu_transport()` — per-backend
constructors (consistent with `modbus_new_tcp_pi()`, cleaner than a flavor enum)
that create a context carrying only the framing (TCP MBAP, or RTU address+CRC),
with all I/O required via `modbus_set_transport()`. No socket or serial port is
opened. On a supported OS this is equivalent to `modbus_new_tcp()/rtu()` +
`modbus_set_transport()`, but it states intent (no address/port or device path)
and is the entry point the socket-free build depends on: each lives in its own
backend file, so when a backend's I/O is compiled out in Increment 3 its
transport constructor stays available for framing.

### Increment 3 — make the native backends optional at build time

Split into 3a (core, done) and 3b (backends + build wiring). Driven by the
compile macro `MODBUS_TRANSPORT_ONLY`.

**3a — core (this branch, third commit).** The `fd_set rset` declaration, the
`FD_ZERO`/`FD_SET` block, the backend `select()` fallback, and the OS sleep in
`_sleep_response_timeout()` in `modbus.c` are guarded by
`#ifndef MODBUS_TRANSPORT_ONLY`. With the macro defined, `modbus.c` compiles with
**no** `select`/`fd_set`/`nanosleep`/`Sleep` reference at all (verified:
`gcc -c -DMODBUS_TRANSPORT_ONLY … modbus.c` then `nm` shows none). The default
build (macro undefined) is byte-for-byte unchanged, confirmed by the existing
tests. The backend I/O still reached only through `ctx->backend->…` function
pointers, so the core has no direct socket symbol.

**3b — backends + build wiring (next).** In `modbus-tcp.c` / `modbus-rtu.c`,
`#ifdef`-out the OS socket/serial `#include`s and the I/O function definitions
(send/recv/select/connect/close/flush/listen/accept), keeping the framing
functions and `modbus_new_*_transport()`; point the backend table's I/O slots at
small `ENOTSUP` stubs when `MODBUS_TRANSPORT_ONLY` is set. Add the build option
(`--enable-transport-only` and the CMake equivalent) that defines the macro and
excludes the unusable native constructors. Validate with a fully socket-free
link (both backends' native I/O compiled out) and a round-trip between two
transport-only contexts over a caller-provided channel.

### Increment 4 — polish

Portable `errno`/libc assumptions documented; a bare-metal example transport;
CI job building the socket-free configuration; docs.

## Testing strategy

Every increment must keep the existing suites green (transport, TCP, RTU). The
socket-free configuration is exercised by building with the backends disabled and
running an exchange through a loopback transport (as `unit-test-transport` does),
confirming the library links without `sys/socket.h` / winsock.

## Status

- [x] Increment 1 — core runtime decoupling
- [x] Increment 2 — `modbus_new_tcp_transport()` / `modbus_new_rtu_transport()`
- [x] Increment 3a — `MODBUS_TRANSPORT_ONLY` core guards (socket/select/sleep-free core)
- [x] Increment 3b (TCP) — `modbus-tcp.c` socket I/O guarded out + stubs; a socket-free
      lib (core + data + tcp) links with zero socket symbols and a two-transport-only-context
      round-trip passes (validated with nm + a loopback exchange)
- [x] Increment 3b (RTU) — `modbus-rtu.c` serial I/O + the `struct termios` in `modbus_rtu_t`
      guarded out; the full lib (core + data + tcp + rtu) links with ZERO socket/serial symbols
      and BOTH a TCP and an RTU round-trip pass between transport-only contexts (validated)
- [x] Increment 3b (build) — `configure --enable-transport-only` defines the macro; the test
      suite swaps to a socket-free loopback test (two transport-only contexts over a socketpair,
      TCP + RTU) that runs under `make check`. Validated end to end; the default build is unchanged.
      (CMake option would live in the separate cmake-support work; no CMakeLists in-tree yet.)
- [x] Increment 4 — Windows transport-only guarding (win32_ser helpers, is_connected win32
      branch, tcp init_win32, new_rtu_transport w_ser init) verified compiling under MSVC with
      `_WIN32` + `MODBUS_TRANSPORT_ONLY`; man pages for the transport constructors + index; a CI
      job building `--enable-transport-only` and running `make check`. A reference Arduino RTU
      example (examples/arduino-rtu) implements a UART transport over HardwareSerial for
      modbus_new_rtu_transport(); its libmodbus API usage was compile+link verified as C++
      against the socket-free lib (the Arduino calls need real hardware to run).

The portable-core work is complete.
