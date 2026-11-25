#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>

#define BAUDRATE B115200
#define FRAME_SIZE 8
#define RESPONSE_TIMEOUT_MS 500
#define INTERBYTE_TIMEOUT_MS 150
#define MAX_RESPONSE 256

static void print_bytes(const char *label,
                        const uint8_t *data,
                        size_t length)
{
    printf("%s (%zu bytes):\n", label, length);

    printf("HEX:     ");
    for (size_t i = 0; i < length; i++)
        printf("%02X ", data[i]);

    printf("\nDECIMAL: ");
    for (size_t i = 0; i < length; i++)
        printf("%u ", data[i]);

    printf("\n");
}

static int configure_serial(int fd)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return -1;
    }

    cfmakeraw(&tty);

    if (cfsetispeed(&tty, BAUDRATE) != 0 ||
        cfsetospeed(&tty, BAUDRATE) != 0) {
        perror("Setting baud rate");
        return -1;
    }

    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    tty.c_cflag |= CS8 | CLOCAL | CREAD;

#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

static int write_all(int fd,
                     const uint8_t *data,
                     size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = write(fd, data + sent, length - sent);

        if (n < 0) {
            if (errno == EINTR)
                continue;

            perror("Serial write");
            return -1;
        }

        if (n == 0) {
            fprintf(stderr, "Serial write returned zero\n");
            return -1;
        }

        sent += (size_t)n;
    }

    return 0;
}

static ssize_t read_response(int fd,
                             uint8_t *buffer,
                             size_t capacity)
{
    size_t total = 0;

    while (total < capacity) {
        struct pollfd pfd = {
            .fd = fd,
            .events = POLLIN
        };

        int timeout = total == 0
                    ? RESPONSE_TIMEOUT_MS
                    : INTERBYTE_TIMEOUT_MS;

        int result = poll(&pfd, 1, timeout);

        if (result < 0) {
            if (errno == EINTR)
                continue;

            perror("poll");
            return -1;
        }

        if (result == 0)
            break;

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr,
                    "Serial port error: revents=0x%X\n",
                    pfd.revents);
            break;
        }

        if (pfd.revents & POLLIN) {
            ssize_t n = read(fd,
                             buffer + total,
                             capacity - total);

            if (n < 0) {
                if (errno == EINTR)
                    continue;

                perror("Serial read");
                return -1;
            }

            if (n > 0)
                total += (size_t)n;
        }
    }

    return (ssize_t)total;
}

int main(int argc, char *argv[])
{
    const char *port = "/dev/ttyUSB0";
    uint16_t address = 1;
    int send_enabled = 0;

    if (argc >= 2)
        port = argv[1];

    if (argc >= 3) {
        char *end = NULL;
        long value = strtol(argv[2], &end, 10);

        if (end == argv[2] || *end != '\0' ||
            value < 1 || value > 255) {
            fprintf(stderr, "Valid register range: 1 to 255\n");
            return 1;
        }

        address = (uint16_t)value;
    }

    if (argc >= 4 && strcmp(argv[3], "--send") == 0)
        send_enabled = 1;

    if (argc > 4 || (argc == 4 && !send_enabled)) {
        fprintf(stderr,
                "Usage: %s [port] [register] [--send]\n",
                argv[0]);
        return 1;
    }

    uint8_t request[FRAME_SIZE] = {
        0x10,
        (uint8_t)((address >> 8) & 0xFF),
        (uint8_t)(address & 0xFF),
        0x00, 0x00, 0x00, 0x00,
        0x55
    };

    printf("\n====================================\n");
    printf(" RXN X3 CONFIGURATION BANK TEST\n");
    printf("====================================\n");

    printf("Port:       %s\n", port);
    printf("Baud rate:  115200\n");
    printf("Format:     8N1\n");
    printf("Bank:       Configuration (0x10)\n");
    printf("Register:   %u\n", address);
    printf("Access:     Read-only test utility\n");

    print_bytes("TX REQUEST", request, FRAME_SIZE);

    if (!send_enabled) {
        printf("\nDRY RUN: No data transmitted.\n");
        printf("Use --send to transmit the request.\n");
        return 0;
    }

    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0) {
        perror("Opening serial port");
        return 1;
    }

    if (configure_serial(fd) != 0) {
        close(fd);
        return 1;
    }

    if (tcflush(fd, TCIFLUSH) != 0) {
        perror("tcflush");
        close(fd);
        return 1;
    }

    printf("\nSending read-only request...\n");

    if (write_all(fd, request, FRAME_SIZE) != 0) {
        close(fd);
        return 1;
    }

    if (tcdrain(fd) != 0) {
        perror("tcdrain");
        close(fd);
        return 1;
    }

    printf("TX completed.\n");
    printf("Waiting for response...\n");

    uint8_t response[MAX_RESPONSE];

    ssize_t received = read_response(
        fd, response, sizeof(response));

    close(fd);

    if (received < 0)
        return 1;

    if (received == 0) {
        printf("\nTIMEOUT: No response received.\n");
        return 2;
    }

    printf("\n====================================\n");
    printf(" RESPONSE\n");
    printf("====================================\n");

    print_bytes("RX RESPONSE", response, (size_t)received);

    printf("\nACK ANALYSIS\n");

    if (response[0] == 0x01)
        printf("ACK = 1: Success indication.\n");
    else
        printf("First response byte: 0x%02X\n", response[0]);

    printf("\nRESPONSE LENGTH\n");
    printf("Received: %zd bytes\n", received);

    if (address >= 1 && address <= 8)
        printf("Manual type: char[16]; expected length: "
               "18 bytes, subject to verification.\n");
    else
        printf("Check the register data type in the manual. "
               "No value decoding performed.\n");

    printf("\nRAW DATA ONLY\n");
    printf("Response checksum and data interpretation "
           "are not yet verified.\n");

    return 0;
}
