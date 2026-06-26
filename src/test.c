/**
 * rp2p.c - librp2p portable contract tests.
 * Summary: Validates exported librp2p behavior through the public C API.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "rp2p.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define KC_TEST_HOST "127.0.0.1"

#ifdef _WIN32
typedef SOCKET kc_socket_t;
typedef HANDLE kc_thread_t;
#define KC_SOCKET_INVALID INVALID_SOCKET
#else
typedef int kc_socket_t;
typedef pthread_t kc_thread_t;
#define KC_SOCKET_INVALID -1
#endif

typedef struct {
    rp2p_t *ctx;
    unsigned short port;
    int result;
    kc_thread_t thread;
} kc_index_t;

typedef struct {
    rp2p_t *ctx;
    const char *host;
    unsigned short index_port;
    const char *id;
    const char *target;
    unsigned short bind_port;
    int proto;
    const char *pass;
    int result;
    kc_thread_t thread;
} kc_peer_t;

typedef struct {
    unsigned short port;
    int proto;
    volatile int stop;
    kc_thread_t thread;
} kc_echo_t;

typedef struct {
    char ids[8][RP2P_ID_MAX + 1];
    int count;
} kc_publishers_t;

static int signal_count = 0;
static rp2p_t *signal_ctx_seen = NULL;
static int signal_count_b = 0;

/**
 * Returns a process-specific base TCP or UDP port.
 * @return Port base.
 */
static unsigned short kc_port_base(void) {
#ifdef _WIN32
    return (unsigned short)(25000UL + ((unsigned long)_getpid() % 20000UL));
#else
    return (unsigned short)(25000UL + ((unsigned long)getpid() % 20000UL));
#endif
}

/**
 * Sleeps for a bounded number of milliseconds.
 * @param ms Milliseconds to sleep.
 * @return None.
 */
static void kc_sleep_ms(unsigned int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;

    ts.tv_sec = (time_t)(ms / 1000U);
    ts.tv_nsec = (long)(ms % 1000U) * 1000000L;
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
    }
#endif
}

/**
 * Starts platform socket state.
 * @return 0 on success, 1 on failure.
 */
static int kc_socket_start(void) {
#ifdef _WIN32
    WSADATA data;

    return WSAStartup(MAKEWORD(2, 2), &data) == 0 ? 0 : 1;
#else
    signal(SIGPIPE, SIG_IGN);
    return 0;
#endif
}

/**
 * Stops platform socket state.
 * @return 0 on success.
 */
static int kc_socket_stop(void) {
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

/**
 * Closes one socket.
 * @param fd Socket descriptor.
 * @return 0 on success, non-zero on failure.
 */
static int kc_socket_close(kc_socket_t fd) {
#ifdef _WIN32
    return closesocket(fd);
#else
    return close(fd);
#endif
}

/**
 * Creates an IPv4 socket.
 * @param proto RP2P protocol value.
 * @return Socket descriptor or invalid socket.
 */
static kc_socket_t kc_socket_create(int proto) {
    int type;

    type = proto == RP2P_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
    return socket(AF_INET, type, 0);
}

/**
 * Enables fast local port reuse where supported.
 * @param fd Socket descriptor.
 * @return 0 on success.
 */
static int kc_socket_reuse(kc_socket_t fd) {
    int one;

    one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
    return 0;
}

/**
 * Sets a receive timeout on one socket.
 * @param fd Socket descriptor.
 * @param ms Timeout in milliseconds.
 * @return 0 on success.
 */
static int kc_socket_timeout(kc_socket_t fd, unsigned int ms) {
#ifdef _WIN32
    DWORD tv;

    tv = (DWORD)ms;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#else
    struct timeval tv;

    tv.tv_sec = (time_t)(ms / 1000U);
    tv.tv_usec = (suseconds_t)(ms % 1000U) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#endif
    return 0;
}

/**
 * Binds one IPv4 loopback socket.
 * @param fd Socket descriptor.
 * @param port TCP or UDP port.
 * @return 0 on success, 1 on failure.
 */
static int kc_socket_bind(kc_socket_t fd, unsigned short port) {
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) == 0 ? 0 : 1;
}

/**
 * Tests whether a loopback TCP port accepts a connection.
 * @param port TCP port.
 * @return 1 when open, 0 when closed.
 */
static int kc_port_open(unsigned short port) {
    kc_socket_t fd;
    struct sockaddr_in addr;
    int rc;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == KC_SOCKET_INVALID) return 0;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    rc = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    kc_socket_close(fd);
    return rc == 0 ? 1 : 0;
}

/**
 * Waits until a TCP port reaches the expected open state.
 * @param port TCP port.
 * @param open Expected state.
 * @return 1 when state is observed, 0 on timeout.
 */
static int kc_wait_port(unsigned short port, int open) {
    int i;

    for (i = 0; i < 80; i++) {
        if (kc_port_open(port) == open) return 1;
        kc_sleep_ms(100U);
    }
    return 0;
}

/**
 * Verifies one integer result and prints a descriptive pass/fail line.
 * @param name Check description.
 * @param expected Expected value.
 * @param actual Actual value.
 * @return 0 on success, 1 on failure.
 */
static int expect_int(const char *name, int expected, int actual) {
    if (expected != actual) {
        printf("\033[31m[FAIL]\033[0m %s: expected %d, got %d\n", name, expected, actual);
        return 1;
    }
    printf("\033[32m[PASS]\033[0m %s\n", name);
    return 0;
}

/**
 * Verifies one string result and prints a descriptive pass/fail line.
 * @param name Check description.
 * @param expected Expected string.
 * @param actual Actual string.
 * @return 0 on success, 1 on failure.
 */
static int expect_string(const char *name, const char *expected, const char *actual) {
    if (actual == NULL || strcmp(expected, actual) != 0) {
        printf("\033[31m[FAIL]\033[0m %s: expected '%s', got '%s'\n", name, expected,
            actual != NULL ? actual : "NULL");
        return 1;
    }
    printf("\033[32m[PASS]\033[0m %s\n", name);
    return 0;
}

/**
 * Verifies one boolean condition and prints a descriptive pass/fail line.
 * @param name Check description.
 * @param condition Non-zero when the check passed.
 * @return 0 on success, 1 on failure.
 */
static int expect_true(const char *name, int condition) {
    if (!condition) {
        printf("\033[31m[FAIL]\033[0m %s\n", name);
        return 1;
    }
    printf("\033[32m[PASS]\033[0m %s\n", name);
    return 0;
}

/**
 * Sets or clears a process environment variable.
 * @param name Variable name.
 * @param value Variable value, or NULL to clear.
 * @return 0 on success, 1 on failure.
 */
static int set_env_value(const char *name, const char *value) {
#ifdef _WIN32
    return _putenv_s(name, value != NULL ? value : "") == 0 ? 0 : 1;
#else
    if (value == NULL) return unsetenv(name) == 0 ? 0 : 1;
    return setenv(name, value, 1) == 0 ? 0 : 1;
#endif
}

/**
 * Stores one observed signal callback.
 * @param ctx Context passed by the library.
 * @return None.
 */
static void count_signal(rp2p_t *ctx) {
    if (ctx != NULL) {
        signal_count++;
        signal_ctx_seen = ctx;
    }
}

/**
 * Second callback to verify handler replacement.
 * @param ctx Context passed by the library.
 * @return None.
 */
static void count_signal_b(rp2p_t *ctx) {
    if (ctx != NULL) {
        signal_count_b++;
    }
}

#ifdef _WIN32
/**
 * Runs one index thread.
 * @param arg Thread argument.
 * @return Thread return value.
 */
static DWORD WINAPI index_main(void *arg) {
    kc_index_t *index;

    index = (kc_index_t *)arg;
    index->result = rp2p_serve_index(index->ctx, NULL, index->port);
    return 0;
}

/**
 * Runs one peer thread.
 * @param arg Thread argument.
 * @return Thread return value.
 */
static DWORD WINAPI peer_main(void *arg) {
    kc_peer_t *peer;

    peer = (kc_peer_t *)arg;
    rp2p_set_protocol(peer->ctx, peer->proto);
    rp2p_set_port(peer->ctx, peer->bind_port);
    if (peer->pass != NULL) rp2p_set_pass(peer->ctx, peer->pass);
    if (peer->id != NULL) {
        peer->result = rp2p_wait(peer->ctx, peer->host, peer->index_port,
            peer->id, peer->bind_port);
    } else {
        peer->result = rp2p_connect(peer->ctx, peer->host, peer->index_port,
            "consumer", peer->target, peer->bind_port);
    }
    return 0;
}
#else
/**
 * Runs one index thread.
 * @param arg Thread argument.
 * @return Thread return value.
 */
static void *index_main(void *arg) {
    kc_index_t *index;

    index = (kc_index_t *)arg;
    index->result = rp2p_serve_index(index->ctx, NULL, index->port);
    return NULL;
}

/**
 * Runs one peer thread.
 * @param arg Thread argument.
 * @return Thread return value.
 */
static void *peer_main(void *arg) {
    kc_peer_t *peer;

    peer = (kc_peer_t *)arg;
    rp2p_set_protocol(peer->ctx, peer->proto);
    rp2p_set_port(peer->ctx, peer->bind_port);
    if (peer->pass != NULL) rp2p_set_pass(peer->ctx, peer->pass);
    if (peer->id != NULL) {
        peer->result = rp2p_wait(peer->ctx, peer->host, peer->index_port,
            peer->id, peer->bind_port);
    } else {
        peer->result = rp2p_connect(peer->ctx, peer->host, peer->index_port,
            "consumer", peer->target, peer->bind_port);
    }
    return NULL;
}
#endif

