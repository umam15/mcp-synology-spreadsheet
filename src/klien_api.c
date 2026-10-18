#include "klien_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "catat.h"
#include "json.h"
#include "penyangga.h"

static konfigurasi_t konfigurasi_global;
static int status_konfigurasi = 0; /* 0 belum dimuat, 1 berhasil, -1 gagal */
static char pesan_konfigurasi[200];
static char *token_global = NULL;
static unsigned long long tanda_konfigurasi = 0; /* tanda isi berkas saat konfigurasi dimuat */

/* Mencabut token (upaya terbaik), membuang token dan kredensial dari memori. */
static void lepas_sesi(void)
{
    if (token_global != NULL && status_konfigurasi == 1) {
        respons_http_t respons;
        char galat[128];

        penyangga_siapkan(&respons.badan);
        if (!http_kirim(&konfigurasi_global.server_api, "POST", "/spreadsheets/authorize/token/revoke",
                        token_global, NULL, 0, &respons, galat, sizeof galat)) {
            catat_info("Token gagal dicabut: %s", galat);
        } else if (respons.kode_status >= 200 && respons.kode_status < 300) {
            catat_info("Token dicabut (HTTP %d)", respons.kode_status);
        } else {
            char ringkas[320];

            api_ringkas_galat(respons.badan.data, respons.badan.panjang, ringkas, sizeof ringkas);
            catat_info("Token gagal dicabut (HTTP %d): %s", respons.kode_status, ringkas);
        }
        penyangga_bebaskan(&respons.badan);
    }
    free(token_global);
    token_global = NULL;
    if (status_konfigurasi == 1) {
        konfigurasi_bebaskan(&konfigurasi_global);
    }
    status_konfigurasi = 0;
}

static int pastikan_konfigurasi(char *galat, size_t ukuran_galat)
{
    /* Berkas konfigurasi berubah: buang sesi lama dan muat ulang, tanpa memulai ulang server. */
    if (status_konfigurasi != 0 && konfigurasi_tanda() != tanda_konfigurasi) {
        lepas_sesi();
    }
    if (status_konfigurasi == 0) {
        status_konfigurasi = konfigurasi_muat_semua(&konfigurasi_global, &tanda_konfigurasi,
                                                    pesan_konfigurasi, sizeof pesan_konfigurasi)
                                 ? 1
                                 : -1;
    }
    if (status_konfigurasi == 1) {
        return 1;
    }
    snprintf(galat, ukuran_galat, "Konfigurasi belum lengkap: %s", pesan_konfigurasi);
    return 0;
}

int api_pastikan_konfigurasi(char *galat, size_t ukuran_galat)
{
    return pastikan_konfigurasi(galat, ukuran_galat);
}

const konfigurasi_t *api_konfigurasi(void)
{
    return status_konfigurasi == 1 ? &konfigurasi_global : NULL;
}

/* ---------- pesan galat ringkas (hemat token) ---------- */

/* Menyalin sumber ke keluar sebagai satu baris: spasi dan karakter kontrol berurutan dilipat,
   tag HTML dibuang bila buang_tag, dipotong pada ukuran-1 byte di tepi karakter UTF-8. */
static void bersihkan(const char *sumber, size_t panjang, int buang_tag, char *keluar,
                      size_t ukuran)
{
    size_t n = 0;
    size_t i;
    int dalam_tag = 0;
    int spasi = 1; /* 1 = jangan menulis spasi (awal atau setelah spasi) */

    if (ukuran == 0) {
        return;
    }
    for (i = 0; i < panjang && n + 1 < ukuran; i++) {
        unsigned char c = (unsigned char)sumber[i];

        if (dalam_tag) {
            if (c == '>') {
                dalam_tag = 0;
            }
            continue;
        }
        if (buang_tag && c == '<') {
            dalam_tag = 1;
            c = ' ';
        }
        if (c <= ' ') {
            if (!spasi) {
                keluar[n++] = ' ';
                spasi = 1;
            }
            continue;
        }
        keluar[n++] = (char)c;
        spasi = 0;
    }
    if (i < panjang && ((unsigned char)sumber[i] & 0xC0) == 0x80) {
        while (n > 0 && ((unsigned char)keluar[n - 1] & 0xC0) == 0x80) {
            n--; /* buang byte lanjutan yang sudah tersalin */
        }
        if (n > 0) {
            n--; /* buang byte awal karakter yang terpotong */
        }
    }
    if (n > 0 && keluar[n - 1] == ' ') {
        n--;
    }
    keluar[n] = '\0';
}

static const char *cari_tanpa_huruf_besar(const char *teks, size_t panjang, const char *jarum)
{
    size_t m = strlen(jarum);
    size_t i;
    size_t j;

    for (i = 0; i + m <= panjang; i++) {
        for (j = 0; j < m; j++) {
            char a = teks[i + j];

            if (a >= 'A' && a <= 'Z') {
                a = (char)(a + ('a' - 'A'));
            }
            if (a != jarum[j]) {
                break;
            }
        }
        if (j == m) {
            return teks + i;
        }
    }
    return NULL;
}

