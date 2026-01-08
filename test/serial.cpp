// test/serial.cpp
// Тест uSerial на "замкнутом" порту (RX<->TX loopback).
// Запуск: ./serial_test /dev/ttyUSB0

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#ifndef ARDUINO
#include <unistd.h>
#endif

#include "stream.h"

static void hexdump(const char *prefix, const uint8_t *data, size_t len)
{
    if (prefix)
        std::printf("%s", prefix);
    for (size_t i = 0; i < len; i++)
        std::printf("%02X%s", data[i], (i + 1 == len) ? "" : " ");
    std::printf("\n");
}

static std::string printable(const uint8_t *data, size_t len)
{
    std::string s;
    s.reserve(len);
    for (size_t i = 0; i < len; i++)
    {
        uint8_t c = data[i];
        if (c >= 32 && c <= 126)
            s.push_back((char)c);
        else
            s.push_back('.');
    }
    return s;
}

static bool echo_test_basic(uSerial &ser, unsigned long baud, int timeout_ms)
{
    // Шаблон без \r\n — чтобы исключить преобразования на стороне терминала/драйвера
    const uint8_t tx[] = {0x55, 0xAA, 0x00, 0xFF, 0x11, 0x22, 0x33, 0x44, 0x77, 0x88};
    uint8_t rx[sizeof(tx)] = {0};

    ser.flush();

    // Демонстрация write(buffer,len)
    size_t w = ser.write(tx, sizeof(tx));
    if (w != sizeof(tx))
    {
        std::printf("  [baud=%lu] write: expected %zu, wrote %zu\n", baud, sizeof(tx), w);
        return false;
    }

    // Демонстрация poll/available/readBytes
    if (!ser.poll(timeout_ms))
    {
        std::printf("  [baud=%lu] poll: timeout waiting data\n", baud);
        return false;
    }

    bool ok = ser.readBytes(rx, sizeof(rx), timeout_ms);
    if (!ok)
    {
        std::printf("  [baud=%lu] readBytes: timeout / partial read\n", baud);
        std::printf("    available() now = %d\n", ser.available());
        return false;
    }

    bool match = (std::memcmp(tx, rx, sizeof(tx)) == 0);
    std::printf("  [baud=%lu] basic echo: %s\n", baud, match ? "OK" : "FAIL");
    if (!match)
    {
        hexdump("    TX: ", tx, sizeof(tx));
        hexdump("    RX: ", rx, sizeof(rx));
    }
    return match;
}

static bool echo_test_text(uSerial &ser, unsigned long baud, int timeout_ms)
{
    // Демонстрация writeString/println
    const char *msg = "uSerial loopback demo";
    ser.flush();
    ser.writeString(msg);
    ser.println(""); // добавит \r\n

    // Ожидаем прочитать msg + "\r\n"
    std::string expected = std::string(msg) + "\r\n";
    std::vector<uint8_t> rx(expected.size(), 0);

    if (!ser.poll(timeout_ms))
    {
        std::printf("  [baud=%lu] poll: timeout (text)\n", baud);
        return false;
    }

    bool ok = ser.readBytes(rx.data(), rx.size(), timeout_ms);
    if (!ok)
    {
        std::printf("  [baud=%lu] readBytes: timeout / partial read (text)\n", baud);
        return false;
    }

    bool match = (std::memcmp(rx.data(), expected.data(), expected.size()) == 0);
    std::printf("  [baud=%lu] text echo: %s  (\"%s\")\n",
                baud,
                match ? "OK" : "FAIL",
                printable(rx.data(), rx.size()).c_str());

    if (!match)
    {
        hexdump("    RX(hex): ", rx.data(), rx.size());
        std::printf("    expected: \"%s\"\n", expected.c_str());
    }
    return match;
}