/**
 * Starts one portable thread.
 * @param thread Output thread handle.
 * @param fn Thread function.
 * @param arg Thread argument.
 * @return 0 on success, 1 on failure.
 */
static int thread_start(kc_thread_t *thread,
#ifdef _WIN32
    DWORD (WINAPI *fn)(void *),
#else
    void *(*fn)(void *),
#endif
    void *arg)
{
#ifdef _WIN32
    *thread = CreateThread(NULL, 0, fn, arg, 0, NULL);
    return *thread != NULL ? 0 : 1;
#else
    return pthread_create(thread, NULL, fn, arg) == 0 ? 0 : 1;
#endif
}

/**
 * Joins one portable thread.
 * @param thread Thread handle.
 * @return 0 on success, 1 on failure.
 */
static int thread_join(kc_thread_t thread) {
#ifdef _WIN32
    if (WaitForSingleObject(thread, 15000U) != WAIT_OBJECT_0) return 1;
    CloseHandle(thread);
    return 0;
#else
    return pthread_join(thread, NULL) == 0 ? 0 : 1;
#endif
}

/**
 * Starts one index server thread.
 * @param index Index state.
 * @param port Control port.
 * @return 0 on success, 1 on failure.
 */
static int index_start(kc_index_t *index, unsigned short port) {
    memset(index, 0, sizeof(*index));
    index->port = port;
    if (rp2p_open(&index->ctx) != RP2P_OK) return 1;
    if (thread_start(&index->thread, index_main, index) != 0) return 1;
    return kc_wait_port(port, 1) ? 0 : 1;
}

/**
 * Stops one index server thread.
 * @param index Index state.
 * @return 0 on success, 1 on failure.
 */
static int index_stop(kc_index_t *index) {
    if (index->ctx != NULL) rp2p_stop(index->ctx);
    thread_join(index->thread);
    if (index->ctx != NULL) rp2p_close(index->ctx);
    index->ctx = NULL;
    return 0;
}

/**
 * Records one publisher id.
 * @param id Publisher identifier.
 * @param userdata Publisher collection.
 * @return None.
 */
static void on_publisher(const char *id, void *userdata) {
    kc_publishers_t *publishers;

    publishers = (kc_publishers_t *)userdata;
    if (publishers->count >= 8) return;
    strncpy(publishers->ids[publishers->count], id, RP2P_ID_MAX);
    publishers->ids[publishers->count][RP2P_ID_MAX] = '\0';
    publishers->count++;
}

/**
 * Returns whether one publisher id was collected.
 * @param publishers Publisher collection.
 * @param id Publisher identifier.
 * @return 1 when found, 0 otherwise.
 */
static int has_publisher(kc_publishers_t *publishers, const char *id) {
    int i;

    for (i = 0; i < publishers->count; i++) {
        if (strcmp(publishers->ids[i], id) == 0) return 1;
    }
    return 0;
}

#ifdef _WIN32
/**
 * Runs one echo backend.
 * @param arg Thread argument.
 * @return Thread return value.
 */
static DWORD WINAPI echo_main(void *arg)
#else
/**
 * Runs one echo backend.
 * @param arg Thread argument.
 * @return Thread return value.
 */
static void *echo_main(void *arg)
#endif
{
    kc_echo_t *echo;
    kc_socket_t fd;
    char buf[4096];

    echo = (kc_echo_t *)arg;
    fd = kc_socket_create(echo->proto);
    if (fd == KC_SOCKET_INVALID) {
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    kc_socket_reuse(fd);
    if (kc_socket_bind(fd, echo->port) != 0) {
        kc_socket_close(fd);
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    if (echo->proto == RP2P_PROTO_TCP) {
        listen(fd, 16);
        while (!echo->stop) {
            kc_socket_t client;
            int n;

            client = accept(fd, NULL, NULL);
            if (client == KC_SOCKET_INVALID) {
                kc_sleep_ms(20U);
                continue;
            }
            while ((n = (int)recv(client, buf, (int)sizeof(buf), 0)) > 0) {
                send(client, buf, n, 0);
            }
            kc_socket_close(client);
        }
    } else {
        while (!echo->stop) {
            struct sockaddr_in src;
#ifdef _WIN32
            int slen;
#else
            socklen_t slen;
#endif
            int n;

            slen = sizeof(src);
            n = (int)recvfrom(fd, buf, (int)sizeof(buf), 0,
                (struct sockaddr *)&src, &slen);
            if (n > 0) sendto(fd, buf, n, 0, (struct sockaddr *)&src, slen);
        }
    }
    kc_socket_close(fd);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/**
 * Starts one echo backend.
 * @param echo Echo backend state.
 * @param proto RP2P protocol value.
 * @param port Backend port.
 * @return 0 on success, 1 on failure.
 */
static int echo_start(kc_echo_t *echo, int proto, unsigned short port) {
    memset(echo, 0, sizeof(*echo));
    echo->proto = proto;
    echo->port = port;
    if (thread_start(&echo->thread, echo_main, echo) != 0) return 1;
    if (proto == RP2P_PROTO_TCP) return kc_wait_port(port, 1) ? 0 : 1;
    kc_sleep_ms(200U);
    return 0;
}

/**
 * Stops one echo backend.
 * @param echo Echo backend state.
 * @return 0 on success.
 */
static int echo_stop(kc_echo_t *echo) {
    echo->stop = 1;
    if (echo->proto == RP2P_PROTO_TCP) {
        kc_socket_t fd;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd != KC_SOCKET_INVALID) {
            struct sockaddr_in addr;

            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(echo->port);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
            kc_socket_close(fd);
        }
    } else {
        kc_socket_t fd;
        struct sockaddr_in addr;

        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd != KC_SOCKET_INVALID) {
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(echo->port);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            sendto(fd, "", 1, 0, (const struct sockaddr *)&addr, sizeof(addr));
            kc_socket_close(fd);
        }
    }
    thread_join(echo->thread);
    return 0;
}

/**
 * Starts one publisher or consumer peer.
 * @param peer Peer state.
 * @return 0 on success, 1 on failure.
 */
static int peer_start(kc_peer_t *peer) {
    if (rp2p_open(&peer->ctx) != RP2P_OK) return 1;
    return thread_start(&peer->thread, peer_main, peer);
}

/**
 * Stops one peer thread.
 * @param peer Peer state.
 * @return 0 on success.
 */
static int peer_stop(kc_peer_t *peer) {
    if (peer->ctx != NULL) rp2p_stop(peer->ctx);
    thread_join(peer->thread);
    if (peer->ctx != NULL) rp2p_close(peer->ctx);
    peer->ctx = NULL;
    return 0;
}

/**
 * Sends one TCP payload to a loopback port and reads the echo.
 * @param port TCP port.
 * @param message Payload.
 * @param out Output buffer.
 * @param cap Output buffer capacity.
 * @return 0 on success, 1 on failure.
 */
static int tcp_roundtrip(unsigned short port, const char *message, char *out, size_t cap) {
    kc_socket_t fd;
    struct sockaddr_in addr;
    int n;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == KC_SOCKET_INVALID) return 1;
    kc_socket_timeout(fd, 5000U);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
        kc_socket_close(fd);
        return 1;
    }
    send(fd, message, (int)strlen(message), 0);
#ifdef _WIN32
    shutdown(fd, SD_SEND);
#else
    shutdown(fd, SHUT_WR);
#endif
    n = (int)recv(fd, out, (int)cap - 1, 0);
    kc_socket_close(fd);
    if (n <= 0) return 1;
    out[n] = '\0';
    return 0;
}

/**
 * Sends one UDP payload to a loopback port and reads the echo.
 * @param port UDP port.
 * @param message Payload.
 * @param out Output buffer.
 * @param cap Output buffer capacity.
 * @return 0 on success, 1 on failure.
 */
static int udp_roundtrip(unsigned short port, const char *message, char *out, size_t cap) {
    kc_socket_t fd;
    struct sockaddr_in addr;
    int n;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == KC_SOCKET_INVALID) return 1;
    kc_socket_timeout(fd, 5000U);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sendto(fd, message, (int)strlen(message), 0,
        (const struct sockaddr *)&addr, sizeof(addr));
    n = (int)recv(fd, out, (int)cap - 1, 0);
    kc_socket_close(fd);
    if (n <= 0) return 1;
    out[n] = '\0';
    return 0;
}

