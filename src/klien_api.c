#include "klien_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        http_kirim(&konfigurasi_global.server_api, "POST", "/spreadsheets/authorize/token/revoke",
                   token_global, NULL, 0, &respons, galat, sizeof galat);
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
        snprintf(galat, ukuran_galat, "Masuk gagal (HTTP %d)%s: %.200s", respons.kode_status,
                 petunjuk_masuk(respons.kode_status), penyangga_isi(&respons.badan));
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
