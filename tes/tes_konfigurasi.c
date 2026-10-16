
#include <string.h>

#include "konfigurasi.h"
#include "tes.h"

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
}