#define KC_CTRL_HELLO          "RP2P_CTRTOK_HELLO RP2P/1"
#define KC_CTRL_HELLO_V0       "RP2P_CTRTOK_HELLO RP2P/0"
#define KC_CTRL_HELLO_V2       "RP2P_CTRTOK_HELLO RP2P/2"
#define KC_CTRL_HELLO_BAD      "RP2P_CTRTOK_HELLO RP2P/x"
#define KC_CTRL_HELLO_OK       "RP2P_CTRTOK_HELLO_OK"
#define KC_CTRL_REGISTER       "RP2P_CTRTOK_REGISTER:"
#define KC_CTRL_DEREGISTER     "RP2P_CTRTOK_DEREGISTER:"
#define KC_CTRL_LOOKUP         "RP2P_CTRTOK_LOOKUP:"
#define KC_CTRL_LIST_PUBS      "RP2P_CTRTOK_LIST_PUBLISHERS"
#define KC_CTRL_PUBLISHER      "RP2P_CTRTOK_PUBLISHER:"
#define KC_CTRL_END            "RP2P_CTRTOK_END"
#define KC_CTRL_CHALLENGE      "RP2P_CTRTOK_CHALLENGE:"
#define KC_CTRL_AUTH_FAILED    "RP2P_CTRTOK_AUTH_FAILED"
#define KC_CTRL_SOLUTION       "RP2P_CTRTOK_SOLUTION:"
#define KC_CTRL_PROOF          "RP2P_CTRTOK_PROOF:"
#define KC_CTRL_KEY            "RP2P_CTRTOK_KEY:"
#define KC_CTRL_NOT_FOUND      "RP2P_CTRTOK_NOT_FOUND"
#define KC_CTRL_PUNCH_REQ2     "RP2P_CTRTOK_PUNCH_REQ2:"
#define KC_CTRL_PUNCH_ACK2     "RP2P_CTRTOK_PUNCH_ACK2:"
#define KC_CTRL_CAND           "RP2P_CTRTOK_CAND:"
#define KC_CTRL_ERR_VERSION    "RP2P_CTRTOK_ERROR:version mismatch"
#define KC_CTRL_ERR_MALFORMED  "RP2P_CTRTOK_ERROR:malformed"
#define KC_CTRL_ERR_UNKNOWN    "RP2P_CTRTOK_ERROR:unknown command"
#define KC_CTRL_ERR_INVALID_ID "RP2P_CTRTOK_ERROR:invalid id"
#define KC_CTRL_ERR_INVALID_KEY "RP2P_CTRTOK_ERROR:invalid key"

/**
 * Sends lines to a TCP port and reads the reply.
 * @param port Target TCP port.
 * @param lines Buffer of newline-terminated lines.
 * @param out Output buffer.
 * @param cap Output buffer capacity.
 * @return 0 on success, 1 on failure.
 */
static int raw_tcp_exchange(unsigned short port, const char *lines,
    char *out, size_t cap)
{
    kc_socket_t fd;
    struct sockaddr_in addr;
    size_t len;
    int n;
    size_t total;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == KC_SOCKET_INVALID) return 1;
    kc_socket_timeout(fd, 3000U);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
        kc_socket_close(fd);
        return 1;
    }
    len = strlen(lines);
    send(fd, lines, (int)len, 0);
#ifdef _WIN32
    shutdown(fd, SD_SEND);
#else
    shutdown(fd, SHUT_WR);
#endif
    total = 0;
    for (;;) {
        n = (int)recv(fd, out + total, (int)(cap - 1 - total), 0);
        if (n <= 0) break;
        total += (size_t)n;
        if (total >= cap - 1) break;
    }
    out[total] = '\0';
    kc_socket_close(fd);
    return total > 0 ? 0 : 1;
}

/**
 * Verifies that a line exists in a multi-line buffer.
 * @param haystack Buffer of newline-separated lines.
 * @param needle Line to search for.
 * @return 1 when found, 0 otherwise.
 */
static int line_present(const char *haystack, const char *needle) {
    const char *p;
    size_t nlen;

    nlen = strlen(needle);
    p = haystack;
    for (;;) {
        const char *nl = strchr(p, '\n');
        size_t llen = nl != NULL ? (size_t)(nl - p) : strlen(p);
        if (llen == nlen && memcmp(p, needle, nlen) == 0) return 1;
        if (nl == NULL) break;
        p = nl + 1;
    }
    return 0;
}

/**
 * Sends a large buffer to a TCP port and reads back the echo, comparing bytes.
 * @param port TCP port.
 * @param data Send buffer.
 * @param size Send buffer size.
 * @param out Output buffer.
 * @param cap Output buffer capacity.
 * @return 0 on success, 1 on failure.
 */
static int tcp_large_transfer(unsigned short port,
    const char *data, size_t size, char *out, size_t cap)
{
    kc_socket_t fd;
    struct sockaddr_in addr;
    size_t sent_total;
    size_t recv_total;
    int n;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == KC_SOCKET_INVALID) return 1;
    kc_socket_timeout(fd, 60000U);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
        kc_socket_close(fd);
        return 1;
    }
    sent_total = 0;
    while (sent_total < size) {
        int s = (int)send(fd, data + sent_total, (int)(size - sent_total), 0);
        if (s <= 0) { kc_socket_close(fd); return 1; }
        sent_total += (size_t)s;
    }
#ifdef _WIN32
    shutdown(fd, SD_SEND);
#else
    shutdown(fd, SHUT_WR);
#endif
    recv_total = 0;
    for (;;) {
        n = (int)recv(fd, out + recv_total, (int)(cap - recv_total), 0);
        if (n <= 0) break;
        recv_total += (size_t)n;
    }
    kc_socket_close(fd);
    if (recv_total != size) return 1;
    if (memcmp(out, data, size) != 0) return 1;
    return 0;
}

/**
 * Sends a large UDP datagram and reads the echo, comparing bytes.
 * @param port UDP port.
 * @param data Send buffer.
 * @param size Send buffer size.
 * @param out Output buffer.
 * @param cap Output buffer capacity.
 * @return 0 on success, 1 on failure.
 */
static int udp_large_transfer(unsigned short port,
    const char *data, size_t size, char *out, size_t cap)
{
    kc_socket_t fd;
    struct sockaddr_in addr;
    int n;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == KC_SOCKET_INVALID) return 1;
    kc_socket_timeout(fd, 10000U);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (sendto(fd, data, (int)size, 0,
        (const struct sockaddr *)&addr, sizeof(addr)) != (int)size) {
        kc_socket_close(fd);
        return 1;
    }
    n = (int)recv(fd, out, (int)cap, 0);
    kc_socket_close(fd);
    if (n != (int)size) return 1;
    if (memcmp(out, data, size) != 0) return 1;
    return 0;
}

/**
 * Fills a buffer with pseudo-random bytes for large transfer tests.
 * @param buf Buffer to fill.
 * @param size Buffer size.
 * @return None.
 */
static void fill_random(char *buf, size_t size) {
    size_t i;
    unsigned int seed = 12345U;

    for (i = 0; i < size; i++) {
        seed = seed * 1103515245U + 12345U;
        buf[i] = (char)((seed >> 16) & 0xFF);
    }
}

/**
 * Starts an index with pass, vip, seats, and pow configuration.
 * @param index Index state.
 * @param port Control port.
 * @param pass Password or NULL.
 * @param vip VIP text or NULL.
 * @param seats Maximum peers (0 for default).
 * @param pow PoW bits (0 or positive).
 * @return 0 on success, 1 on failure.
 */
static int index_start_cfg(kc_index_t *index, unsigned short port,
    const char *pass, const char *vip, int seats, int pow)
{
    memset(index, 0, sizeof(*index));
    index->port = port;
    if (rp2p_open(&index->ctx) != RP2P_OK) return 1;
    if (pass != NULL) rp2p_set_pass(index->ctx, pass);
    if (vip != NULL) rp2p_set_vip(index->ctx, vip, NULL, 0);
    if (seats > 0) rp2p_set_seats(index->ctx, seats);
    if (pow >= 0) rp2p_set_pow(index->ctx, pow);
    if (thread_start(&index->thread, index_main, index) != 0) return 1;
    return kc_wait_port(port, 1) ? 0 : 1;
}

/**
 * Starts a publisher peer with fault-injection env vars.
 * @param peer Peer state.
 * @param host Index host.
 * @param index_port Index port.
 * @param id Publisher id.
 * @param target Target id or NULL for publisher.
 * @param bind_port Bind port.
 * @param proto Protocol.
 * @param pass Password or NULL.
 * @param drop_every RP2P_DEBUG_STREAM_DROP_EVERY value or NULL.
 * @param reorder_every RP2P_DEBUG_STREAM_REORDER_EVERY value or NULL.
 * @return 0 on success, 1 on failure.
 */
static int peer_start_fault(kc_peer_t *peer, const char *host,
    unsigned short index_port, const char *id, const char *target,
    unsigned short bind_port, int proto, const char *pass,
    const char *drop_every, const char *reorder_every)
{
    if (drop_every != NULL) set_env_value("RP2P_DEBUG_STREAM_DROP_EVERY", drop_every);
    if (reorder_every != NULL) set_env_value("RP2P_DEBUG_STREAM_REORDER_EVERY", reorder_every);
    memset(peer, 0, sizeof(*peer));
    peer->host = host;
    peer->index_port = index_port;
    peer->id = id;
    peer->target = target;
    peer->bind_port = bind_port;
    peer->proto = proto;
    peer->pass = pass;
    if (rp2p_open(&peer->ctx) != RP2P_OK) return 1;
    if (thread_start(&peer->thread, peer_main, peer) != 0) return 1;
    return 0;
}

/**
 * Verifies rp2p_version, rp2p_strerror in all meaningful paths.
 * @return 0 on success, 1 on failure.
 */
