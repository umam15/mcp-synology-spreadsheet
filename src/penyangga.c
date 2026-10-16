#include "penyangga.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void penyangga_siapkan(penyangga_t *penyangga)
{
    penyangga->data = NULL;
    penyangga->panjang = 0;
    penyangga->kapasitas = 0;
    penyangga->gagal = 0;
}

void penyangga_bebaskan(penyangga_t *penyangga)
{
    free(penyangga->data);
    penyangga_siapkan(penyangga);
}

void penyangga_kosongkan(penyangga_t *penyangga)
{
    penyangga->panjang = 0;
    penyangga->gagal = 0;
    if (penyangga->data != NULL) {
        penyangga->data[0] = '\0';
    }
}

static int pastikan_kapasitas(penyangga_t *penyangga, size_t tambahan)
{
    size_t butuh;
    size_t baru;
    char *data_baru;

    if (penyangga->gagal) {
        return 0;
    }
    if (tambahan > BATAS_PENYANGGA_BYTE ||
        penyangga->panjang > BATAS_PENYANGGA_BYTE - tambahan) {
        penyangga->gagal = 1;
        return 0;
    }
    butuh = penyangga->panjang + tambahan + 1;
    if (butuh <= penyangga->kapasitas) {
        return 1;
    }
    baru = penyangga->kapasitas != 0 ? penyangga->kapasitas : 256;
    while (baru < butuh) {
        baru *= 2;
    }
    data_baru = realloc(penyangga->data, baru);
    if (data_baru == NULL) {
        penyangga->gagal = 1;
        return 0;
    }
    penyangga->data = data_baru;
    penyangga->kapasitas = baru;
    return 1;
}

int penyangga_tambah(penyangga_t *penyangga, const char *data, size_t panjang)
{
    if (!pastikan_kapasitas(penyangga, panjang)) {
        return 0;
    }
    if (panjang > 0) {
        memcpy(penyangga->data + penyangga->panjang, data, panjang);
    }
    penyangga->panjang += panjang;
    penyangga->data[penyangga->panjang] = '\0';
    return 1;
}

int penyangga_tambah_teks(penyangga_t *penyangga, const char *teks)
{
    return penyangga_tambah(penyangga, teks, strlen(teks));
}

int penyangga_tambah_format(penyangga_t *penyangga, const char *format, ...)
{
    va_list argumen;
    va_list salinan;
    int n;

    va_start(argumen, format);
    va_copy(salinan, argumen);
    n = vsnprintf(NULL, 0, format, argumen);
    va_end(argumen);
    if (n < 0 || !pastikan_kapasitas(penyangga, (size_t)n)) {
        va_end(salinan);
        penyangga->gagal = 1;
        return 0;
    }
    vsnprintf(penyangga->data + penyangga->panjang, (size_t)n + 1, format, salinan);
    va_end(salinan);
    penyangga->panjang += (size_t)n;
    return 1;
}

/* Panjang urutan UTF-8 valid di awal s, atau 0 bila tidak valid. */
static size_t panjang_utf8_valid(const unsigned char *s, size_t sisa)
{
    unsigned char c = s[0];

    if (c < 0x80) {
        return 1;
    }
    if (c >= 0xC2 && c <= 0xDF) {
        return (sisa >= 2 && (s[1] & 0xC0) == 0x80) ? 2 : 0;
    }
    if (c >= 0xE0 && c <= 0xEF) {
        if (sisa < 3 || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) {
            return 0;
        }
        if (c == 0xE0 && s[1] < 0xA0) {
            return 0;
        }
        if (c == 0xED && s[1] >= 0xA0) {
            return 0; /* pengganti (surrogate) */
        }
        return 3;
    }
    if (c >= 0xF0 && c <= 0xF4) {
        if (sisa < 4 || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 ||
            (s[3] & 0xC0) != 0x80) {
            return 0;
        }
        if (c == 0xF0 && s[1] < 0x90) {
            return 0;
        }
        if (c == 0xF4 && s[1] >= 0x90) {
            return 0;
        }
        return 4;
    }
    return 0;
}

int penyangga_tambah_teks_json(penyangga_t *penyangga, const char *teks, size_t panjang)
{
    const unsigned char *s = (const unsigned char *)teks;
    size_t i = 0;

    if (!penyangga_tambah(penyangga, "\"", 1)) {
        return 0;
    }
    while (i < panjang) {
        unsigned char c = s[i];
        if (c < 0x80) {
            switch (c) {
            case '"':
                penyangga_tambah(penyangga, "\\\"", 2);
                break;
            case '\\':
                penyangga_tambah(penyangga, "\\\\", 2);
                break;
            case '\n':
                penyangga_tambah(penyangga, "\\n", 2);
                break;
            case '\r':
                penyangga_tambah(penyangga, "\\r", 2);
                break;
            case '\t':
                penyangga_tambah(penyangga, "\\t", 2);
                break;
            case '\b':
                penyangga_tambah(penyangga, "\\b", 2);
                break;
            case '\f':
                penyangga_tambah(penyangga, "\\f", 2);
                break;
            default:
                if (c < 0x20) {
                    penyangga_tambah_format(penyangga, "\\u%04x", (unsigned)c);
                } else {
                    penyangga_tambah(penyangga, (const char *)&s[i], 1);
                }
            }
            i++;
        } else {
            size_t n = panjang_utf8_valid(&s[i], panjang - i);
            if (n == 0) {
                penyangga_tambah(penyangga, "\\ufffd", 6);
                i++;
            } else {
                penyangga_tambah(penyangga, (const char *)&s[i], n);
                i += n;
            }
        }
    }
    return penyangga_tambah(penyangga, "\"", 1);
}

const char *penyangga_isi(const penyangga_t *penyangga)
{
    return penyangga->data != NULL ? penyangga->data : "";
}
