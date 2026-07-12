/*
 * Modbus RTU master on Arduino using libmodbus built transport-only.
 *
 * libmodbus is compiled with -DMODBUS_TRANSPORT_ONLY, so it contains no native
 * serial backend (no termios). All I/O is provided by the small UART transport
 * below, which drives an Arduino HardwareSerial. This is the natural way to run
 * Modbus RTU on boards whose serial API is not POSIX termios (ESP32, ESP8266,
 * SAMD, Teensy, ...).
 *
 * Wiring: RS485_SERIAL TX/RX -> an RS485 transceiver -> the RTU bus. If the
 * transceiver needs an explicit driver-enable line, set RS485_DE_PIN.
 *
 * See README.md for how to add libmodbus to the project.
 */

#include <Arduino.h>
#include <errno.h>
#include <modbus.h> /* declares C API; already wrapped in extern "C" internally */

/* ------------------------------------------------------------------ config */
#define RS485_SERIAL Serial1 /* UART wired to the RS485 transceiver */
#define RS485_BAUD   9600
#define RS485_DE_PIN -1 /* driver-enable pin, or -1 for auto-direction modules */
#define SLAVE_ID     17
#define REG_ADDRESS  0
#define REG_COUNT    4

/* --------------------------------------------------------------- transport */
struct uart_priv {
    HardwareSerial *port;
    int de_pin;
};

/* The callbacks are given C linkage so they match the modbus_transport_t
 * function-pointer types exactly. */
extern "C" {

static int uart_connect(modbus_transport_t *t)
{
    (void) t; /* the port is opened in setup() */
    return 0;
}

static ssize_t uart_send(modbus_transport_t *t, const uint8_t *buf, int len)
{
    uart_priv *p = (uart_priv *) t->priv;
    if (p->de_pin >= 0)
        digitalWrite(p->de_pin, HIGH); /* drive the bus */
    size_t n = p->port->write(buf, len);
    p->port->flush(); /* block until the frame has left the UART */
    if (p->de_pin >= 0)
        digitalWrite(p->de_pin, LOW); /* release the bus to receive */
    return (ssize_t) n;
}

static ssize_t uart_recv(modbus_transport_t *t, uint8_t *buf, int len)
{
    uart_priv *p = (uart_priv *) t->priv;
    int total = 0;
    /* libmodbus calls select() first, so bytes are on their way; read exactly
     * `len`, tolerating small inter-byte gaps. */
    unsigned long deadline = millis() + 50;
    while (total < len) {
        if (p->port->available()) {
            buf[total++] = (uint8_t) p->port->read();
            deadline = millis() + 50;
        } else if ((long) (millis() - deadline) >= 0) {
            break; /* gap in the frame */
        }
    }
    return total;
}

static int uart_select(modbus_transport_t *t, struct timeval *tv)
{
    uart_priv *p = (uart_priv *) t->priv;
    unsigned long timeout_ms =
        tv ? (tv->tv_sec * 1000UL + tv->tv_usec / 1000UL) : 3600000UL;
    unsigned long deadline = millis() + timeout_ms;
    while (!p->port->available()) {
        if ((long) (millis() - deadline) >= 0)
            return 0; /* timeout: no data */
        yield();      /* keep RTOS/watchdog happy on ESP32 etc. */
    }
    return 1; /* data ready */
}

static void uart_close(modbus_transport_t *t)
{
    (void) t;
}

} /* extern "C" */

static uart_priv priv = {&RS485_SERIAL, RS485_DE_PIN};
static modbus_transport_t transport;
static modbus_t *ctx;

/* ------------------------------------------------------------------- setup */
void setup()
{
    Serial.begin(115200);
    RS485_SERIAL.begin(RS485_BAUD);
    if (RS485_DE_PIN >= 0) {
        pinMode(RS485_DE_PIN, OUTPUT);
        digitalWrite(RS485_DE_PIN, LOW);
    }

    /* Fill the transport (send/recv/select/connect required; flush/close/free
     * optional). */
    transport.connect = uart_connect;
    transport.send = uart_send;
    transport.recv = uart_recv;
    transport.select = uart_select;
    transport.flush = NULL;
    transport.close = uart_close;
    transport.free = NULL;
    transport.priv = &priv;

    ctx = modbus_new_rtu_transport();
    modbus_set_slave(ctx, SLAVE_ID);
    modbus_set_transport(ctx, &transport);
    modbus_connect(ctx);
    modbus_set_response_timeout(ctx, 1, 0);

    Serial.println("libmodbus transport-only RTU master ready");
}

/* -------------------------------------------------------------------- loop */
void loop()
{
    uint16_t regs[REG_COUNT];
    int rc = modbus_read_registers(ctx, REG_ADDRESS, REG_COUNT, regs);
    if (rc == REG_COUNT) {
        Serial.print("registers:");
        for (int i = 0; i < REG_COUNT; i++) {
            Serial.print(' ');
            Serial.print(regs[i]);
        }
        Serial.println();
    } else {
        Serial.print("read failed: ");
        Serial.println(modbus_strerror(errno));
    }
    delay(1000);
}