void api_ringkas_galat(const char *badan, size_t panjang, char *keluar, size_t ukuran)
{
    dokumen_json_t dok;

    if (ukuran == 0) {
        return;
    }
    keluar[0] = '\0';
    if (badan == NULL || panjang == 0) {
        snprintf(keluar, ukuran, "(tanpa isi)");
        return;
    }
    if (json_urai(&dok, badan, panjang)) {
        char *teks = NULL;

        if (json_akar(&dok)->jenis == JSON_OBJEK) {
            const simpul_json_t *n = json_cari(&dok, json_akar(&dok), "message");

            if (n == NULL) {
                n = json_cari(&dok, json_akar(&dok), "error");
            }
            if (n != NULL) {
                teks = json_teks_salin(&dok, n);
            }
        }
        json_bebaskan(&dok);
        if (teks != NULL) {
            bersihkan(teks, strlen(teks), 0, keluar, ukuran);
            free(teks);
        }
    }
    if (keluar[0] == '\0') {
        const char *awal = cari_tanpa_huruf_besar(badan, panjang, "<title>");
        const char *akhir = NULL;

        if (awal != NULL) {
            awal += strlen("<title>");
            akhir = cari_tanpa_huruf_besar(awal, panjang - (size_t)(awal - badan), "</title>");
        }
        if (awal != NULL && akhir != NULL) {
            bersihkan(awal, (size_t)(akhir - awal), 0, keluar, ukuran);
        } else {
            size_t k = 0;

            while (k < panjang && (unsigned char)badan[k] <= ' ') {
                k++;
            }
            bersihkan(badan, panjang, k < panjang && badan[k] == '<', keluar, ukuran);
        }
    }
    if (keluar[0] == '\0') {
        snprintf(keluar, ukuran, "(tanpa isi)");
    }
}

const char *api_petunjuk_galat(int kode_status, const char *ringkas)
{
    if (kode_status == 502 || kode_status == 503 || kode_status == 504) {
        return " - layanan Spreadsheet API di balik proxy tidak menjawab; periksa layanan di NAS";
    }
    if (kode_status == 500 && ringkas != NULL && strstr(ringkas, "Unexpected error") != NULL) {
        return " - kemungkinan ID spreadsheet tidak ada atau sudah dihapus";
    }
    return "";
}

static const char *petunjuk_masuk(int kode_status)
{
    switch (kode_status) {
    case 400:
    case 401:
    case 403:
        return " - periksa nama pengguna, kata sandi, dan alamat NAS; akun dengan 2FA tidak "
               "didukung";
    default:
        return "";
    }
}

int api_masuk(char *galat, size_t ukuran_galat)
{
    penyangga_t badan;
    respons_http_t respons;
    dokumen_json_t dok;
    int berhasil = 0;

    if (!pastikan_konfigurasi(galat, ukuran_galat)) {
        return 0;
    }
    penyangga_siapkan(&badan);
    penyangga_siapkan(&respons.badan);

    penyangga_tambah_teks(&badan, "{\"username\":");
    penyangga_tambah_teks_json(&badan, konfigurasi_global.nama_pengguna,
                               strlen(konfigurasi_global.nama_pengguna));
    penyangga_tambah_teks(&badan, ",\"password\":");
    penyangga_tambah_teks_json(&badan, konfigurasi_global.kata_sandi,
                               strlen(konfigurasi_global.kata_sandi));
    penyangga_tambah_teks(&badan, ",\"host\":");
    penyangga_tambah_teks_json(&badan, konfigurasi_global.server_nas.host_dan_port,
                               strlen(konfigurasi_global.server_nas.host_dan_port));
    penyangga_tambah_format(&badan, ",\"protocol\":\"%s\"}",
                            konfigurasi_global.server_nas.https ? "https" : "http");
    if (badan.gagal) {
        snprintf(galat, ukuran_galat, "Memori tidak cukup");
        goto selesai;
    }
    if (!http_kirim(&konfigurasi_global.server_api, "POST", "/spreadsheets/authorize", NULL,
                    badan.data, badan.panjang, &respons, galat, ukuran_galat)) {
        goto selesai;
    }
    if (respons.kode_status != 200) {
        char ringkas[320];

        api_ringkas_galat(respons.badan.data, respons.badan.panjang, ringkas, sizeof ringkas);
        snprintf(galat, ukuran_galat, "Masuk gagal (HTTP %d): %s%s%s", respons.kode_status, ringkas,
                 petunjuk_masuk(respons.kode_status),
                 api_petunjuk_galat(respons.kode_status, ringkas));
        goto selesai;
    }
    if (json_urai(&dok, respons.badan.data, respons.badan.panjang)) {
        char *token = json_teks_salin(&dok, json_cari(&dok, json_akar(&dok), "token"));
        json_bebaskan(&dok);
        if (token != NULL && token[0] != '\0') {
            free(token_global);
            token_global = token;
            berhasil = 1;
        } else {
            free(token);
        }
    }
    if (!berhasil) {
        snprintf(galat, ukuran_galat, "Respons masuk tidak memuat token");
    }

selesai:
    if (badan.data != NULL) {
        memset(badan.data, 0, badan.panjang); /* badan memuat kata sandi */
    }
    penyangga_bebaskan(&badan);
    penyangga_bebaskan(&respons.badan);
    return berhasil;
}

int api_panggil(const char *metode, const char *jalur, const char *badan, size_t panjang_badan,
                respons_http_t *respons, char *galat, size_t ukuran_galat)
{
    int percobaan;

    if (!pastikan_konfigurasi(galat, ukuran_galat)) {
        return 0;
    }
    for (percobaan = 0; percobaan < 2; percobaan++) {
        if (token_global == NULL && !api_masuk(galat, ukuran_galat)) {
            return 0;
        }
        penyangga_kosongkan(&respons->badan);
        if (!http_kirim(&konfigurasi_global.server_api, metode, jalur, token_global, badan,
                        panjang_badan, respons, galat, ukuran_galat)) {
            return 0;
        }
        if (respons->kode_status != 401) {
            return 1;
        }
        free(token_global); /* token ditolak: masuk ulang sekali */
        token_global = NULL;
    }
    return 1;
}

void api_cabut(void)
{
    lepas_sesi();
    http_tutup();
}
