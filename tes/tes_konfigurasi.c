
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "klien_api.h"
#include "konfigurasi.h"
#include "tes.h"

#define JSON_LENGKAP(pengguna, sandi)                                                      \
    "{\"server_api\":\"http://api.berkas:3000\",\"server_nas\":\"nas.berkas:5001\","       \
    "\"nama_pengguna\":\"" pengguna "\",\"kata_sandi\":\"" sandi "\"}"

static char dir_konfigurasi[640];
static char berkas_konfigurasi[700];

static void buat_dir(const char *jalur)
{
#ifdef _WIN32
    _mkdir(jalur);
#else
    mkdir(jalur, 0700);
#endif
}

static void hapus_dir(const char *jalur)
{
#ifdef _WIN32
    _rmdir(jalur);
#else
    rmdir(jalur);
#endif
}

static void siapkan_berkas_uji(void)
{
    char tengah[560];

    snprintf(tengah, sizeof tengah, "%s/.config", tes_dir_profil);
    snprintf(dir_konfigurasi, sizeof dir_konfigurasi, "%s/mcp-synology-spreadsheet", tengah);
    snprintf(berkas_konfigurasi, sizeof berkas_konfigurasi, "%s/config.json", dir_konfigurasi);
    buat_dir(tes_dir_profil);
    buat_dir(tengah);
    buat_dir(dir_konfigurasi);
}

static void bersihkan_berkas_uji(void)
{
    char tengah[560];

    remove(berkas_konfigurasi);
    snprintf(tengah, sizeof tengah, "%s/.config", tes_dir_profil);
    hapus_dir(dir_konfigurasi);
    hapus_dir(tengah);
    hapus_dir(tes_dir_profil);
}

static void tulis_berkas_uji(const char *isi, size_t panjang)
{
    FILE *f = fopen(berkas_konfigurasi, "wb");

    PERIKSA(f != NULL);
    if (f != NULL) {
        fwrite(isi, 1, panjang, f);
        fclose(f);
    }
}

static void tulis_teks_uji(const char *isi)
{
    tulis_berkas_uji(isi, strlen(isi));
}

static void periksa_gagal_muat(const char *isi, const char *bagian_galat, const char *rahasia)
{
    konfigurasi_t k;
    char galat[256];
    unsigned long long tanda = 0;

    tulis_teks_uji(isi);
    PERIKSA(!konfigurasi_muat_semua(&k, &tanda, galat, sizeof galat));
    PERIKSA(tanda != 0);
    PERIKSA_MENGANDUNG(galat, bagian_galat);
    if (rahasia != NULL) {
        PERIKSA_TIDAK_MENGANDUNG(galat, rahasia);
    }
    PERIKSA(k.nama_pengguna == NULL && k.kata_sandi == NULL);
}

