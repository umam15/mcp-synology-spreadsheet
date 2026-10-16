#include "json.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    dokumen_json_t *dok;
    size_t pos;
} pengurai_t;

#define ADALAH_ANGKA(c) ((c) >= '0' && (c) <= '9')

static int urai_nilai(pengurai_t *p, int kedalaman);

static int gagal(pengurai_t *p, const char *pesan)
{
    if (p->dok->galat[0] == '\0') {
        snprintf(p->dok->galat, sizeof p->dok->galat, "%s (posisi %zu)", pesan, p->pos);
    }
    return -1;
}

static int tambah_simpul(dokumen_json_t *dok, jenis_json_t jenis, size_t awal)
{
    simpul_json_t *s;

    if (dok->jumlah >= JSON_SIMPUL_MAKS) {
        return -1;
    }
    if (dok->jumlah == dok->kapasitas) {
        int kapasitas_baru = dok->kapasitas != 0 ? dok->kapasitas * 2 : 32;
        simpul_json_t *baru = realloc(dok->simpul, (size_t)kapasitas_baru * sizeof *baru);
        if (baru == NULL) {
            return -1;
        }
        dok->simpul = baru;
        dok->kapasitas = kapasitas_baru;
    }
    s = &dok->simpul[dok->jumlah];
    memset(s, 0, sizeof *s);
    s->jenis = jenis;
    s->awal = awal;
    s->akhir = awal;
    s->anak_pertama = -1;
    s->saudara_berikut = -1;
    return dok->jumlah++;
}

static void lewati_spasi(pengurai_t *p)
{
    while (p->pos < p->dok->panjang) {
        char c = p->dok->sumber[p->pos];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            p->pos++;
        } else {
            break;
        }
    }
}

