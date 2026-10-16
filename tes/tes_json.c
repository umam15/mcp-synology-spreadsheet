#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "tes.h"

/* Mengembalikan 1 bila teks diterima sebagai JSON. */
static int diterima(const char *teks)
{
    dokumen_json_t dok;
    int ok = json_urai(&dok, teks, strlen(teks));

    if (ok) {
        json_bebaskan(&dok);
    }
    return ok;
}

static void periksa_teks_hasil(const dokumen_json_t *dok, const simpul_json_t *simpul,
                               const char *harapan)
{
    char *teks = json_teks_salin(dok, simpul);

    PERIKSA_TEKS(teks, harapan);
    free(teks);
}

static void periksa_penerimaan(void)
{
    char dalam[200];
    char terlalu_dalam[401];
    size_t i;

    PERIKSA(diterima("{}"));
    PERIKSA(diterima("[]"));
    PERIKSA(diterima("null"));
    PERIKSA(diterima("true"));
    PERIKSA(diterima("\"x\""));
    PERIKSA(diterima("-1.5e3"));
    PERIKSA(diterima("0"));
    PERIKSA(diterima("  {\"a\":[1,{\"b\":null}]}  "));
    PERIKSA(diterima("\"\\u00e9\""));

    PERIKSA(!diterima(""));
    PERIKSA(!diterima("{"));
    PERIKSA(!diterima("{\"a\":}"));
    PERIKSA(!diterima("[1,]"));
    PERIKSA(!diterima("{\"a\":1,}"));
    PERIKSA(!diterima("{a:1}"));
    PERIKSA(!diterima("'x'"));
    PERIKSA(!diterima("1 2"));
    PERIKSA(!diterima("[1 2]"));
    PERIKSA(!diterima("\"abc"));
    PERIKSA(!diterima("\"\\x\""));
    PERIKSA(!diterima("nul"));
    PERIKSA(!diterima("{\"a\" 1}"));
    PERIKSA(!diterima("01"));
    PERIKSA(!diterima("+1"));
    PERIKSA(!diterima(".5"));
    PERIKSA(!diterima("1."));
    PERIKSA(!diterima("\"a\nb\"")); /* karakter kendali mentah dalam teks */

    /* kedalaman: 10 tingkat diterima, 200 ditolak */
    for (i = 0; i < 10; i++) {
        dalam[i] = '[';
        dalam[10 + i] = ']';
    }
    dalam[20] = '\0';
    PERIKSA(diterima(dalam));
    for (i = 0; i < 200; i++) {
        terlalu_dalam[i] = '[';
        terlalu_dalam[200 + i] = ']';
    }
    terlalu_dalam[400] = '\0';
    PERIKSA(!diterima(terlalu_dalam));
}

static void periksa_akses(void)
{
    static const char sumber[] =
        "{\"nama\":\"Budi\",\"umur\":30,\"aktif\":true,\"data\":[1,[2,3]],\"kosong\":null,"
        "\"teks\":\"a\\nb\\u00e9\",\"besar\":99999999999999999999,\"pecahan\":1.5}";
    dokumen_json_t dok;
    const simpul_json_t *akar;
    const simpul_json_t *s;
    const char *awal;
    size_t panjang;
    long bulat = 0;
    int logika = 0;

    PERIKSA(json_urai(&dok, sumber, strlen(sumber)));
    akar = json_akar(&dok);
    PERIKSA(akar != NULL && akar->jenis == JSON_OBJEK);

    s = json_cari(&dok, akar, "nama");
    PERIKSA(s != NULL && s->jenis == JSON_TEKS);
    periksa_teks_hasil(&dok, s, "Budi");

    s = json_cari(&dok, akar, "umur");
    PERIKSA(s != NULL && s->jenis == JSON_ANGKA);
    PERIKSA(json_bulat(&dok, s, &bulat) && bulat == 30);
    PERIKSA(json_teks_salin(&dok, s) == NULL); /* bukan teks */

    s = json_cari(&dok, akar, "aktif");
    PERIKSA(s != NULL && s->jenis == JSON_LOGIKA);
    PERIKSA(json_logika(&dok, s, &logika) && logika == 1);
    PERIKSA(!json_logika(&dok, json_cari(&dok, akar, "umur"), &logika));

    s = json_cari(&dok, akar, "data");
    PERIKSA(s != NULL && s->jenis == JSON_LARIK);
    json_mentah(&dok, s, &awal, &panjang);
    PERIKSA(panjang == 9 && memcmp(awal, "[1,[2,3]]", 9) == 0);
    PERIKSA(json_cari(&dok, s, "x") == NULL); /* bukan objek */

    s = json_cari(&dok, akar, "kosong");
    PERIKSA(s != NULL && s->jenis == JSON_KOSONG);

    periksa_teks_hasil(&dok, json_cari(&dok, akar, "teks"), "a\nb\xC3\xA9");

    PERIKSA(!json_bulat(&dok, json_cari(&dok, akar, "besar"), &bulat));
    PERIKSA(!json_bulat(&dok, json_cari(&dok, akar, "pecahan"), &bulat));
    PERIKSA(json_cari(&dok, akar, "tidak_ada") == NULL);
    PERIKSA(json_cari(&dok, NULL, "nama") == NULL);
    PERIKSA(json_teks_salin(&dok, NULL) == NULL);
    json_bebaskan(&dok);
}

static void periksa_unicode(void)
{
    static const char sumber[] =
        "[\"\\ud83d\\ude00\",\"\\u0000\",\"\\/\\\\\\\"\\b\\f\\n\\r\\t\"]";
    dokumen_json_t dok;
    const simpul_json_t *akar;
    int indeks;

    PERIKSA(json_urai(&dok, sumber, strlen(sumber)));
    akar = json_akar(&dok);
    PERIKSA(akar != NULL && akar->jenis == JSON_LARIK);
    indeks = akar->anak_pertama;
    periksa_teks_hasil(&dok, &dok.simpul[indeks], "\xF0\x9F\x98\x80"); /* pasangan pengganti */
    indeks = dok.simpul[indeks].saudara_berikut;
    periksa_teks_hasil(&dok, &dok.simpul[indeks], "\xEF\xBF\xBD"); /* U+0000 diganti U+FFFD */
    indeks = dok.simpul[indeks].saudara_berikut;
    periksa_teks_hasil(&dok, &dok.simpul[indeks], "/\\\"\b\f\n\r\t");
    json_bebaskan(&dok);
}

void tes_periksa_json(void)
{
    periksa_penerimaan();
    periksa_akses();
    periksa_unicode();
}
