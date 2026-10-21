/* Bounded, allocation-free parser. Shared by the driver and host tests. */
#pragma once
#define NABU_MAX_PARTITIONS 256u
typedef struct _NABU_FW_PART {
    unsigned int bin, sram, size, crc;
} NABU_FW_PART;

static unsigned int NabuLe32(const unsigned char* b)
{
    return (unsigned int)b[0] | ((unsigned int)b[1] << 8) |
        ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24);
}

/* size is the last-byte index; size == 0 denotes a reserved partition. */
static int NabuParseFirmware(const unsigned char* b, unsigned int length,
    NABU_FW_PART* parts, unsigned int capacity, unsigned int* count)
{
    unsigned int end, limit, info, overlay, cascade, found = 0, n, pos, i;
    if (!count) return 0;
    *count = 0;
    if (!b || !parts || length < 0x30u) return 0;
    end = NabuLe32(b);
    cascade = (b[0x20] & 2u) != 0;
    if (end < 0x30u || end > length || (end & 15u)) return 0;
    limit = cascade ? end / 2u : end;
    if (limit < 0x30u || (limit & 15u)) return 0;
    info = (limit - 0x30u) / 16u + cascade;
    overlay = (b[0x28] & 0x10u) ? b[0x28] & 15u : 0u;
    if (info > NABU_MAX_PARTITIONS - 2u - overlay) return 0;
    n = 2u + info + overlay;
    if (n > capacity) return 0;
    for (i = 0; i < n; ++i) {
        NABU_FW_PART* p = &parts[i];
        if (i < 2u) {
            p->bin = NabuLe32(b + i * 12u);
            p->sram = NabuLe32(b + 4u + i * 12u);
            p->size = NabuLe32(b + 8u + i * 12u);
            p->crc = NabuLe32(b + 0x18u + i * 4u);
            if (p->size > 0x1ffffu) return 0; /* CRC length register is 17 bits. */
        } else {
            if (i < 2u + info) {
                pos = found && cascade ? end - 16u : 0x30u + 16u * (i - 2u);
            } else {
                unsigned int offset = 16u * (i - 2u - info);
                if (parts[1].bin > length || offset > length - parts[1].bin) return 0;
                pos = parts[1].bin + offset;
            }
            if (pos > length || length - pos < 16u) return 0;
            p->sram = NabuLe32(b + pos);
            p->size = NabuLe32(b + pos + 4u);
            p->bin = NabuLe32(b + pos + 8u);
            p->crc = NabuLe32(b + pos + 12u);
            if (i < 2u + info && p->bin < end && p->size) found = 1;
        }
        if (!p->size) continue;
        if (p->bin >= length || p->size >= length - p->bin) return 0;
        if (p->sram > 0xffffffu || p->size > 0xffffffu - p->sram) return 0;
    }
    *count = n;
    return 1;
}