static void periksa_berkas_konfigurasi(void)
{
    konfigurasi_t k;
    char galat[256];
    unsigned long long tanda = 0;
    unsigned long long tanda_lain;
    char *besar;

    siapkan_berkas_uji();
    tes_atur_lingkungan(LINGKUNGAN_SERVER_API, "http://env.local:3000");
    tes_atur_lingkungan(LINGKUNGAN_SERVER_NAS, "env.local:5001");
    tes_atur_lingkungan(LINGKUNGAN_NAMA_PENGGUNA, "pengguna_env");
    tes_atur_lingkungan(LINGKUNGAN_KATA_SANDI, "sandi-env-1");

    /* tanpa berkas: dipakai variabel lingkungan */
    PERIKSA(konfigurasi_tanda() == 0);
    PERIKSA(konfigurasi_muat_semua(&k, &tanda, galat, sizeof galat));
    PERIKSA(tanda == 0);
    PERIKSA_TEKS(k.nama_pengguna, "pengguna_env");
    konfigurasi_bebaskan(&k);

    /* ada berkas: berkas menang atas lingkungan */
    tulis_teks_uji(JSON_LENGKAP("pengguna_berkas", "sandi-berkas-2"));
    PERIKSA(konfigurasi_tanda() != 0);
    PERIKSA(konfigurasi_muat_semua(&k, &tanda, galat, sizeof galat));
    PERIKSA(tanda == konfigurasi_tanda());
    PERIKSA(k.server_api.https == 0 && k.server_api.port == 3000);
    PERIKSA(k.server_nas.https == 1 && k.server_nas.port == 5001);
    PERIKSA_TEKS(k.nama_pengguna, "pengguna_berkas");
    PERIKSA_TEKS(k.kata_sandi, "sandi-berkas-2");
    konfigurasi_bebaskan(&k);

    /* isi berubah: tanda berubah */
    tulis_teks_uji(JSON_LENGKAP("pengguna_berkas", "sandi-berkas-9"));
    tanda_lain = konfigurasi_tanda();
    PERIKSA(tanda_lain != 0 && tanda_lain != tanda);

    /* BOM UTF-8 dari editor diterima */
    tulis_teks_uji("\xEF\xBB\xBF" JSON_LENGKAP("pengguna_bom", "sandi-bom-3"));
    PERIKSA(konfigurasi_muat_semua(&k, NULL, galat, sizeof galat));
    PERIKSA_TEKS(k.nama_pengguna, "pengguna_bom");
    konfigurasi_bebaskan(&k);

    /* berkas bermasalah: galat jelas, tanpa nilai rahasia */
    periksa_gagal_muat("{\"server_api\":\"http://api.berkas:3000\",\"kata_sandi\":\"sandi-rahasia-3\"}",
                       "belum diisi", "sandi-rahasia-3");
    periksa_gagal_muat("{\"server_api\":\"\",\"server_nas\":\"\",\"nama_pengguna\":\"\","
                       "\"kata_sandi\":\"\"}",
                       "belum diisi", NULL);
    periksa_gagal_muat("{\"server_api\":\"http://api.berkas:3000\",\"server_nas\":\"n\","
                       "\"nama_pengguna\":\"u\",\"kata_sandi\":123}",
                       "harus berupa teks", NULL);
    periksa_gagal_muat("{\"server_api\":", "bukan JSON valid", NULL);
    periksa_gagal_muat("[]", "objek JSON", NULL);
    periksa_gagal_muat("{\"server_api\":\"http://api.berkas:3000\",\"server_nas\":\"nas/jalur\","
                       "\"nama_pengguna\":\"u\",\"kata_sandi\":\"sandi-rahasia-4\"}",
                       "server_nas tidak valid", "sandi-rahasia-4");

    /* berkas terlalu besar ditolak */
    besar = malloc(20 * 1024);
    PERIKSA(besar != NULL);
    if (besar != NULL) {
        memset(besar, ' ', 20 * 1024);
        tulis_berkas_uji(besar, 20 * 1024);
        PERIKSA(!konfigurasi_muat_semua(&k, &tanda, galat, sizeof galat));
        PERIKSA_MENGANDUNG(galat, "terlalu besar");
        free(besar);
    }

    /* berkas dihapus: kembali ke lingkungan */
    remove(berkas_konfigurasi);
    PERIKSA(konfigurasi_tanda() == 0);
    PERIKSA(konfigurasi_muat_semua(&k, &tanda, galat, sizeof galat));
    PERIKSA_TEKS(k.nama_pengguna, "pengguna_env");
    konfigurasi_bebaskan(&k);

    /* klien: perubahan berkas berlaku pada panggilan berikutnya, tanpa memulai ulang */
    tulis_teks_uji(JSON_LENGKAP("pengguna_a", "sandi-a"));
    PERIKSA(api_pastikan_konfigurasi(galat, sizeof galat));
    PERIKSA(api_konfigurasi() != NULL);
    if (api_konfigurasi() != NULL) {
        PERIKSA_TEKS(api_konfigurasi()->nama_pengguna, "pengguna_a");
    }
    tulis_teks_uji(JSON_LENGKAP("pengguna_b", "sandi-b"));
    PERIKSA(api_pastikan_konfigurasi(galat, sizeof galat));
    if (api_konfigurasi() != NULL) {
        PERIKSA_TEKS(api_konfigurasi()->nama_pengguna, "pengguna_b");
    }
    tulis_teks_uji("{rusak");
    PERIKSA(!api_pastikan_konfigurasi(galat, sizeof galat));
    PERIKSA_MENGANDUNG(galat, "Konfigurasi belum lengkap");
    PERIKSA(api_konfigurasi() == NULL);
    tulis_teks_uji(JSON_LENGKAP("pengguna_c", "sandi-c"));
    PERIKSA(api_pastikan_konfigurasi(galat, sizeof galat));
    if (api_konfigurasi() != NULL) {
        PERIKSA_TEKS(api_konfigurasi()->nama_pengguna, "pengguna_c");
    }
    remove(berkas_konfigurasi);
    PERIKSA(api_pastikan_konfigurasi(galat, sizeof galat));
    if (api_konfigurasi() != NULL) {
        PERIKSA_TEKS(api_konfigurasi()->nama_pengguna, "pengguna_env");
    }
    api_cabut();

    tes_atur_lingkungan(LINGKUNGAN_SERVER_API, NULL);
    tes_atur_lingkungan(LINGKUNGAN_SERVER_NAS, NULL);
    tes_atur_lingkungan(LINGKUNGAN_NAMA_PENGGUNA, NULL);
    tes_atur_lingkungan(LINGKUNGAN_KATA_SANDI, NULL);
    bersihkan_berkas_uji();
}