static bool echo_test_sbuf(uSerial &ser, unsigned long baud, int timeout_ms)
{
    // Демонстрация writeBuf/readBuf на sbu_t.
    // Схема:
    //   - Подготовим tx буфер в sbuf
    //   - writeBuf отправит всё и продвинет ptr до конца
    //   - Затем читаем в rx sbuf, пока не наберём нужный объём или не истечёт timeout
    uint8_t tx_raw[] = {'S', 'B', 'U', 'F', '_', '0', '1', '2', '3', '4', 0x00, 0xFE, 0xEF};
    uint8_t rx_raw[sizeof(tx_raw)] = {0};

    sbu_t tx{};
    sbu_t rx{};

    tx.ptr = tx_raw;
    tx.end = tx_raw + sizeof(tx_raw);

    rx.ptr = rx_raw;
    rx.end = rx_raw + sizeof(rx_raw);

    ser.flush();

    bool w_ok = ser.writeBuf(&tx);
    if (!w_ok)
    {
        std::printf("  [baud=%lu] writeBuf: FAIL (wrote partial or none)\n", baud);
        return false;
    }

    // Читать будем "порциями", демонстрируя readBuf (он читает сколько доступно и сколько осталось).
    uint32_t start = millis();
    while (sbu_left(&rx) > 0)
    {
        if (ser.readBuf(&rx))
        {
            // что-то прочитали
        }
        else
        {
            if ((int)(millis() - start) > timeout_ms)
                break;
#ifndef ARDUINO
            usleep(1000);
#endif
        }
    }

    bool full = (sbu_left(&rx) == 0);
    if (!full)
    {
        std::printf(
            "  [baud=%lu] readBuf: timeout, remaining=%zu\n",
            baud,
            static_cast<size_t>(sbu_left(&rx)));

        hexdump("    partial RX: ", rx_raw, (size_t)(rx.ptr - rx_raw));
        return false;
    }

    bool match = (std::memcmp(tx_raw, rx_raw, sizeof(tx_raw)) == 0);
    std::printf("  [baud=%lu] sbuf echo: %s\n", baud, match ? "OK" : "FAIL");
    if (!match)
    {
        hexdump("    TX: ", tx_raw, sizeof(tx_raw));
        hexdump("    RX: ", rx_raw, sizeof(rx_raw));
    }
    return match;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "Usage: %s <serial_port>\n", argv[0]);
        std::fprintf(stderr, "Example: %s /dev/ttyUSB0\n", argv[0]);
        return 2;
    }

    const char *port = argv[1];

    // Набор скоростей — можно расширить
    const std::vector<unsigned long> baudrates = {
        9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 4000000, 420000};

    std::printf("uSerial loopback test\n");
    std::printf("Port: %s\n", port);
    std::printf("NOTE: RX and TX must be physically connected (loopback).\n\n");

    int total = 0;
    int passed = 0;

    for (auto baud : baudrates)
    {
        total++;
        std::printf("=== Baudrate: %lu ===\n", baud);

        uSerial ser;
        if (!ser.open(port, baud))
        {
            std::printf("  open: FAIL (baud=%lu)\n\n", baud);
            continue;
        }

        std::printf("  isOpen()=%s, isWritable()=%s, isReadable()=%s\n",
                    ser.isOpen() ? "true" : "false",
                    ser.isWritable() ? "true" : "false",
                    ser.isReadable() ? "true" : "false");

        // (опционально) низкая задержка, если реализовано
        ser.setLowLatency(true);

        const int timeout_ms = 800;

        bool ok1 = echo_test_basic(ser, baud, timeout_ms);
        bool ok2 = echo_test_text(ser, baud, timeout_ms);
        bool ok3 = echo_test_sbuf(ser, baud, timeout_ms);

        bool ok = ok1 && ok2 && ok3;

        ser.close();
        std::printf("  close() done\n");
        std::printf("Result @ %lu: %s\n\n", baud, ok ? "PASS" : "FAIL");

        if (ok)
            passed++;
    }

    std::printf("Summary: %d/%d baudrates passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