static int case_version_strerror(void) {
    int rc;

    rc = 0;
    rc += expect_true("version returns non-zero build timestamp",
        rp2p_version() != 0U);
    rc += expect_string("strerror(RP2P_OK) returns OK", "OK",
        rp2p_strerror(RP2P_OK));
    rc += expect_string("strerror(RP2P_ERROR) returns general error",
        "general error", rp2p_strerror(RP2P_ERROR));
    rc += expect_string("strerror(RP2P_ENET) returns network error",
        "network error", rp2p_strerror(RP2P_ENET));
    rc += expect_string("strerror(RP2P_ENOENT) returns peer not found",
        "peer not found", rp2p_strerror(RP2P_ENOENT));
    rc += expect_string("strerror(RP2P_ETIMEOUT) returns timeout",
        "timeout", rp2p_strerror(RP2P_ETIMEOUT));
    rc += expect_string("strerror(RP2P_EFULL) returns peer table full",
        "peer table full", rp2p_strerror(RP2P_EFULL));
    rc += expect_string("strerror(999) returns unknown error",
        "unknown error", rp2p_strerror(999));
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies rp2p_is_valid_id in all meaningful paths.
 * @return 0 on success, 1 on failure.
 */
static int case_validators(void) {
    int rc;

    rc = 0;
    rc += expect_int("valid id with alphanumeric", 1, rp2p_is_valid_id("abcXYZ123"));
    rc += expect_int("valid id single char", 1, rp2p_is_valid_id("a"));
    rc += expect_int("invalid id NULL", 0, rp2p_is_valid_id(NULL));
    rc += expect_int("invalid id empty string", 0, rp2p_is_valid_id(""));
    rc += expect_int("invalid id with colon", 0, rp2p_is_valid_id("bad:id"));
    rc += expect_int("invalid id with space", 0, rp2p_is_valid_id("bad id"));
    rc += expect_int("invalid id with at sign", 0, rp2p_is_valid_id("bad@id"));
    rc += expect_int("invalid id with dash", 0, rp2p_is_valid_id("bad-id"));
    rc += expect_int("invalid id too long", 0, rp2p_is_valid_id(
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
    rc += expect_int("valid pass with allowed specials", 1,
        rp2p_is_valid_pass_token("a._-+=,:@%/"));
    rc += expect_int("valid pass single char", 1, rp2p_is_valid_pass_token("x"));
    rc += expect_int("invalid pass NULL", 0, rp2p_is_valid_pass_token(NULL));
    rc += expect_int("invalid pass empty string", 0, rp2p_is_valid_pass_token(""));
    rc += expect_int("invalid pass with backtick", 0,
        rp2p_is_valid_pass_token("bad`pass"));
    rc += expect_int("invalid pass with space", 0,
        rp2p_is_valid_pass_token("bad pass"));
    rc += expect_int("invalid pass with exclamation", 0,
        rp2p_is_valid_pass_token("bad!pass"));
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies rp2p_options_default, rp2p_options_load_env, rp2p_options_free
 * in all meaningful paths.
 * @return 0 on success, 1 on failure.
 */
static int case_options(void) {
    rp2p_options_t opts;
    int rc;

    rc = 0;
    opts = rp2p_options_default();
    rc += expect_int("default index_port matches RP2P_PORT_DEFAULT",
        RP2P_PORT_DEFAULT, opts.index_port);
    rc += expect_int("default pow is 0", 0, opts.pow);
    rc += expect_int("default sweep is 20", 20, opts.sweep);
    rc += expect_true("default vip is NULL", opts.vip == NULL);
    rc += expect_true("default pass is empty", opts.pass[0] == '\0');
    rc += expect_true("default index_host is empty", opts.index_host[0] == '\0');
    rc += expect_true("default bind_addr is empty", opts.bind_addr[0] == '\0');
    rc += expect_true("default stun_url is empty", opts.stun_url[0] == '\0');

    set_env_value("RP2P_INDEX", NULL);
    set_env_value("RP2P_BIND", NULL);
    set_env_value("RP2P_PASS", NULL);
    set_env_value("RP2P_VIP", NULL);
    set_env_value("RP2P_SWEEP", NULL);
    set_env_value("RP2P_STUN", NULL);
    set_env_value("RP2P_SEATS", NULL);
    set_env_value("RP2P_POW", NULL);

    rp2p_options_load_env(&opts);
    rc += expect_true("load_env without env vars does not change index_host",
        opts.index_host[0] == '\0');
    rc += expect_true("load_env without env vars does not change vip",
        opts.vip == NULL);

    set_env_value("RP2P_INDEX", "idx.local:1234");
    set_env_value("RP2P_BIND", "127.0.0.1:4567");
    set_env_value("RP2P_PASS", "abc");
    set_env_value("RP2P_VIP", "vip vip-pass");
    set_env_value("RP2P_SWEEP", "9");
    set_env_value("RP2P_STUN", "stun:example.com:3478");
    set_env_value("RP2P_SEATS", "50");
    set_env_value("RP2P_POW", "20");
    rp2p_options_load_env(&opts);
    rc += expect_string("load_env parses RP2P_INDEX host", "idx.local",
        opts.index_host);
    rc += expect_int("load_env parses RP2P_INDEX port", 1234, opts.index_port);
    rc += expect_string("load_env parses RP2P_BIND addr", "127.0.0.1",
        opts.bind_addr);
    rc += expect_int("load_env parses RP2P_BIND port", 4567, opts.bind_port);
    rc += expect_string("load_env parses RP2P_PASS", "abc", opts.pass);
    rc += expect_string("load_env parses RP2P_VIP", "vip vip-pass", opts.vip);
    rc += expect_int("load_env parses RP2P_SWEEP", 9, opts.sweep);
    rc += expect_string("load_env parses RP2P_STUN", "stun:example.com:3478",
        opts.stun_url);
    rc += expect_int("load_env parses RP2P_SEATS", 50, opts.seats);
    rc += expect_int("load_env parses RP2P_POW", 20, opts.pow);

    set_env_value("RP2P_INDEX", NULL);
    set_env_value("RP2P_BIND", NULL);
    set_env_value("RP2P_PASS", NULL);
    set_env_value("RP2P_VIP", NULL);
    set_env_value("RP2P_SWEEP", NULL);
    set_env_value("RP2P_STUN", NULL);
    set_env_value("RP2P_SEATS", NULL);
    set_env_value("RP2P_POW", NULL);

    rp2p_options_free(&opts);
    rc += expect_true("options_free clears vip to NULL", opts.vip == NULL);
    rp2p_options_free(&opts);
    rc += expect_true("options_free is safe to call twice", opts.vip == NULL);
    rp2p_options_free(NULL);
    rc += expect_true("options_free(NULL) does not crash", 1);
    rp2p_options_load_env(NULL);
    rc += expect_true("load_env(NULL) does not crash", 1);

    return rc == 0 ? 0 : 1;
}

/**
 * Verifies rp2p_open and rp2p_close in all meaningful paths.
 * @return 0 on success, 1 on failure.
 */
static int case_open_close(void) {
    rp2p_t *ctx;
    int rc;

    rc = 0;
    ctx = NULL;
    rc += expect_int("open(NULL) returns ERROR", RP2P_ERROR, rp2p_open(NULL));
    rc += expect_true("open(NULL) leaves out as NULL", ctx == NULL);
    rc += expect_int("open(out) returns OK", RP2P_OK, rp2p_open(&ctx));
    rc += expect_true("open sets ctx to non-NULL", ctx != NULL);
    rc += expect_int("close(NULL) returns ERROR", RP2P_ERROR, rp2p_close(NULL));
    rc += expect_int("close(ctx) returns OK", RP2P_OK, rp2p_close(ctx));
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies rp2p_stop in all meaningful paths.
 * @return 0 on success, 1 on failure.
 */
static int case_stop(void) {
    rp2p_t *ctx;
    int rc;

    rc = 0;
    rc += expect_int("stop(NULL) returns ERROR", RP2P_ERROR, rp2p_stop(NULL));
    rc += expect_int("open returns OK", RP2P_OK, rp2p_open(&ctx));
    rc += expect_int("stop(ctx) returns OK", RP2P_OK, rp2p_stop(ctx));
    rc += expect_int("stop(ctx) second call is idempotent", RP2P_OK,
        rp2p_stop(ctx));
    rc += expect_int("close after stop returns OK", RP2P_OK, rp2p_close(ctx));
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies all setter functions: set_seats, set_pow, set_port, set_protocol,
 * set_pass, set_vip, set_sweep, set_stun_url in all meaningful paths.
 * @return 0 on success, 1 on failure.
 */
static int case_setters(void) {
    rp2p_t *ctx;
    char err[128];
    int rc;

    rc = 0;
    rc += expect_int("set_seats(NULL) returns ERROR", RP2P_ERROR,
        rp2p_set_seats(NULL, 10));
    rc += expect_int("set_pow(NULL) returns ERROR", RP2P_ERROR,
        rp2p_set_pow(NULL, 10));
    rc += expect_int("set_port(NULL) returns ERROR", RP2P_ERROR,
        rp2p_set_port(NULL, 9000));
    rc += expect_int("set_protocol(NULL) returns ERROR", RP2P_ERROR,
        rp2p_set_protocol(NULL, RP2P_PROTO_TCP));
    rc += expect_int("set_pass(NULL, pass) returns ERROR", RP2P_ERROR,
        rp2p_set_pass(NULL, "pass"));
    rc += expect_int("set_vip(NULL) returns ERROR", RP2P_ERROR,
        rp2p_set_vip(NULL, "vip pass", err, sizeof(err)));
    rc += expect_int("set_sweep(NULL) returns ERROR", RP2P_ERROR,
        rp2p_set_sweep(NULL, 10));
    rc += expect_int("set_stun_url(NULL) returns ERROR", RP2P_ERROR,
        rp2p_set_stun_url(NULL, "stun:example.com:3478"));

    rc += expect_int("open returns OK", RP2P_OK, rp2p_open(&ctx));

    rc += expect_int("set_seats(ctx, 3) returns OK", RP2P_OK,
        rp2p_set_seats(ctx, 3));
    rc += expect_int("set_seats(ctx, -1) returns OK", RP2P_OK,
        rp2p_set_seats(ctx, -1));
    rc += expect_int("set_pow(ctx, 20) returns OK", RP2P_OK,
        rp2p_set_pow(ctx, 20));
    rc += expect_int("set_pow(ctx, -1) returns OK and clamps to 0", RP2P_OK,
        rp2p_set_pow(ctx, -1));
    rc += expect_int("set_port(ctx, 9000) returns OK", RP2P_OK,
        rp2p_set_port(ctx, 9000));
    rc += expect_int("set_port(ctx, 0) returns OK", RP2P_OK,
        rp2p_set_port(ctx, 0));
    rc += expect_int("set_protocol(ctx, TCP) returns OK", RP2P_OK,
        rp2p_set_protocol(ctx, RP2P_PROTO_TCP));
    rc += expect_int("set_protocol(ctx, UDP) returns OK", RP2P_OK,
        rp2p_set_protocol(ctx, RP2P_PROTO_UDP));
    rc += expect_int("set_protocol(ctx, 99) returns ERROR", RP2P_ERROR,
        rp2p_set_protocol(ctx, 99));
    rc += expect_int("set_protocol(ctx, 0) returns ERROR", RP2P_ERROR,
        rp2p_set_protocol(ctx, 0));
    rc += expect_int("set_pass(ctx, secret) returns OK", RP2P_OK,
        rp2p_set_pass(ctx, "secret"));
    rc += expect_int("set_pass(ctx, empty) returns OK", RP2P_OK,
        rp2p_set_pass(ctx, ""));
    rc += expect_int("set_pass(ctx, NULL) returns ERROR", RP2P_ERROR,
        rp2p_set_pass(ctx, NULL));
    rc += expect_int("set_pass(ctx, bad backtick) returns ERROR", RP2P_ERROR,
        rp2p_set_pass(ctx, "bad`pass"));
    rc += expect_int("set_pass(ctx, allowed specials) returns OK", RP2P_OK,
        rp2p_set_pass(ctx, "a._-+=,:@%/"));
    rc += expect_int("set_vip(ctx, valid pair) returns OK", RP2P_OK,
        rp2p_set_vip(ctx, "one pass1 two pass2", err, sizeof(err)));
    rc += expect_int("set_vip(ctx, odd token count) returns ERROR", RP2P_ERROR,
        rp2p_set_vip(ctx, "one", err, sizeof(err)));
    rc += expect_int("set_vip(ctx, invalid id) returns ERROR", RP2P_ERROR,
        rp2p_set_vip(ctx, "bad:id pass", err, sizeof(err)));
    rc += expect_int("set_vip(ctx, invalid pass) returns ERROR", RP2P_ERROR,
        rp2p_set_vip(ctx, "goodid bad`pass", err, sizeof(err)));
    rc += expect_int("set_vip(ctx, duplicate seat) returns ERROR", RP2P_ERROR,
        rp2p_set_vip(ctx, "one pass1 one pass2", err, sizeof(err)));
    rc += expect_int("set_vip(ctx, NULL) returns OK and clears", RP2P_OK,
        rp2p_set_vip(ctx, NULL, err, sizeof(err)));
    rc += expect_int("set_vip(ctx, empty string) returns OK", RP2P_OK,
        rp2p_set_vip(ctx, "", err, sizeof(err)));
    rc += expect_int("set_sweep(ctx, 10) returns OK", RP2P_OK,
        rp2p_set_sweep(ctx, 10));
    rc += expect_int("set_sweep(ctx, 0) returns OK", RP2P_OK,
        rp2p_set_sweep(ctx, 0));
    rc += expect_int("set_stun_url(ctx, valid) returns OK", RP2P_OK,
        rp2p_set_stun_url(ctx, "stun:example.com:3478"));
    rc += expect_int("set_stun_url(ctx, NULL) returns OK and clears", RP2P_OK,
        rp2p_set_stun_url(ctx, NULL));

    rc += expect_int("close returns OK", RP2P_OK, rp2p_close(ctx));
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies rp2p_on_signal and rp2p_raise_signal in all meaningful paths:
 * NULL guards, registration, replacement, removal, removal of non-existent,
 * multiple signals, callback ctx routing, and dynamic array growth.
 * @return 0 on success, 1 on failure.
 */
static int case_signals(void) {
    rp2p_t *ctx;
    int rc;
    int i;

    rc = 0;
    signal_count = 0;
    signal_count_b = 0;
    signal_ctx_seen = NULL;

    rc += expect_int("on_signal(NULL) returns ERROR", RP2P_ERROR,
        rp2p_on_signal(NULL, 1, count_signal));
    rc += expect_int("raise_signal(NULL) returns 0", 0,
        rp2p_raise_signal(NULL, 1));
    rc += expect_int("listen_signals(NULL) returns ERROR", RP2P_ERROR,
        rp2p_listen_signals(NULL));

    rc += expect_int("open returns OK", RP2P_OK, rp2p_open(&ctx));

    rc += expect_int("raise_signal on unhandled signal returns 0", 0,
        rp2p_raise_signal(ctx, 1));

    rc += expect_int("register signal 1 handler returns OK", RP2P_OK,
        rp2p_on_signal(ctx, 1, count_signal));
    rc += expect_int("raise signal 1 after register returns 1", 1,
        rp2p_raise_signal(ctx, 1));
    rc += expect_int("signal callback was invoked exactly once", 1,
        signal_count);
    rc += expect_true("signal callback received the correct ctx pointer",
        signal_ctx_seen == ctx);

    rc += expect_int("register signal 2 handler returns OK", RP2P_OK,
        rp2p_on_signal(ctx, 2, count_signal));
    signal_count = 0;
    rc += expect_int("raise signal 2 returns 1", 1,
        rp2p_raise_signal(ctx, 2));
    rc += expect_int("signal 2 callback was invoked", 1, signal_count);

    signal_count = 0;
    rc += expect_int("raise signal 1 still works with two handlers", 1,
        rp2p_raise_signal(ctx, 1));
    rc += expect_int("signal 1 callback was invoked with two handlers", 1,
        signal_count);

    rc += expect_int("register second handler for signal 1 returns OK",
        RP2P_OK, rp2p_on_signal(ctx, 1, count_signal_b));
    signal_count = 0;
    signal_count_b = 0;
    rc += expect_int("raise signal 1 with two handlers returns 2", 2,
        rp2p_raise_signal(ctx, 1));
    rc += expect_int("first callback was invoked", 1, signal_count);
    rc += expect_int("second callback was invoked", 1, signal_count_b);

    rc += expect_int("remove first signal 1 handler returns OK", RP2P_OK,
        rp2p_on_signal(ctx, 1, NULL));
    signal_count = 0;
    signal_count_b = 0;
    rc += expect_int("raise signal 1 after removing one handler returns 1",
        1, rp2p_raise_signal(ctx, 1));
    rc += expect_int("remaining handler was invoked", 1, signal_count_b);

    rc += expect_int("remove second signal 1 handler returns OK", RP2P_OK,
        rp2p_on_signal(ctx, 1, NULL));
    rc += expect_int("raise signal 1 after removing all handlers returns 0",
        0, rp2p_raise_signal(ctx, 1));

    rc += expect_int("remove non-existent signal 100 returns ENOENT",
        RP2P_ENOENT, rp2p_on_signal(ctx, 100, NULL));

    rc += expect_int("remove signal 2 handler returns OK", RP2P_OK,
        rp2p_on_signal(ctx, 2, NULL));
    rc += expect_int("raise signal 2 after removal returns 0", 0,
        rp2p_raise_signal(ctx, 2));

    for (i = 0; i < 8; i++) {
        rc += expect_int("register handler for growth signal returns OK",
            RP2P_OK, rp2p_on_signal(ctx, 200 + i, count_signal));
    }
    signal_count = 0;
    rc += expect_int("raise last growth signal returns 1", 1,
        rp2p_raise_signal(ctx, 207));
    rc += expect_int("growth signal callback was invoked", 1, signal_count);

    for (i = 0; i < 8; i++) {
        rp2p_on_signal(ctx, 200 + i, NULL);
    }

    rc += expect_int("listen_signals(ctx) returns OK", RP2P_OK,
        rp2p_listen_signals(ctx));
    rc += expect_int("close returns OK", RP2P_OK, rp2p_close(ctx));
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies that two contexts coexist with independent stop state, that
 * stop on one does not affect the other, and that listen_signals
 * registers both in the global list.
 * @return 0 on success, 1 on failure.
 */
static int case_multictx(void) {
    rp2p_t *first;
    rp2p_t *second;
    int rc;

    rc = 0;
    first = NULL;
    second = NULL;

    rc += expect_int("open first returns OK", RP2P_OK, rp2p_open(&first));
    rc += expect_int("open second returns OK", RP2P_OK, rp2p_open(&second));
    rc += expect_int("stop first returns OK", RP2P_OK, rp2p_stop(first));
    rc += expect_int("stop second returns OK", RP2P_OK, rp2p_stop(second));
    rc += expect_int("stop first again is idempotent", RP2P_OK,
        rp2p_stop(first));
    rc += expect_int("listen_signals(first) returns OK", RP2P_OK,
        rp2p_listen_signals(first));
    rc += expect_int("listen_signals(second) returns OK", RP2P_OK,
        rp2p_listen_signals(second));
    rc += expect_int("close first returns OK", RP2P_OK, rp2p_close(first));
    rc += expect_int("close second returns OK", RP2P_OK, rp2p_close(second));
    rc += expect_int("stop(NULL) returns ERROR", RP2P_ERROR, rp2p_stop(NULL));
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies listing, registration, and deregistration through the library API.
 * @return 0 on success, 1 on failure.
 */
static int case_index_catalog(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t peer;
    rp2p_t *client;
    kc_publishers_t publishers;
    int rc;

    base = kc_port_base();
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&peer, 0, sizeof(peer));
    peer.host = KC_TEST_HOST;
    peer.index_port = (unsigned short)(base + 1U);
    peer.id = "target";
    peer.bind_port = (unsigned short)(base + 2U);
    peer.proto = RP2P_PROTO_TCP;
    if (peer_start(&peer) != 0) return 1;
    kc_sleep_ms(800U);
    memset(&publishers, 0, sizeof(publishers));
    rc += expect_int("open list client returns OK", RP2P_OK, rp2p_open(&client));
    rc += expect_int("list publishers returns OK", RP2P_OK, rp2p_list_publishers(client,
        KC_TEST_HOST, (unsigned short)(base + 1U), on_publisher, &publishers));
    rc += expect_true("publisher list contains target", has_publisher(&publishers, "target"));
    rc += expect_int("deregister target returns OK", RP2P_OK, rp2p_deregister(client,
        KC_TEST_HOST, (unsigned short)(base + 1U), "target"));
    memset(&publishers, 0, sizeof(publishers));
    rc += expect_int("list after deregister returns OK", RP2P_OK, rp2p_list_publishers(client,
        KC_TEST_HOST, (unsigned short)(base + 1U), on_publisher, &publishers));
    rc += expect_true("target removed after deregister", !has_publisher(&publishers, "target"));
    rp2p_close(client);
    peer_stop(&peer);
    echo_stop(&echo);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies TCP tunnel behavior through library publisher and consumer APIs.
 * @return 0 on success, 1 on failure.
 */
static int case_tcp_tunnel(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char out[64];
    int rc;

    base = (unsigned short)(kc_port_base() + 20U);
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&publisher, 0, sizeof(publisher));
    publisher.host = KC_TEST_HOST;
    publisher.index_port = (unsigned short)(base + 1U);
    publisher.id = "target";
    publisher.bind_port = (unsigned short)(base + 2U);
    publisher.proto = RP2P_PROTO_TCP;
    if (peer_start(&publisher) != 0) return 1;
    kc_sleep_ms(800U);
    memset(&consumer, 0, sizeof(consumer));
    consumer.host = KC_TEST_HOST;
    consumer.index_port = (unsigned short)(base + 1U);
    consumer.target = "target";
    consumer.bind_port = (unsigned short)(base + 3U);
    consumer.proto = RP2P_PROTO_TCP;
    if (peer_start(&consumer) != 0) return 1;
    if (!kc_wait_port((unsigned short)(base + 3U), 1)) return 1;
    rc += expect_int("tcp roundtrip succeeds", 0, tcp_roundtrip((unsigned short)(base + 3U),
        "ping", out, sizeof(out)));
    rc += expect_string("tcp payload matches", "ping", out);
    peer_stop(&consumer);
    peer_stop(&publisher);
    echo_stop(&echo);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies UDP tunnel behavior through library publisher and consumer APIs.
 * @return 0 on success, 1 on failure.
 */
static int case_udp_tunnel(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char out[64];
    int rc;

    base = (unsigned short)(kc_port_base() + 40U);
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_UDP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&publisher, 0, sizeof(publisher));
    publisher.host = KC_TEST_HOST;
    publisher.index_port = (unsigned short)(base + 1U);
    publisher.id = "target";
    publisher.bind_port = (unsigned short)(base + 2U);
    publisher.proto = RP2P_PROTO_UDP;
    if (peer_start(&publisher) != 0) return 1;
    kc_sleep_ms(800U);
    memset(&consumer, 0, sizeof(consumer));
    consumer.host = KC_TEST_HOST;
    consumer.index_port = (unsigned short)(base + 1U);
    consumer.target = "target";
    consumer.bind_port = (unsigned short)(base + 3U);
    consumer.proto = RP2P_PROTO_UDP;
    if (peer_start(&consumer) != 0) return 1;
    kc_sleep_ms(800U);
    rc += expect_int("udp roundtrip succeeds", 0, udp_roundtrip((unsigned short)(base + 3U),
        "pong", out, sizeof(out)));
    rc += expect_string("udp payload matches", "pong", out);
    peer_stop(&consumer);
    peer_stop(&publisher);
    echo_stop(&echo);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies that the index control plane is TCP-only.
 * @return 0 on success, 1 on failure.
 */
static int case_index_tcp_only(void) {
    unsigned short base;
    kc_index_t index;
    kc_socket_t fd;
    struct sockaddr_in addr;
    char byte;
    int n;
    int rc;

    base = (unsigned short)(kc_port_base() + 100U);
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == KC_SOCKET_INVALID) return 1;
    kc_socket_timeout(fd, 400U);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)(base + 1U));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sendto(fd, "x", 1, 0, (const struct sockaddr *)&addr, sizeof(addr));
    n = (int)recv(fd, &byte, 1, 0);
    rc += expect_true("index UDP control port does not answer", n <= 0);
    kc_socket_close(fd);
    rc += expect_true("index TCP control port accepts", kc_port_open(index.port));
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies raw control protocol handshake, lookup, list, deregister, errors.
 * @return 0 on success, 1 on failure.
 */
static int case_control_catalog(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t peer;
    char out[2048];
    char req[512];
    int rc;

    base = (unsigned short)(kc_port_base() + 120U);
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&peer, 0, sizeof(peer));
    peer.host = KC_TEST_HOST;
    peer.index_port = (unsigned short)(base + 1U);
    peer.id = "rawtarget";
    peer.bind_port = (unsigned short)(base + 2U);
    peer.proto = RP2P_PROTO_TCP;
    if (peer_start(&peer) != 0) return 1;
    kc_sleep_ms(800U);
    snprintf(req, sizeof(req), "%s\n%srawtarget\n", KC_CTRL_HELLO,
        KC_CTRL_LOOKUP);
    rc += expect_int("raw lookup succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("raw lookup includes publisher token",
        strstr(out, KC_CTRL_PUBLISHER) != NULL);
    snprintf(req, sizeof(req), "%s\n%s\n", KC_CTRL_HELLO, KC_CTRL_LIST_PUBS);
    rc += expect_int("raw list succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("raw list includes publisher id",
        strstr(out, "rawtarget") != NULL);
    snprintf(req, sizeof(req), "%s\n%srawtarget\n%s\n", KC_CTRL_HELLO,
        KC_CTRL_DEREGISTER, KC_CTRL_LIST_PUBS);
    rc += expect_int("raw deregister succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("raw deregister removes publisher",
        !line_present(out, "rawtarget"));
    snprintf(req, sizeof(req), "%s\n", KC_CTRL_HELLO_V2);
    rc += expect_int("raw bad version exchange succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("bad version returns version error",
        strstr(out, KC_CTRL_ERR_VERSION) != NULL);
    snprintf(req, sizeof(req), "%s\nBOGUS\n", KC_CTRL_HELLO);
    rc += expect_int("raw unknown command exchange succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("unknown command returns error",
        strstr(out, KC_CTRL_ERR_UNKNOWN) != NULL);
    peer_stop(&peer);
    echo_stop(&echo);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies adversarial protocol vectors do not corrupt the index.
 * @return 0 on success, 1 on failure.
 */
static int case_protocol_vectors(void) {
    unsigned short base;
    kc_index_t index;
    char out[2048];
    char req[512];
    int rc;

    base = (unsigned short)(kc_port_base() + 140U);
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    snprintf(req, sizeof(req), "%s\n", KC_CTRL_HELLO_BAD);
    rc += expect_int("bad hello exchange succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("bad hello returns malformed/version error",
        strstr(out, KC_CTRL_ERR_VERSION) != NULL ||
        strstr(out, KC_CTRL_ERR_MALFORMED) != NULL);
    snprintf(req, sizeof(req), "%s\n%sbad:id 127.0.0.1 1\n", KC_CTRL_HELLO,
        KC_CTRL_REGISTER);
    rc += expect_int("invalid id register exchange succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("invalid id register returns error",
        strstr(out, KC_CTRL_ERR_INVALID_ID) != NULL ||
        strstr(out, KC_CTRL_ERR_MALFORMED) != NULL);
    snprintf(req, sizeof(req), "%s\n%snope\n", KC_CTRL_HELLO,
        KC_CTRL_LOOKUP);
    rc += expect_int("not found lookup exchange succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("not found lookup reports missing peer",
        strstr(out, KC_CTRL_NOT_FOUND) != NULL);
    snprintf(req, sizeof(req), "%s\n%s\n", KC_CTRL_HELLO,
        KC_CTRL_PUNCH_ACK2);
    rc += expect_int("malformed punch exchange succeeds", 0,
        raw_tcp_exchange(index.port, req, out, sizeof(out)));
    rc += expect_true("malformed punch does not close control socket",
        out[0] != '\0');
    rc += expect_true("index remains alive after protocol vectors",
        kc_port_open(index.port));
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies two concurrent TCP clients can use one tunnel.
 * @return 0 on success, 1 on failure.
 */
static int case_tcp_concurrent(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char one[64];
    char two[64];
    int rc;

    base = (unsigned short)(kc_port_base() + 160U);
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&publisher, 0, sizeof(publisher));
    publisher.host = KC_TEST_HOST;
    publisher.index_port = (unsigned short)(base + 1U);
    publisher.id = "target";
    publisher.bind_port = (unsigned short)(base + 2U);
    publisher.proto = RP2P_PROTO_TCP;
    if (peer_start(&publisher) != 0) return 1;
    kc_sleep_ms(800U);
    memset(&consumer, 0, sizeof(consumer));
    consumer.host = KC_TEST_HOST;
    consumer.index_port = (unsigned short)(base + 1U);
    consumer.target = "target";
    consumer.bind_port = (unsigned short)(base + 3U);
    consumer.proto = RP2P_PROTO_TCP;
    if (peer_start(&consumer) != 0) return 1;
    if (!kc_wait_port((unsigned short)(base + 3U), 1)) return 1;
    rc += expect_int("first tcp client succeeds", 0,
        tcp_roundtrip((unsigned short)(base + 3U), "one", one, sizeof(one)));
    rc += expect_int("second tcp client succeeds", 0,
        tcp_roundtrip((unsigned short)(base + 3U), "two", two, sizeof(two)));
    rc += expect_string("first tcp payload matches", "one", one);
    rc += expect_string("second tcp payload matches", "two", two);
    peer_stop(&consumer);
    peer_stop(&publisher);
    echo_stop(&echo);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies reconnect by opening multiple sequential TCP streams.
 * @return 0 on success, 1 on failure.
 */
static int case_tcp_reconnect(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char out[64];
    int rc;

    base = (unsigned short)(kc_port_base() + 180U);
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&publisher, 0, sizeof(publisher));
    publisher.host = KC_TEST_HOST;
    publisher.index_port = (unsigned short)(base + 1U);
    publisher.id = "target";
    publisher.bind_port = (unsigned short)(base + 2U);
    publisher.proto = RP2P_PROTO_TCP;
    if (peer_start(&publisher) != 0) return 1;
    kc_sleep_ms(800U);
    memset(&consumer, 0, sizeof(consumer));
    consumer.host = KC_TEST_HOST;
    consumer.index_port = (unsigned short)(base + 1U);
    consumer.target = "target";
    consumer.bind_port = (unsigned short)(base + 3U);
    consumer.proto = RP2P_PROTO_TCP;
    if (peer_start(&consumer) != 0) return 1;
    if (!kc_wait_port((unsigned short)(base + 3U), 1)) return 1;
    rc += expect_int("first reconnect roundtrip succeeds", 0,
        tcp_roundtrip((unsigned short)(base + 3U), "first", out, sizeof(out)));
    rc += expect_string("first reconnect payload matches", "first", out);
    rc += expect_int("second reconnect roundtrip succeeds", 0,
        tcp_roundtrip((unsigned short)(base + 3U), "second", out, sizeof(out)));
    rc += expect_string("second reconnect payload matches", "second", out);
    peer_stop(&consumer);
    peer_stop(&publisher);
    echo_stop(&echo);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies a large TCP payload remains byte-exact through the tunnel.
 * @return 0 on success, 1 on failure.
 */
static int case_tcp_large(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char data[65536];
    char out[65536];
    int rc;

    base = (unsigned short)(kc_port_base() + 200U);
    rc = 0;
    fill_random(data, sizeof(data));
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&publisher, 0, sizeof(publisher));
    publisher.host = KC_TEST_HOST;
    publisher.index_port = (unsigned short)(base + 1U);
    publisher.id = "target";
    publisher.bind_port = (unsigned short)(base + 2U);
    publisher.proto = RP2P_PROTO_TCP;
    if (peer_start(&publisher) != 0) return 1;
    kc_sleep_ms(800U);
    memset(&consumer, 0, sizeof(consumer));
    consumer.host = KC_TEST_HOST;
    consumer.index_port = (unsigned short)(base + 1U);
    consumer.target = "target";
    consumer.bind_port = (unsigned short)(base + 3U);
    consumer.proto = RP2P_PROTO_TCP;
    if (peer_start(&consumer) != 0) return 1;
    if (!kc_wait_port((unsigned short)(base + 3U), 1)) return 1;
    rc += expect_int("large tcp transfer succeeds", 0,
        tcp_large_transfer((unsigned short)(base + 3U), data, sizeof(data),
            out, sizeof(out)));
    peer_stop(&consumer);
    peer_stop(&publisher);
    echo_stop(&echo);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies TCP recovery when debug stream loss is enabled.
 * @return 0 on success, 1 on failure.
 */
static int case_tcp_loss(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char out[64];
    int rc;

    base = (unsigned short)(kc_port_base() + 220U);
    rc = 0;
    set_env_value("RP2P_DEBUG_STREAM_DROP_EVERY", NULL);
    set_env_value("RP2P_DEBUG_STREAM_REORDER_EVERY", NULL);
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    if (peer_start_fault(&publisher, KC_TEST_HOST, (unsigned short)(base + 1U),
        "target", NULL, (unsigned short)(base + 2U), RP2P_PROTO_TCP, NULL,
        "3", NULL) != 0) return 1;
    kc_sleep_ms(800U);
    if (peer_start_fault(&consumer, KC_TEST_HOST, (unsigned short)(base + 1U),
        NULL, "target", (unsigned short)(base + 3U), RP2P_PROTO_TCP, NULL,
        NULL, NULL) != 0) return 1;
    if (!kc_wait_port((unsigned short)(base + 3U), 1)) return 1;
    rc += expect_int("loss tcp roundtrip succeeds", 0,
        tcp_roundtrip((unsigned short)(base + 3U), "loss", out, sizeof(out)));
    rc += expect_string("loss tcp payload matches", "loss", out);
    peer_stop(&consumer);
    peer_stop(&publisher);
    echo_stop(&echo);
    index_stop(&index);
    set_env_value("RP2P_DEBUG_STREAM_DROP_EVERY", NULL);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies TCP recovery when debug stream reordering is enabled.
 * @return 0 on success, 1 on failure.
 */
static int case_tcp_reorder(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char out[64];
    int rc;

    base = (unsigned short)(kc_port_base() + 240U);
    rc = 0;
    set_env_value("RP2P_DEBUG_STREAM_DROP_EVERY", NULL);
    set_env_value("RP2P_DEBUG_STREAM_REORDER_EVERY", NULL);
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    if (peer_start_fault(&publisher, KC_TEST_HOST, (unsigned short)(base + 1U),
        "target", NULL, (unsigned short)(base + 2U), RP2P_PROTO_TCP, NULL,
        NULL, "2") != 0) return 1;
    kc_sleep_ms(800U);
    if (peer_start_fault(&consumer, KC_TEST_HOST, (unsigned short)(base + 1U),
        NULL, "target", (unsigned short)(base + 3U), RP2P_PROTO_TCP, NULL,
        NULL, NULL) != 0) return 1;
    if (!kc_wait_port((unsigned short)(base + 3U), 1)) return 1;
    rc += expect_int("reorder tcp roundtrip succeeds", 0,
        tcp_roundtrip((unsigned short)(base + 3U), "reorder", out, sizeof(out)));
    rc += expect_string("reorder tcp payload matches", "reorder", out);
    peer_stop(&consumer);
    peer_stop(&publisher);
    echo_stop(&echo);
    index_stop(&index);
    set_env_value("RP2P_DEBUG_STREAM_REORDER_EVERY", NULL);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies a large UDP payload remains byte-exact through the tunnel.
 * @return 0 on success, 1 on failure.
 */
static int case_udp_large(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char data[1400];
    char out[1400];
    int rc;

    base = (unsigned short)(kc_port_base() + 260U);
    rc = 0;
    fill_random(data, sizeof(data));
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_UDP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&publisher, 0, sizeof(publisher));
    publisher.host = KC_TEST_HOST;
    publisher.index_port = (unsigned short)(base + 1U);
    publisher.id = "target";
    publisher.bind_port = (unsigned short)(base + 2U);
    publisher.proto = RP2P_PROTO_UDP;
    if (peer_start(&publisher) != 0) return 1;
    kc_sleep_ms(800U);
    memset(&consumer, 0, sizeof(consumer));
    consumer.host = KC_TEST_HOST;
    consumer.index_port = (unsigned short)(base + 1U);
    consumer.target = "target";
    consumer.bind_port = (unsigned short)(base + 3U);
    consumer.proto = RP2P_PROTO_UDP;
    if (peer_start(&consumer) != 0) return 1;
    kc_sleep_ms(800U);
    rc += expect_int("large udp transfer succeeds", 0,
        udp_large_transfer((unsigned short)(base + 3U), data, sizeof(data),
            out, sizeof(out)));
    peer_stop(&consumer);
    peer_stop(&publisher);
    echo_stop(&echo);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies peer disappearance does not crash index or consumer.
 * @return 0 on success, 1 on failure.
 */
static int case_peer_disappearance(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo;
    kc_peer_t publisher;
    kc_peer_t consumer;
    char out[64];
    int rc;

    base = (unsigned short)(kc_port_base() + 280U);
    rc = 0;
    if (index_start(&index, (unsigned short)(base + 1U)) != 0) return 1;
    if (echo_start(&echo, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0) return 1;
    memset(&publisher, 0, sizeof(publisher));
    publisher.host = KC_TEST_HOST;
    publisher.index_port = (unsigned short)(base + 1U);
    publisher.id = "target";
    publisher.bind_port = (unsigned short)(base + 2U);
    publisher.proto = RP2P_PROTO_TCP;
    if (peer_start(&publisher) != 0) return 1;
    kc_sleep_ms(800U);
    memset(&consumer, 0, sizeof(consumer));
    consumer.host = KC_TEST_HOST;
    consumer.index_port = (unsigned short)(base + 1U);
    consumer.target = "target";
    consumer.bind_port = (unsigned short)(base + 3U);
    consumer.proto = RP2P_PROTO_TCP;
    if (peer_start(&consumer) != 0) return 1;
    if (!kc_wait_port((unsigned short)(base + 3U), 1)) return 1;
    rc += expect_int("peer disappearance preflight succeeds", 0,
        tcp_roundtrip((unsigned short)(base + 3U), "alive", out, sizeof(out)));
    peer_stop(&publisher);
    echo_stop(&echo);
    kc_sleep_ms(500U);
    rc += expect_true("index remains alive after publisher disappearance",
        kc_port_open(index.port));
    peer_stop(&consumer);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies failure cases: no index, unknown target, and full index table.
 * @return 0 on success, 1 on failure.
 */
static int case_failure_cases(void) {
    unsigned short base;
    kc_index_t index;
    rp2p_t *ctx;
    kc_echo_t echo_one;
    kc_echo_t echo_two;
    kc_peer_t peer_one;
    kc_peer_t peer_two;
    int rc;

    base = (unsigned short)(kc_port_base() + 300U);
    rc = 0;
    rc += expect_int("open failure client returns OK", RP2P_OK, rp2p_open(&ctx));
    rc += expect_int("lookup with no index returns network error", RP2P_ENET,
        rp2p_connect(ctx, KC_TEST_HOST, (unsigned short)(base + 1U),
            "consumer", "missing", (unsigned short)(base + 2U)));
    rp2p_close(ctx);
    if (index_start_cfg(&index, (unsigned short)(base + 3U), NULL, NULL, 1, 0) != 0)
        return 1;
    rc += expect_int("open unknown target client returns OK", RP2P_OK,
        rp2p_open(&ctx));
    rc += expect_int("unknown target returns ENOENT", RP2P_ENOENT,
        rp2p_connect(ctx, KC_TEST_HOST, (unsigned short)(base + 3U),
            "consumer", "missing", (unsigned short)(base + 4U)));
    rp2p_close(ctx);
    if (echo_start(&echo_one, RP2P_PROTO_TCP, (unsigned short)(base + 5U)) != 0)
        return 1;
    memset(&peer_one, 0, sizeof(peer_one));
    peer_one.host = KC_TEST_HOST;
    peer_one.index_port = (unsigned short)(base + 3U);
    peer_one.id = "one";
    peer_one.bind_port = (unsigned short)(base + 5U);
    peer_one.proto = RP2P_PROTO_TCP;
    if (peer_start(&peer_one) != 0) return 1;
    kc_sleep_ms(800U);
    if (echo_start(&echo_two, RP2P_PROTO_TCP, (unsigned short)(base + 6U)) != 0)
        return 1;
    memset(&peer_two, 0, sizeof(peer_two));
    peer_two.host = KC_TEST_HOST;
    peer_two.index_port = (unsigned short)(base + 3U);
    peer_two.id = "two";
    peer_two.bind_port = (unsigned short)(base + 6U);
    peer_two.proto = RP2P_PROTO_TCP;
    if (peer_start(&peer_two) != 0) return 1;
    kc_sleep_ms(800U);
    rc += expect_int("second publisher rejected by full index", RP2P_ERROR,
        peer_two.result);
    peer_stop(&peer_two);
    peer_stop(&peer_one);
    echo_stop(&echo_two);
    echo_stop(&echo_one);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies auth, VIP password precedence, and capacity behavior.
 * @return 0 on success, 1 on failure.
 */
static int case_auth_vip_capacity(void) {
    unsigned short base;
    kc_index_t index;
    kc_echo_t echo_one;
    kc_echo_t echo_two;
    kc_peer_t peer_one;
    kc_peer_t peer_two;
    int rc;

    base = (unsigned short)(kc_port_base() + 60U);
    rc = 0;
    memset(&index, 0, sizeof(index));
    index.port = (unsigned short)(base + 1U);
    if (rp2p_open(&index.ctx) != RP2P_OK) return 1;
    rc += expect_int("index set pass returns OK", RP2P_OK, rp2p_set_pass(index.ctx, "global"));
    rc += expect_int("index set vip returns OK", RP2P_OK, rp2p_set_vip(index.ctx, "vip vip-pass",
        NULL, 0));
    rc += expect_int("index set seats returns OK", RP2P_OK, rp2p_set_seats(index.ctx, 1));
    if (thread_start(&index.thread, index_main, &index) != 0) return 1;
    if (!kc_wait_port(index.port, 1)) return 1;
    if (echo_start(&echo_one, RP2P_PROTO_TCP, (unsigned short)(base + 2U)) != 0)
        return 1;
    memset(&peer_one, 0, sizeof(peer_one));
    peer_one.host = KC_TEST_HOST;
    peer_one.index_port = (unsigned short)(base + 1U);
    peer_one.id = "plain";
    peer_one.bind_port = (unsigned short)(base + 2U);
    peer_one.proto = RP2P_PROTO_TCP;
    peer_one.pass = "global";
    if (peer_start(&peer_one) != 0) return 1;
    kc_sleep_ms(1000U);
    rc += expect_true("non-VIP rejected by reserved seat",
        peer_one.result != RP2P_OK);
    peer_stop(&peer_one);
    if (echo_start(&echo_two, RP2P_PROTO_TCP, (unsigned short)(base + 3U)) != 0)
        return 1;
    memset(&peer_two, 0, sizeof(peer_two));
    peer_two.host = KC_TEST_HOST;
    peer_two.index_port = (unsigned short)(base + 1U);
    peer_two.id = "vip";
    peer_two.bind_port = (unsigned short)(base + 3U);
    peer_two.proto = RP2P_PROTO_TCP;
    peer_two.pass = "vip-pass";
    if (peer_start(&peer_two) != 0) return 1;
    kc_sleep_ms(1000U);
    rc += expect_true("VIP with correct pass remains running",
        peer_two.result == RP2P_OK);
    peer_stop(&peer_two);
    echo_stop(&echo_two);
    echo_stop(&echo_one);
    index_stop(&index);
    return rc == 0 ? 0 : 1;
}

/**
 * Verifies that stopping one index context leaves another context running.
 * @return 0 on success, 1 on failure.
 */
static int case_multictx_stop(void) {
    unsigned short base;
    kc_index_t first;
    kc_index_t second;
    int rc;

    base = (unsigned short)(kc_port_base() + 80U);
    rc = 0;
    if (index_start(&first, (unsigned short)(base + 1U)) != 0) return 1;
    if (index_start(&second, (unsigned short)(base + 2U)) != 0) return 1;
    rc += expect_int("stop first index returns OK", RP2P_OK, rp2p_stop(first.ctx));
    rc += expect_int("join first index thread", 0, thread_join(first.thread));
    rc += expect_true("first index port is closed after stop",
        kc_wait_port(first.port, 0));
    rc += expect_true("second index port is still open",
        kc_port_open(second.port));
    rc += expect_int("first index result is OK", RP2P_OK, first.result);
    rc += expect_int("close first index returns OK", RP2P_OK, rp2p_close(first.ctx));
    first.ctx = NULL;
    rc += expect_int("stop second index returns OK", RP2P_OK, rp2p_stop(second.ctx));
    rc += expect_int("join second index thread", 0, thread_join(second.thread));
    rc += expect_int("second index result is OK", RP2P_OK, second.result);
    rc += expect_int("close second index returns OK", RP2P_OK, rp2p_close(second.ctx));
    second.ctx = NULL;
    return rc == 0 ? 0 : 1;
}

/**
 * Runs one librp2p contract test case.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, 1 or 2 on failure.
 */
int main(int argc, char **argv) {
    int rc;

    if (argc != 2) {
        fprintf(stderr, "test case: expected one argument, got %d\n", argc - 1);
        return 2;
    }
    set_env_value("HOME", ".");
    if (kc_socket_start() != 0) return 1;
    if (strcmp(argv[1], "version-strerror") == 0) rc = case_version_strerror();
    else if (strcmp(argv[1], "validators") == 0) rc = case_validators();
    else if (strcmp(argv[1], "options") == 0) rc = case_options();
    else if (strcmp(argv[1], "open-close") == 0) rc = case_open_close();
    else if (strcmp(argv[1], "stop") == 0) rc = case_stop();
    else if (strcmp(argv[1], "setters") == 0) rc = case_setters();
    else if (strcmp(argv[1], "signals") == 0) rc = case_signals();
    else if (strcmp(argv[1], "multictx") == 0) rc = case_multictx();
    else if (strcmp(argv[1], "index-catalog") == 0) rc = case_index_catalog();
    else if (strcmp(argv[1], "tcp-tunnel") == 0) rc = case_tcp_tunnel();
    else if (strcmp(argv[1], "udp-tunnel") == 0) rc = case_udp_tunnel();
    else if (strcmp(argv[1], "index-tcp-only") == 0) rc = case_index_tcp_only();
    else if (strcmp(argv[1], "control-catalog") == 0) rc = case_control_catalog();
    else if (strcmp(argv[1], "protocol-vectors") == 0) rc = case_protocol_vectors();
    else if (strcmp(argv[1], "tcp-concurrent") == 0) rc = case_tcp_concurrent();
    else if (strcmp(argv[1], "tcp-reconnect") == 0) rc = case_tcp_reconnect();
    else if (strcmp(argv[1], "tcp-large") == 0) rc = case_tcp_large();
    else if (strcmp(argv[1], "tcp-loss") == 0) rc = case_tcp_loss();
    else if (strcmp(argv[1], "tcp-reorder") == 0) rc = case_tcp_reorder();
    else if (strcmp(argv[1], "udp-large") == 0) rc = case_udp_large();
    else if (strcmp(argv[1], "peer-disappearance") == 0) rc = case_peer_disappearance();
    else if (strcmp(argv[1], "failure-cases") == 0) rc = case_failure_cases();
    else if (strcmp(argv[1], "auth-vip-capacity") == 0) rc = case_auth_vip_capacity();
    else if (strcmp(argv[1], "multictx-stop") == 0) rc = case_multictx_stop();
    else {
        fprintf(stderr, "unknown test case: %s\n", argv[1]);
        rc = 2;
    }
    kc_socket_stop();
    return rc;
}