static void periksa_alamat(const char *teks, int https_bawaan, int https, int port,
                           const char *host, const char *host_dan_port)
{
    alamat_t a;
    char galat[96];

    PERIKSA(alamat_urai(teks, https_bawaan, &a, galat, sizeof galat));
    PERIKSA(a.https == https);
    PERIKSA(a.port == port);
    PERIKSA_TEKS(a.nama_host, host);
    PERIKSA_TEKS(a.host_dan_port, host_dan_port);
}

static void periksa_galat(const char *teks, const char *bagian_galat)
{
    alamat_t a;
    char galat[96];

    PERIKSA(!alamat_urai(teks, 1, &a, galat, sizeof galat));
    PERIKSA_MENGANDUNG(galat, bagian_galat);
}

static void periksa_host_panjang(size_t panjang, int harapan_berhasil)
{
    char teks[400];
    alamat_t a;
    char galat[96];

    memset(teks, 'a', panjang);
    teks[panjang] = '\0';
    PERIKSA(alamat_urai(teks, 1, &a, galat, sizeof galat) == harapan_berhasil);
    if (!harapan_berhasil) {
        PERIKSA_MENGANDUNG(galat, "terlalu panjang");
    }
}

static void atur_semua(const char *api, const char *nas, const char *pengguna, const char *sandi)
{
    tes_atur_lingkungan(LINGKUNGAN_SERVER_API, api);
    tes_atur_lingkungan(LINGKUNGAN_SERVER_NAS, nas);
    tes_atur_lingkungan(LINGKUNGAN_NAMA_PENGGUNA, pengguna);
    tes_atur_lingkungan(LINGKUNGAN_KATA_SANDI, sandi);
}