static int nilai_heks(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static int urai_teks(pengurai_t *p, size_t *isi_awal, size_t *isi_akhir)
{
    const char *s = p->dok->sumber;
    size_t n = p->dok->panjang;

    p->pos++; /* kutip pembuka */
    *isi_awal = p->pos;
    for (;;) {
        unsigned char c;
        if (p->pos >= n) {
            return gagal(p, "teks tidak ditutup");
        }
        c = (unsigned char)s[p->pos];
        if (c == '"') {
            break;
        }
        if (c < 0x20) {
            return gagal(p, "karakter kendali di dalam teks");
        }
        if (c == '\\') {
            p->pos++;
            if (p->pos >= n) {
                return gagal(p, "escape tidak lengkap");
            }
            switch (s[p->pos]) {
            case '"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
                p->pos++;
                break;
            case 'u': {
                int k;
                p->pos++;
                for (k = 0; k < 4; k++) {
                    if (p->pos >= n || nilai_heks(s[p->pos]) < 0) {
                        return gagal(p, "urutan \\u tidak valid");
                    }
                    p->pos++;
                }
                break;
            }
            default:
                return gagal(p, "escape tidak valid");
            }
        } else {
            p->pos++;
        }
    }
    *isi_akhir = p->pos;
    p->pos++; /* kutip penutup */
    return 0;
}

static int urai_angka(pengurai_t *p)
{
    const char *s = p->dok->sumber;
    size_t n = p->dok->panjang;
    size_t i = p->pos;

    if (i < n && s[i] == '-') {
        i++;
    }
    if (i >= n) {
        return gagal(p, "angka tidak lengkap");
    }
    if (s[i] == '0') {
        i++;
    } else if (s[i] >= '1' && s[i] <= '9') {
        while (i < n && ADALAH_ANGKA(s[i])) {
            i++;
        }
    } else {
        return gagal(p, "nilai tidak valid");
    }
    if (i < n && s[i] == '.') {
        i++;
        if (i >= n || !ADALAH_ANGKA(s[i])) {
            return gagal(p, "pecahan tidak valid");
        }
        while (i < n && ADALAH_ANGKA(s[i])) {
            i++;
        }
    }
    if (i < n && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        if (i < n && (s[i] == '+' || s[i] == '-')) {
            i++;
        }
        if (i >= n || !ADALAH_ANGKA(s[i])) {
            return gagal(p, "eksponen tidak valid");
        }
        while (i < n && ADALAH_ANGKA(s[i])) {
            i++;
        }
    }
    p->pos = i;
    return 0;
}

static int cocok_literal(pengurai_t *p, const char *literal)
{
    size_t panjang = strlen(literal);

    if (p->dok->panjang - p->pos >= panjang &&
        memcmp(p->dok->sumber + p->pos, literal, panjang) == 0) {
        p->pos += panjang;
        return 0;
    }
    return gagal(p, "nilai tidak valid");
}

static int urai_wadah(pengurai_t *p, int kedalaman, int objek)
{
    const char *s = p->dok->sumber;
    size_t n = p->dok->panjang;
    char penutup = objek ? '}' : ']';
    int idx = tambah_simpul(p->dok, objek ? JSON_OBJEK : JSON_LARIK, p->pos);
    int terakhir = -1;

    if (idx < 0) {
        return gagal(p, "simpul terlalu banyak atau memori habis");
    }
    p->pos++;
    lewati_spasi(p);
    if (p->pos < n && s[p->pos] == penutup) {
        p->pos++;
        p->dok->simpul[idx].akhir = p->pos;
        return idx;
    }
    for (;;) {
        size_t kunci_awal = 0;
        size_t kunci_akhir = 0;
        int anak;

        lewati_spasi(p);
        if (objek) {
            if (p->pos >= n || s[p->pos] != '"') {
                return gagal(p, "kunci objek harus teks");
            }
            if (urai_teks(p, &kunci_awal, &kunci_akhir) < 0) {
                return -1;
            }
            lewati_spasi(p);
            if (p->pos >= n || s[p->pos] != ':') {
                return gagal(p, "':' diharapkan");
            }
            p->pos++;
        }
        anak = urai_nilai(p, kedalaman + 1);
        if (anak < 0) {
            return -1;
        }
        if (objek) {
            p->dok->simpul[anak].punya_kunci = 1;
            p->dok->simpul[anak].kunci_awal = kunci_awal;
            p->dok->simpul[anak].kunci_akhir = kunci_akhir;
        }
        if (terakhir < 0) {
            p->dok->simpul[idx].anak_pertama = anak;
        } else {
            p->dok->simpul[terakhir].saudara_berikut = anak;
        }
        terakhir = anak;
        lewati_spasi(p);
        if (p->pos >= n) {
            return gagal(p, "akhir data tak terduga");
        }
        if (s[p->pos] == ',') {
            p->pos++;
            continue;
        }
        if (s[p->pos] == penutup) {
            p->pos++;
            break;
        }
        return gagal(p, "',' atau penutup diharapkan");
    }
    p->dok->simpul[idx].akhir = p->pos;
    return idx;
}

static int urai_nilai(pengurai_t *p, int kedalaman)
{
    const char *s = p->dok->sumber;
    int idx;
    char c;

    if (kedalaman > JSON_KEDALAMAN_MAKS) {
        return gagal(p, "bersarang terlalu dalam");
    }
    lewati_spasi(p);
    if (p->pos >= p->dok->panjang) {
        return gagal(p, "nilai diharapkan");
    }
    c = s[p->pos];
    switch (c) {
    case '{':
        return urai_wadah(p, kedalaman, 1);
    case '[':
        return urai_wadah(p, kedalaman, 0);
    case '"': {
        size_t isi_awal;
        size_t isi_akhir;
        idx = tambah_simpul(p->dok, JSON_TEKS, p->pos);
        if (idx < 0) {
            return gagal(p, "simpul terlalu banyak atau memori habis");
        }
        if (urai_teks(p, &isi_awal, &isi_akhir) < 0) {
            return -1;
        }
        p->dok->simpul[idx].akhir = p->pos;
        return idx;
    }
    case 't':
    case 'f':
    case 'n': {
        jenis_json_t jenis = (c == 'n') ? JSON_KOSONG : JSON_LOGIKA;
        idx = tambah_simpul(p->dok, jenis, p->pos);
        if (idx < 0) {
            return gagal(p, "simpul terlalu banyak atau memori habis");
        }
        if (cocok_literal(p, c == 't' ? "true" : (c == 'f' ? "false" : "null")) < 0) {
            return -1;
        }
        p->dok->simpul[idx].akhir = p->pos;
        return idx;
    }
    default:
        if (c == '-' || ADALAH_ANGKA(c)) {
            idx = tambah_simpul(p->dok, JSON_ANGKA, p->pos);
            if (idx < 0) {
                return gagal(p, "simpul terlalu banyak atau memori habis");
            }
            if (urai_angka(p) < 0) {
                return -1;
            }
            p->dok->simpul[idx].akhir = p->pos;
            return idx;
        }
        return gagal(p, "nilai tidak valid");
    }
}

int json_urai(dokumen_json_t *dok, const char *sumber, size_t panjang)
{
    pengurai_t p;

    memset(dok, 0, sizeof *dok);
    dok->sumber = sumber;
    dok->panjang = panjang;
    p.dok = dok;
    p.pos = 0;
    if (urai_nilai(&p, 0) < 0) {
        json_bebaskan(dok);
        return 0;
    }
    lewati_spasi(&p);
    if (p.pos != panjang) {
        gagal(&p, "data tambahan setelah nilai JSON");
        json_bebaskan(dok);
        return 0;
    }
    return 1;
}

void json_bebaskan(dokumen_json_t *dok)
{
    free(dok->simpul);
    dok->simpul = NULL;
    dok->jumlah = 0;
    dok->kapasitas = 0;
}

const simpul_json_t *json_akar(const dokumen_json_t *dok)
{
    return dok->jumlah > 0 ? &dok->simpul[0] : NULL;
}

static size_t tulis_utf8(char *keluar, unsigned long kode)
{
    if (kode < 0x80) {
        keluar[0] = (char)kode;
        return 1;
    }
    if (kode < 0x800) {
        keluar[0] = (char)(0xC0 | (kode >> 6));
        keluar[1] = (char)(0x80 | (kode & 0x3F));
        return 2;
    }
    if (kode < 0x10000) {
        keluar[0] = (char)(0xE0 | (kode >> 12));
        keluar[1] = (char)(0x80 | ((kode >> 6) & 0x3F));
        keluar[2] = (char)(0x80 | (kode & 0x3F));
        return 3;
    }
    keluar[0] = (char)(0xF0 | (kode >> 18));
    keluar[1] = (char)(0x80 | ((kode >> 12) & 0x3F));
    keluar[2] = (char)(0x80 | ((kode >> 6) & 0x3F));
    keluar[3] = (char)(0x80 | (kode & 0x3F));
    return 4;
}

static unsigned long ambil_heks4(const char *m)
{
    return ((unsigned long)nilai_heks(m[0]) << 12) | ((unsigned long)nilai_heks(m[1]) << 8) |
           ((unsigned long)nilai_heks(m[2]) << 4) | (unsigned long)nilai_heks(m[3]);
}

/* m sudah tervalidasi oleh json_urai. Hasil tidak lebih panjang dari n. */
static char *buka_teks(const char *m, size_t n)
{
    char *keluar = malloc(n + 1);
    size_t i = 0;
    size_t o = 0;

    if (keluar == NULL) {
        return NULL;
    }
    while (i < n) {
        if (m[i] != '\\') {
            keluar[o++] = m[i++];
            continue;
        }
        i++;
        switch (m[i]) {
        case 'b':
            keluar[o++] = '\b';
            i++;
            break;
        case 'f':
            keluar[o++] = '\f';
            i++;
            break;
        case 'n':
            keluar[o++] = '\n';
            i++;
            break;
        case 'r':
            keluar[o++] = '\r';
            i++;
            break;
        case 't':
            keluar[o++] = '\t';
            i++;
            break;
        case 'u': {
            unsigned long kode = ambil_heks4(m + i + 1);
            i += 5;
            if (kode >= 0xD800 && kode <= 0xDBFF) {
                if (i + 6 <= n && m[i] == '\\' && m[i + 1] == 'u') {
                    unsigned long rendah = ambil_heks4(m + i + 2);
                    if (rendah >= 0xDC00 && rendah <= 0xDFFF) {
                        kode = 0x10000 + ((kode - 0xD800) << 10) + (rendah - 0xDC00);
                        i += 6;
                    } else {
                        kode = 0xFFFD;
                    }
                } else {
                    kode = 0xFFFD;
                }
            } else if ((kode >= 0xDC00 && kode <= 0xDFFF) || kode == 0) {
                kode = 0xFFFD;
            }
            o += tulis_utf8(keluar + o, kode);
            break;
        }
        default: /* " \ / */
            keluar[o++] = m[i++];
            break;
        }
    }
    keluar[o] = '\0';
    return keluar;
}

static int kunci_sama(const dokumen_json_t *dok, const simpul_json_t *s, const char *kunci)
{
    size_t n = s->kunci_akhir - s->kunci_awal;
    const char *m = dok->sumber + s->kunci_awal;

    if (memchr(m, '\\', n) == NULL) {
        return strlen(kunci) == n && memcmp(m, kunci, n) == 0;
    }
    {
        char *buka = buka_teks(m, n);
        int hasil = buka != NULL && strcmp(buka, kunci) == 0;
        free(buka);
        return hasil;
    }
}

const simpul_json_t *json_cari(const dokumen_json_t *dok, const simpul_json_t *objek,
                               const char *kunci)
{
    int i;

    if (objek == NULL || objek->jenis != JSON_OBJEK) {
        return NULL;
    }
    for (i = objek->anak_pertama; i >= 0; i = dok->simpul[i].saudara_berikut) {
        if (kunci_sama(dok, &dok->simpul[i], kunci)) {
            return &dok->simpul[i];
        }
    }
    return NULL;
}

char *json_teks_salin(const dokumen_json_t *dok, const simpul_json_t *simpul)
{
    if (simpul == NULL || simpul->jenis != JSON_TEKS) {
        return NULL;
    }
    return buka_teks(dok->sumber + simpul->awal + 1, simpul->akhir - simpul->awal - 2);
}

void json_mentah(const dokumen_json_t *dok, const simpul_json_t *simpul, const char **awal,
                 size_t *panjang)
{
    *awal = dok->sumber + simpul->awal;
    *panjang = simpul->akhir - simpul->awal;
}

int json_logika(const dokumen_json_t *dok, const simpul_json_t *simpul, int *hasil)
{
    if (simpul == NULL || simpul->jenis != JSON_LOGIKA) {
        return 0;
    }
    *hasil = dok->sumber[simpul->awal] == 't';
    return 1;
}

int json_bulat(const dokumen_json_t *dok, const simpul_json_t *simpul, long *hasil)
{
    char salinan[32];
    size_t n;
    size_t i;
    char *akhir;
    long nilai;

    if (simpul == NULL || simpul->jenis != JSON_ANGKA) {
        return 0;
    }
    n = simpul->akhir - simpul->awal;
    if (n == 0 || n >= sizeof salinan) {
        return 0;
    }
    memcpy(salinan, dok->sumber + simpul->awal, n);
    salinan[n] = '\0';
    for (i = 0; i < n; i++) {
        if (!(ADALAH_ANGKA(salinan[i]) || (i == 0 && salinan[i] == '-'))) {
            return 0;
        }
    }
    errno = 0;
    nilai = strtol(salinan, &akhir, 10);
    if (errno != 0 || *akhir != '\0') {
        return 0;
    }
    *hasil = nilai;
    return 1;
}
