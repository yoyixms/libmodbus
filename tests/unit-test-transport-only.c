/*
 * Copyright © Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Regression test for the transport-only constructors
 * (modbus_new_tcp_transport / modbus_new_rtu_transport). They build a context
 * that carries only the Modbus framing and require a pluggable transport for
 * all I/O. The constructor sanity checks run everywhere; the round-trip drives
 * a transport-only TCP client against a normal server over a socketpair, so it
 * runs on POSIX only. */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <modbus.h>

#ifndef _WIN32
/* Minimal socketpair transport: the fd is the only private state. */
static int t_connect(modbus_transport_t *t)
{
    (void) t;
    return 0;
}

static ssize_t t_send(modbus_transport_t *t, const uint8_t *buf, int len)
{
    return send(*(int *) t->priv, (const char *) buf, len, 0);
}

static ssize_t t_recv(modbus_transport_t *t, uint8_t *buf, int len)
{
    int total = 0;
    while (total < len) {
        int rc = recv(*(int *) t->priv, (char *) buf + total, len - total, 0);
        if (rc == 0)
            return total;
        if (rc < 0)
            return -1;
        total += rc;
    }
    return total;
}

static int t_select(modbus_transport_t *t, struct timeval *tv)
{
    int fd = *(int *) t->priv;
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    return select(fd + 1, &rset, NULL, NULL, tv);
}

static void t_close(modbus_transport_t *t)
{
    close(*(int *) t->priv);
}

static void test_tcp_transport_roundtrip(void)
{
    printf("[1] tcp_transport round-trip... ");
    fflush(stdout);

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    modbus_mapping_t *map = modbus_mapping_new(0, 0, 10, 0);
    assert(map);
    for (int i = 0; i < 10; i++)
        map->tab_registers[i] = (uint16_t) (300 + i);

    pid_t pid = fork();
    if (pid == 0) {
        /* Server: a normal TCP backend context over fds[1]. */
        close(fds[0]);
        modbus_t *srv = modbus_new_tcp("127.0.0.1", 1502);
        modbus_set_socket(srv, fds[1]);
        modbus_set_debug(srv, FALSE);
        uint8_t req[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc = modbus_receive(srv, req);
        if (rc > 0)
            modbus_reply(srv, req, rc, map);
        modbus_free(srv);
        modbus_mapping_free(map);
        _exit(0);
    }
    close(fds[1]);

    /* Client: a transport-only context, no socket of its own. */
    int cfd = fds[0];
    modbus_transport_t tr = {
        .connect = t_connect,
        .send = t_send,
        .recv = t_recv,
        .select = t_select,
        .close = t_close,
        .priv = &cfd,
    };

    modbus_t *cli = modbus_new_tcp_transport();
    assert(cli);
    assert(modbus_get_transport(cli) == NULL);
    assert(modbus_set_transport(cli, &tr) == 0);
    assert(modbus_connect(cli) == 0);
    assert(tr.connected == 1);
    modbus_set_slave(cli, MODBUS_TCP_SLAVE);
    modbus_set_response_timeout(cli, 2, 0);
    modbus_set_debug(cli, FALSE);

    uint16_t out[10];
    assert(modbus_read_registers(cli, 0, 10, out) == 10);
    for (int i = 0; i < 10; i++)
        assert(out[i] == (uint16_t) (300 + i));

    modbus_close(cli);
    modbus_free(cli);

    int status;
    waitpid(pid, &status, 0);
    modbus_mapping_free(map);
    printf("PASS\n");
}
#endif

static void test_constructors(void)
{
    printf("[2] transport-only constructors... ");
    fflush(stdout);

    modbus_t *tcp = modbus_new_tcp_transport();
    assert(tcp);
    assert(modbus_get_transport(tcp) == NULL);
    assert(modbus_get_header_length(tcp) == 7); /* MBAP header */
    modbus_free(tcp);

    modbus_t *rtu = modbus_new_rtu_transport();
    assert(rtu);
    assert(modbus_get_transport(rtu) == NULL);
    assert(modbus_get_header_length(rtu) == 1); /* slave address only */
    modbus_free(rtu);

    printf("PASS\n");
}

int main(void)
{
    printf("=== modbus transport-only constructor tests ===\n");

#ifndef _WIN32
    test_tcp_transport_roundtrip();
#else
    printf("[1] tcp_transport round-trip... SKIP (no fork on Windows)\n");
#endif
    test_constructors();

    printf("All tests passed.\n");
    return EXIT_SUCCESS;
}
