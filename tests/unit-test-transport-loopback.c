/*
 * Copyright © Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Test for the transport-only build (configure --enable-transport-only). The
 * library carries only the Modbus framing and performs no socket/serial I/O, so
 * this test wires two transport-only contexts (client + server) together through
 * a socketpair provided by its own transport callbacks, and exercises a full
 * read-registers round-trip over both the TCP (MBAP) and RTU (address + CRC)
 * framings. The socketpair lives only in the test; the library stays I/O-free.
 *
 * POSIX only (uses socketpair()/fork()); the transport-only build targets
 * non-Windows embedded/RTOS environments. */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <modbus.h>

#ifndef _WIN32
static int t_connect(modbus_transport_t *t)
{
    (void) t;
    return 0;
}
static ssize_t t_send(modbus_transport_t *t, const uint8_t *b, int n)
{
    return send(*(int *) t->priv, (const char *) b, n, 0);
}
static ssize_t t_recv(modbus_transport_t *t, uint8_t *b, int n)
{
    int total = 0, rc;
    while (total < n) {
        rc = recv(*(int *) t->priv, (char *) b + total, n - total, 0);
        if (rc <= 0)
            return rc == 0 ? total : -1;
        total += rc;
    }
    return total;
}
static int t_select(modbus_transport_t *t, struct timeval *tv)
{
    int fd = *(int *) t->priv;
    fd_set r;
    FD_ZERO(&r);
    FD_SET(fd, &r);
    return select(fd + 1, &r, NULL, NULL, tv);
}
static void t_close(modbus_transport_t *t)
{
    close(*(int *) t->priv);
}
static modbus_transport_t make_tr(int *fd)
{
    modbus_transport_t tr = {.connect = t_connect,
                             .send = t_send,
                             .recv = t_recv,
                             .select = t_select,
                             .close = t_close,
                             .priv = fd};
    return tr;
}

static void round_trip(const char *label,
                       modbus_t *(*make_ctx)(void),
                       int slave,
                       int base)
{
    printf("%s round-trip... ", label);
    fflush(stdout);

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    pid_t pid = fork();
    if (pid == 0) {
        int sfd = fds[1];
        modbus_transport_t str = make_tr(&sfd);
        modbus_mapping_t *map = modbus_mapping_new(0, 0, 10, 0);
        for (int i = 0; i < 10; i++)
            map->tab_registers[i] = (uint16_t) (base + i);
        modbus_t *srv = make_ctx();
        modbus_set_slave(srv, slave);
        modbus_set_transport(srv, &str);
        modbus_connect(srv);
        uint8_t req[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc = modbus_receive(srv, req);
        if (rc > 0)
            modbus_reply(srv, req, rc, map);
        modbus_close(srv);
        modbus_free(srv);
        modbus_mapping_free(map);
        _exit(0);
    }

    int cfd = fds[0];
    modbus_transport_t ctr = make_tr(&cfd);
    modbus_t *cli = make_ctx();
    modbus_set_slave(cli, slave);
    modbus_set_transport(cli, &ctr);
    modbus_connect(cli);
    modbus_set_response_timeout(cli, 2, 0);

    uint16_t out[10];
    int rc = modbus_read_registers(cli, 0, 10, out);
    assert(rc == 10);
    for (int i = 0; i < 10; i++)
        assert(out[i] == (uint16_t) (base + i));

    modbus_close(cli);
    modbus_free(cli);
    int status;
    waitpid(pid, &status, 0);
    printf("PASS\n");
}
#endif /* !_WIN32 */

int main(void)
{
    printf("=== modbus transport-only build loopback tests ===\n");

#ifndef _WIN32
    round_trip("[1] TCP", modbus_new_tcp_transport, MODBUS_TCP_SLAVE, 900);
    round_trip("[2] RTU", modbus_new_rtu_transport, 17, 400);
#else
    printf("SKIP (no fork on Windows)\n");
#endif

    printf("All tests passed.\n");
    return EXIT_SUCCESS;
}
