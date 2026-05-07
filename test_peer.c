#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "greatest.h"

#ifndef PEERH
#include "peer.h"
#endif

/* ports 19190-19194; far from common services */
#define PORT_BASE 19190

/* give the parent time to reach accept() before the child connects */
static void client_sleep(void) {
    struct timespec ts = { 0, 50000000L }; /* 50 ms */
    nanosleep(&ts, NULL);
}

/* ------------------------------------------------------------------ */
/* Suite: connect (failure cases — no server needed)                   */
/* ------------------------------------------------------------------ */

TEST connect_bad_host(void) {
    Peer *p = peer_connect("host.invalid", PORT_BASE);
    ASSERT_EQ(NULL, p);
    PASS();
}

TEST connect_refused(void) {
    Peer *p = peer_connect("127.0.0.1", PORT_BASE);
    ASSERT_EQ(NULL, p);
    PASS();
}

SUITE(connect_suite) {
    RUN_TEST(connect_bad_host);
    RUN_TEST(connect_refused);
}

/* ------------------------------------------------------------------ */
/* Suite: accept / connect / send / recv (fork for concurrency)        */
/* ------------------------------------------------------------------ */

TEST accept_and_connect(void) {
    pid_t pid = fork();
    if (pid == 0) {
        client_sleep();
        Peer *c = peer_connect("127.0.0.1", PORT_BASE + 1);
        exit(c != NULL ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    Peer *s = peer_accept(PORT_BASE + 1);
    int status;
    waitpid(pid, &status, 0);

    ASSERT_NEQ(NULL, s);
    peer_close(s);
    ASSERT(WIFEXITED(status));
    ASSERT_EQ(EXIT_SUCCESS, WEXITSTATUS(status));
    PASS();
}

TEST send_from_client(void) {
    static const unsigned char payload[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    pid_t pid = fork();
    if (pid == 0) {
        client_sleep();
        Peer *c = peer_connect("127.0.0.1", PORT_BASE + 2);
        if (!c) exit(EXIT_FAILURE);
        int r = peer_send(c, payload, sizeof(payload));
        peer_close(c);
        exit(r == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    Peer *s = peer_accept(PORT_BASE + 2);
    ASSERT_NEQ(NULL, s);
    unsigned char got[8] = { 0 };
    int r = peer_recv(s, got, sizeof(got));
    peer_close(s);

    int status;
    waitpid(pid, &status, 0);

    ASSERT_EQ(0, r);
    ASSERT_MEM_EQ(payload, got, sizeof(payload));
    ASSERT(WIFEXITED(status));
    ASSERT_EQ(EXIT_SUCCESS, WEXITSTATUS(status));
    PASS();
}

TEST send_from_server(void) {
    static const unsigned char payload[] = { 0xAA, 0xBB, 0xCC, 0xDD };

    pid_t pid = fork();
    if (pid == 0) {
        client_sleep();
        Peer *c = peer_connect("127.0.0.1", PORT_BASE + 3);
        if (!c) exit(EXIT_FAILURE);
        unsigned char got[4] = { 0 };
        int r = peer_recv(c, got, sizeof(got));
        peer_close(c);
        if (r != 0) exit(EXIT_FAILURE);
        exit(memcmp(payload, got, sizeof(payload)) == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    Peer *s = peer_accept(PORT_BASE + 3);
    ASSERT_NEQ(NULL, s);
    int r = peer_send(s, payload, sizeof(payload));
    peer_close(s);

    int status;
    waitpid(pid, &status, 0);

    ASSERT_EQ(0, r);
    ASSERT(WIFEXITED(status));
    ASSERT_EQ(EXIT_SUCCESS, WEXITSTATUS(status));
    PASS();
}

TEST echo_round_trip(void) {
    static const unsigned char payload[] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80
    };

    pid_t pid = fork();
    if (pid == 0) {
        client_sleep();
        Peer *c = peer_connect("127.0.0.1", PORT_BASE + 4);
        if (!c) exit(EXIT_FAILURE);
        if (peer_send(c, payload, sizeof(payload)) != 0) { peer_close(c); exit(EXIT_FAILURE); }
        unsigned char echo[16] = { 0 };
        if (peer_recv(c, echo, sizeof(echo)) != 0) { peer_close(c); exit(EXIT_FAILURE); }
        peer_close(c);
        exit(memcmp(payload, echo, sizeof(payload)) == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    Peer *s = peer_accept(PORT_BASE + 4);
    ASSERT_NEQ(NULL, s);
    unsigned char buf[16] = { 0 };
    int r1 = peer_recv(s, buf, sizeof(buf));
    int r2 = (r1 == 0) ? peer_send(s, buf, sizeof(buf)) : -1;
    peer_close(s);

    int status;
    waitpid(pid, &status, 0);

    ASSERT_EQ(0, r1);
    ASSERT_EQ(0, r2);
    ASSERT(WIFEXITED(status));
    ASSERT_EQ(EXIT_SUCCESS, WEXITSTATUS(status));
    PASS();
}

SUITE(io_suite) {
    RUN_TEST(accept_and_connect);
    RUN_TEST(send_from_client);
    RUN_TEST(send_from_server);
    RUN_TEST(echo_round_trip);
}

/* ------------------------------------------------------------------ */
/* Suite: close                                                         */
/* ------------------------------------------------------------------ */

TEST close_null_safe(void) {
    peer_close(NULL); /* must not crash */
    PASS();
}

SUITE(close_suite) {
    RUN_TEST(close_null_safe);
}

/* ------------------------------------------------------------------ */

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(connect_suite);
    RUN_SUITE(io_suite);
    RUN_SUITE(close_suite);
    GREATEST_MAIN_END();
}