void tes_periksa_konfigurasi(void)
{
    konfigurasi_t k;
    char galat[256];

    /* alamat: bentuk yang diterima */
    periksa_alamat("nas.local:5001", 1, 1, 5001, "nas.local", "nas.local:5001");
    periksa_alamat("http://192.168.1.5:5000/", 1, 0, 5000, "192.168.1.5", "192.168.1.5:5000");
    periksa_alamat("HTTPS://nas", 0, 1, 443, "nas", "nas");
    periksa_alamat("  nas  ", 0, 0, 80, "nas", "nas");
    periksa_alamat("[fe80::1]:5001", 1, 1, 5001, "fe80::1", "[fe80::1]:5001");
    periksa_alamat("[::1]", 0, 0, 80, "::1", "[::1]");

    /* alamat: bentuk yang ditolak */
    periksa_galat(NULL, "kosong");
    periksa_galat("", "kosong");
    periksa_galat("nas/jalur", "jalur");
    periksa_galat("nas:0", "port tidak valid");
    periksa_galat("nas:", "port tidak valid");
    periksa_galat("nas:65536", "port tidak valid");
    periksa_galat("nas:abc", "port tidak valid");
    periksa_galat("fe80::1", "IPv6 harus");
    periksa_galat("na s", "tidak diizinkan");
    periksa_galat("http://", "host kosong");
    periksa_galat(":5000", "host kosong");
    periksa_galat("[]", "IPv6 tidak valid");
    periksa_galat("[::1]x", "IPv6 tidak valid");

    /* batas panjang host: 255 diterima; 256 dan 300 ditolak, bukan dipotong */
    periksa_host_panjang(255, 1);
    periksa_host_panjang(256, 0);
    periksa_host_panjang(300, 0);

    /* konfigurasi: semua variabel belum diatur */
    atur_semua(NULL, NULL, NULL, NULL);
    PERIKSA(!konfigurasi_muat(&k, galat, sizeof galat));
    PERIKSA_MENGANDUNG(galat, "belum diatur");
    PERIKSA_MENGANDUNG(galat, LINGKUNGAN_SERVER_API);
    PERIKSA_MENGANDUNG(galat, LINGKUNGAN_SERVER_NAS);
    PERIKSA_MENGANDUNG(galat, LINGKUNGAN_NAMA_PENGGUNA);
    PERIKSA_MENGANDUNG(galat, LINGKUNGAN_KATA_SANDI);
    PERIKSA(k.nama_pengguna == NULL && k.kata_sandi == NULL);

    /* konfigurasi: lengkap */
    atur_semua("http://nas.local:3000", "nas.local:5001", "pengguna_uji", "rahasia-uji-123");
    PERIKSA(konfigurasi_muat(&k, galat, sizeof galat));
    PERIKSA(k.server_api.https == 0 && k.server_api.port == 3000);
    PERIKSA(k.server_nas.https == 1 && k.server_nas.port == 5001);
    PERIKSA_TEKS(k.nama_pengguna, "pengguna_uji");
    PERIKSA_TEKS(k.kata_sandi, "rahasia-uji-123");
    konfigurasi_bebaskan(&k);
    PERIKSA(k.nama_pengguna == NULL && k.kata_sandi == NULL);

    /* konfigurasi: satu variabel kurang; pesan hanya memuat nama, tidak nilai */
    atur_semua("http://nas.local:3000", "nas.local:5001", "pengguna_uji", NULL);
    PERIKSA(!konfigurasi_muat(&k, galat, sizeof galat));
    PERIKSA_MENGANDUNG(galat, LINGKUNGAN_KATA_SANDI);
    PERIKSA_TIDAK_MENGANDUNG(galat, LINGKUNGAN_NAMA_PENGGUNA);
    PERIKSA_TIDAK_MENGANDUNG(galat, "pengguna_uji");

    /* konfigurasi: alamat tidak valid; pesan tidak memuat kata sandi */
    atur_semua("http://nas.local:3000", "nas/jalur", "pengguna_uji", "rahasia-uji-123");
    PERIKSA(!konfigurasi_muat(&k, galat, sizeof galat));
    PERIKSA_MENGANDUNG(galat, LINGKUNGAN_SERVER_NAS " tidak valid");
    PERIKSA_TIDAK_MENGANDUNG(galat, "rahasia-uji-123");
    PERIKSA(k.nama_pengguna == NULL && k.kata_sandi == NULL);

    atur_semua(NULL, NULL, NULL, NULL);

    periksa_berkas_konfigurasi();
}
