#include <stdlib.h>

#include "json.h"
#include "penyangga.h"
#include "tes.h"

/* Teks JSON hasil penyangga harus kembali menjadi teks semula (atau yang sudah dinormalkan). */
static void periksa_bolak_balik(const char *asal, size_t panjang, const char *harapan)
{
    penyangga_t p;
    dokumen_json_t dok;
    char *balik;

    penyangga_siapkan(&p);
    PERIKSA(penyangga_tambah_teks_json(&p, asal, panjang));
    PERIKSA(p.panjang >= 2 && p.data[0] == '"' && p.data[p.panjang - 1] == '"');
    PERIKSA(json_urai(&dok, p.data, p.panjang));
    balik = json_teks_salin(&dok, json_akar(&dok));
    PERIKSA_TEKS(balik, harapan);
    free(balik);
    json_bebaskan(&dok);
    penyangga_bebaskan(&p);
}

void tes_periksa_penyangga(void)
{
    penyangga_t p;
    size_t i;
    static const char kendali[] = "a\"b\\c\nd\te\x01";

    penyangga_siapkan(&p);
    PERIKSA(p.panjang == 0);
    PERIKSA_TEKS(penyangga_isi(&p), "");
    PERIKSA(penyangga_tambah_teks(&p, "abc"));
    PERIKSA(penyangga_tambah_format(&p, "-%d-%s", 42, "x"));
    PERIKSA_TEKS(penyangga_isi(&p), "abc-42-x");
    PERIKSA(p.panjang == 8);
    penyangga_kosongkan(&p);
    PERIKSA(p.panjang == 0);
    PERIKSA_TEKS(penyangga_isi(&p), "");
    PERIKSA(!p.gagal);
    penyangga_bebaskan(&p);

    /* tumbuh otomatis dan tetap berakhir NUL */
    penyangga_siapkan(&p);
    for (i = 0; i < 100000; i++) {
        penyangga_tambah_teks(&p, "0123456789");
    }
    PERIKSA(p.panjang == 1000000);
    PERIKSA(p.data[p.panjang] == '\0');
    PERIKSA(!p.gagal);
    penyangga_bebaskan(&p);

    /* teks JSON: bentuk sederhana dan bolak-balik untuk karakter sulit */
    penyangga_siapkan(&p);
    penyangga_tambah_teks_json(&p, "a\"b", 3);
    PERIKSA_TEKS(penyangga_isi(&p), "\"a\\\"b\"");
    penyangga_bebaskan(&p);

    periksa_bolak_balik("", 0, "");
    periksa_bolak_balik(kendali, sizeof kendali - 1, kendali);
    periksa_bolak_balik("caf\xC3\xA9 \xF0\x9F\x98\x80", 10, "caf\xC3\xA9 \xF0\x9F\x98\x80");
    /* byte UTF-8 tidak valid diganti U+FFFD */
    periksa_bolak_balik("x\xFF" "y", 3, "x\xEF\xBF\xBD" "y");
    periksa_bolak_balik("x\xC3", 2, "x\xEF\xBF\xBD");
}
