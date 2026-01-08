
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "streambuf.h"

static ptrdiff_t POS(const uint8_t *base, const sbu_t *b) {
    return (ptrdiff_t)(sbu_const_ptr(b) - base);
}
static int LEFT(sbu_t *b) {
    return sbu_left(b);
}

static void dump_bytes(const uint8_t *p, int n) {
    for (int i = 0; i < n; i++) {
        printf("%02X", p[i]);
        if (i + 1 != n) putchar(' ');
    }
}

static void show_state(const char *tag, const uint8_t *base, sbu_t *b) {
    printf("%s POS %td LEFT %d\n", tag, POS(base, b), LEFT(b));
}

static void show_used(const char *tag, const uint8_t *base, const sbu_t *b) {
    ptrdiff_t used = (ptrdiff_t)(sbu_const_ptr(b) - base);
    printf("%s USED %td\n", tag, used);
}

int main(void) {
    uint8_t mem[256];
    memset(mem, 0x00, sizeof(mem));

    sbu_t b;
    sbu_init(&b, mem, mem + (int)sizeof(mem));

    // ---------------- WRITE ----------------
    show_state("W0", mem, &b);

    // fill
    sbu_fill(&b, 0xAA, 4);
    printf("W fill 0xAA x4 -> wrote: ");
    dump_bytes(mem + 0, 4);
    putchar('\n');
    show_state("W1", mem, &b);

    // skip
    sbu_skip(&b, 2);
    printf("W skip 2 -> bytes at skipped area now: ");
    dump_bytes(mem + 4, 2);
    putchar('\n');
    show_state("W2", mem, &b);

    // write_data
    {
        uint8_t d[3] = {0x01, 0x02, 0x03};
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_data(&b, d, 3);
        printf("W write_data 01 02 03 -> wrote: ");
        dump_bytes(mem + p0, 3);
        putchar('\n');
        show_state("W3", mem, &b);
    }

    // write_data_safe
    {
        uint8_t d[3] = {0x01, 0x02, 0x03};
        ptrdiff_t p0 = POS(mem, &b);
        bool ok = sbu_write_data_safe(&b, d, 3);
        printf("W write_data_safe 01 02 03 -> %s", ok ? "WROTE" : "NO SPACE");
        if (ok) {
            printf(" bytes: ");
            dump_bytes(mem + p0, 3);
        }
        putchar('\n');
        show_state("W4", mem, &b);
    }

    // write_u8
    {
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_u8(&b, 0x7F);
        printf("W write_u8 0x7F -> wrote: ");
        dump_bytes(mem + p0, 1);
        putchar('\n');
        show_state("W5", mem, &b);
    }

    // write_i8
    {
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_i8(&b, (int8_t)-5);
        printf("W write_i8 -5 -> wrote(byte): ");
        dump_bytes(mem + p0, 1);
        putchar('\n');
        show_state("W6", mem, &b);
    }

    // write_u16le / be
    {
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_u16le(&b, 0x1122);
        printf("W write_u16le 0x1122 -> wrote: ");
        dump_bytes(mem + p0, 2);
        putchar('\n');
        show_state("W7", mem, &b);
    }
    {
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_u16be(&b, 0x3344);
        printf("W write_u16be 0x3344 -> wrote: ");
        dump_bytes(mem + p0, 2);
        putchar('\n');
        show_state("W8", mem, &b);
    }

    // write_u32le / be
    {
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_u32le(&b, 0xA1B2C3D4u);
        printf("W write_u32le 0xA1B2C3D4 -> wrote: ");
        dump_bytes(mem + p0, 4);
        putchar('\n');
        show_state("W9", mem, &b);
    }
    {
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_u32be(&b, 0x01020304u);
        printf("W write_u32be 0x01020304 -> wrote: ");
        dump_bytes(mem + p0, 4);
        putchar('\n');
        show_state("W10", mem, &b);
    }

    // write strings
    // Важно: формат write_string зависит от твоей реализации.
    // Мы не гадаем “как читать строку назад”, поэтому СРАЗУ показываем байты, которые реально легли в буфер.
    {
        const char *s = "HELLO";
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_string(&b, s);
        ptrdiff_t p1 = POS(mem, &b);
        printf("W write_string \"%s\" -> wrote %td bytes: ", s, (p1 - p0));
        dump_bytes(mem + p0, (int)(p1 - p0));
        putchar('\n');
        show_state("W11", mem, &b);
    }
    {
        const char *s = "PSCL";
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_string_pscl(&b, s);
        ptrdiff_t p1 = POS(mem, &b);
        printf("W write_string_pscl \"%s\" -> wrote %td bytes: ", s, (p1 - p0));
        dump_bytes(mem + p0, (int)(p1 - p0));
        putchar('\n');
        show_state("W12", mem, &b);
    }
    {
        const char *s = "ZERO";
        ptrdiff_t p0 = POS(mem, &b);
        sbu_write_string_zero(&b, s);
        ptrdiff_t p1 = POS(mem, &b);
        printf("W write_string_zero \"%s\" -> wrote %td bytes: ", s, (p1 - p0));
        dump_bytes(mem + p0, (int)(p1 - p0));
        putchar('\n');
        show_state("W13", mem, &b);
    }

    // сколько всего записали
    show_used("W_END", mem, &b);

    // ---------------- SWITCH TO READ ----------------
    // после записи: base=mem; ptr становится mem; end становится текущий "used" (как у тебя реализовано)
    sbu_switch_to_reader(&b, mem);
    show_state("R0", mem, &b);

    // ---------------- READ ----------------
    // читаем ровно то, что писали: тут нет “4 раза read_u8 непонятно зачем” —
    // мы пишем “R read fill(4 bytes)” и показываем массив из 4 байт.

    // read fill bytes (4)
    {
        uint8_t x[4];
        ptrdiff_t p0 = POS(mem, &b);
        sbu_read_data(&b, x, 4);
        printf("R read_data 4 (was fill) @%td -> ", p0);
        dump_bytes(x, 4);
        putchar('\n');
        show_state("R1", mem, &b);
    }

    // read skip 2
    {
        ptrdiff_t p0 = POS(mem, &b);
        sbu_skip(&b, 2);
        printf("R skip 2 @%td -> now POS %td\n", p0, POS(mem, &b));
        show_state("R2", mem, &b);
    }

    // read_data (3)
    {
        uint8_t x[3];
        ptrdiff_t p0 = POS(mem, &b);
        sbu_read_data(&b, x, 3);
        printf("R read_data 3 (was write_data) @%td -> ", p0);
        dump_bytes(x, 3);
        putchar('\n');
        show_state("R3", mem, &b);
    }

    // read_data_safe (3)
    {
        uint8_t x[3];
        ptrdiff_t p0 = POS(mem, &b);
        bool ok = sbu_read_data_safe(&b, x, 3);
        printf("R read_data_safe 3 (was write_data_safe) @%td -> %s", p0, ok ? "OK " : "FAIL");
        if (ok) dump_bytes(x, 3);
        putchar('\n');
        show_state("R4", mem, &b);
    }

    // read_u8
    {
        ptrdiff_t p0 = POS(mem, &b);
        uint8_t v = sbu_read_u8(&b);
        printf("R read_u8 (was write_u8) @%td -> %02X\n", p0, v);
        show_state("R5", mem, &b);
    }

    // read_i8
    {
        ptrdiff_t p0 = POS(mem, &b);
        int8_t v = sbu_read_i8(&b);
        printf("R read_i8 (was write_i8) @%td -> %d\n", p0, (int)v);
        show_state("R6", mem, &b);
    }

    // read_u16le / be
    {
        ptrdiff_t p0 = POS(mem, &b);
        uint16_t v = sbu_read_u16le(&b);
        printf("R read_u16le (was write_u16le) @%td -> %04X\n", p0, v);
        show_state("R7", mem, &b);
    }
    {
        ptrdiff_t p0 = POS(mem, &b);
        uint16_t v = sbu_read_u16be(&b);
        printf("R read_u16be (was write_u16be) @%td -> %04X\n", p0, v);
        show_state("R8", mem, &b);
    }

    // read_u32le / be
    {
        ptrdiff_t p0 = POS(mem, &b);
        uint32_t v = sbu_read_u32le(&b);
        printf("R read_u32le (was write_u32le) @%td -> %08X\n", p0, v);
        show_state("R9", mem, &b);
    }
    {
        ptrdiff_t p0 = POS(mem, &b);
        uint32_t v = sbu_read_u32be(&b);
        printf("R read_u32be (was write_u32be) @%td -> %08X\n", p0, v);
        show_state("R10", mem, &b);
    }

    // read i16 / i32: в заголовке есть read_i16/read_i32 — покажем их на тех же данных:
    // Для этого создаём маленький отдельный буфер и туда кладём конкретные байты, потом читаем signed.
    {
        uint8_t t[16];
        sbu_t x;
        sbu_init(&x, t, t + (int)sizeof(t));
        sbu_write_u16le(&x, (uint16_t)0xFF9C);         // -100 if i16le
        sbu_write_u16be(&x, (uint16_t)0xFF9C);         // -100 if i16be
        sbu_write_u32le(&x, (uint32_t)0xFFFFFF9C);     // -100 if i32le
        sbu_write_u32be(&x, (uint32_t)0xFFFFFF9C);     // -100 if i32be
        sbu_switch_to_reader(&x, t);

        printf("R signed i16le -> %d\n", (int)sbu_read_i16le(&x));
        printf("R signed i16be -> %d\n", (int)sbu_read_i16be(&x));
        printf("R signed i32le -> %d\n", (int)sbu_read_i32le(&x));
        printf("R signed i32be -> %d\n", (int)sbu_read_i32be(&x));
    }

    // strings: не “угадываем” формат, а показываем, сколько байт осталось в буфере под строки
    // и читаем ровно те байты, которые реально записались (по факту: used diff в WRITE секции уже был показан).
    // Здесь просто читаем “всё, что осталось”, чтобы было видно: “да, строки были в буфере и мы их прочитали”.
    {
        int left = sbu_left(&b);
        uint8_t rest[128];
        int n = (int)MIN(left, (int)sizeof(rest));
        ptrdiff_t p0 = POS(mem, &b);

        bool ok = sbu_read_data_safe(&b, rest, n);
        printf("R read_rest_strings @%td -> %s %d bytes: ", p0, ok ? "OK" : "FAIL", n);
        if (ok) dump_bytes(rest, n);
        putchar('\n');
        show_state("R_STR_END", mem, &b);
    }

    // ---------------- SAFE API: покажем “успех” и “фейл” на одном и том же типе ----------------
    {
        uint8_t t[4];
        sbu_t x;
        sbu_init(&x, t, t + 4);
        sbu_write_u32le(&x, 0xDEADBEEFu);
        sbu_switch_to_reader(&x, t);

        uint32_t v = 0;
        bool ok1 = sbu_read_u32le_safe(&v, &x);
        printf("SAFE read_u32le_safe -> %s %08X\n", ok1 ? "OK" : "FAIL", v);

        bool ok2 = sbu_read_u32le_safe(&v, &x); // второй раз уже нет данных
        printf("SAFE read_u32le_safe again -> %s\n", ok2 ? "OK" : "FAIL");
    }

    {
        uint8_t t[4];
        sbu_t x;
        sbu_init(&x, t, t + 4);
        uint8_t d[3] = {9,8,7};
        bool ok1 = sbu_write_data_safe(&x, d, 3);
        bool ok2 = sbu_write_data_safe(&x, d, 3);
        printf("SAFE write_data_safe 3 -> %s\n", ok1 ? "OK" : "FAIL");
        printf("SAFE write_data_safe 3 again -> %s\n", ok2 ? "OK" : "FAIL");
    }

    return 0;
}
