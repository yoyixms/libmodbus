# modbus_new_tcp_transport

## Name

modbus_new_tcp_transport - create a transport-only Modbus TCP context

## Synopsis

```c
modbus_t *modbus_new_tcp_transport(void);
```

## Description

The *modbus_new_tcp_transport()* function shall allocate and initialize a Modbus
TCP context that carries only the MBAP framing. No socket is opened: all I/O must
be provided by a pluggable transport registered with
[modbus_set_transport](modbus_set_transport.md) before
[modbus_connect](modbus_connect.md).

This is useful to run Modbus TCP over a custom or userspace network stack, and it
is the entry point for a library built with `--enable-transport-only`, which
compiles without any native socket I/O.

Without a registered transport, *modbus_connect()* and the read/write functions
fail with errno set to ENOTSUP.

## Return value

The function shall return a pointer to a *modbus_t* context if successful.
Otherwise it shall return NULL and set errno.

## Example

```c
modbus_t *ctx = modbus_new_tcp_transport();
modbus_set_transport(ctx, &my_transport);
modbus_connect(ctx);
```

## See also

- [modbus_set_transport](modbus_set_transport.md)
- [modbus_new_tcp](modbus_new_tcp.md)
- [modbus_connect](modbus_connect.md)
