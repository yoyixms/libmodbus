# modbus_new_rtu_transport

## Name

modbus_new_rtu_transport - create a transport-only Modbus RTU context

## Synopsis

```c
modbus_t *modbus_new_rtu_transport(void);
```

## Description

The *modbus_new_rtu_transport()* function shall allocate and initialize a Modbus
RTU context that carries only the RTU framing (unit address and CRC). No serial
port is opened: all I/O must be provided by a pluggable transport registered with
[modbus_set_transport](modbus_set_transport.md) before
[modbus_connect](modbus_connect.md). The slave must be set with
[modbus_set_slave](modbus_set_slave.md) before use.

This is useful to run Modbus RTU over a custom serial driver or a non-serial
link, and it is the entry point for a library built with
`--enable-transport-only`, which compiles without any native serial I/O.

Without a registered transport, *modbus_connect()* and the read/write functions
fail with errno set to ENOTSUP.

## Return value

The function shall return a pointer to a *modbus_t* context if successful.
Otherwise it shall return NULL and set errno.

## Example

```c
modbus_t *ctx = modbus_new_rtu_transport();
modbus_set_slave(ctx, 17);
modbus_set_transport(ctx, &my_transport);
modbus_connect(ctx);
```

## See also

- [modbus_set_transport](modbus_set_transport.md)
- [modbus_new_rtu](modbus_new_rtu.md)
- [modbus_set_slave](modbus_set_slave.md)
- [modbus_connect](modbus_connect.md)
