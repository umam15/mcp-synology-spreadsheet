#include "konfigurasi.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static void isi_galat(char *galat, size_t ukuran, const char *pesan)
{
    if (galat != NULL && ukuran > 0) {
        snprintf(galat, ukuran, "%s", pesan);
    }
}

/* Menyalin teks ke tujuan; mengembalikan 0 (tanpa memotong) bila tidak muat. */
static int salin_batas(char *tujuan, size_t ukuran, const char *sumber)
{
    size_t n = strlen(sumber);

    if (n >= ukuran) {
        return 0;
    }
    memcpy(tujuan, sumber, n + 1);
    return 1;
}

static int punya_awalan(const char *teks, const char *awalan)
{
    while (*awalan != '\0') {
        if (tolower((unsigned char)*teks) != *awalan) {
            return 0;
        }
        teks++;
        awalan++;
    }
    return 1;
}

static int bagian_port(const char *teks, int *port)
{
    long nilai = 0;

    if (*teks == '\0') {
        return 0;
    }
    for (; *teks != '\0'; teks++) {
        if (*teks < '0' || *teks > '9') {
            return 0;
        }
        nilai = nilai * 10 + (*teks - '0');
        if (nilai > 65535) {
            return 0;
        }
    }
    if (nilai < 1) {
        return 0;
    }
    *port = (int)nilai;
    return 1;
}

int alamat_urai(const char *teks, int https_bawaan, alamat_t *alamat, char *galat,
                size_t ukuran_galat)
{
    char buf[300];
    char *p;
    char *garis;
    size_t n;
    size_t i;
    int https = https_bawaan;
    int port;

    memset(alamat, 0, sizeof *alamat);
    if (teks == NULL) {
        isi_galat(galat, ukuran_galat, "alamat kosong");
        return 0;
    }
    while (*teks != '\0' && isspace((unsigned char)*teks)) {
        teks++;
    }
    n = strlen(teks);
    while (n > 0 && isspace((unsigned char)teks[n - 1])) {
        n--;
    }
    if (n == 0 || n >= sizeof buf) {
        isi_galat(galat, ukuran_galat, "alamat kosong atau terlalu panjang");
        return 0;
    }
    memcpy(buf, teks, n);
    buf[n] = '\0';
    p = buf;
    if (punya_awalan(p, "https://")) {
        https = 1;
        p += 8;
    } else if (punya_awalan(p, "http://")) {
        https = 0;
        p += 7;
    }
    garis = strchr(p, '/');
    if (garis != NULL) {
        if (garis[1] != '\0') {
            isi_galat(galat, ukuran_galat, "alamat tidak boleh memuat jalur");
            return 0;
        }
        *garis = '\0';
    }
    if (*p == '\0') {
        isi_galat(galat, ukuran_galat, "host kosong");
        return 0;
    }
    for (i = 0; p[i] != '\0'; i++) {
        unsigned char c = (unsigned char)p[i];
        if (!(isalnum(c) || c == '.' || c == '-' || c == '_' || c == ':' || c == '[' ||
              c == ']')) {
            isi_galat(galat, ukuran_galat, "host memuat karakter yang tidak diizinkan");
            return 0;
        }
    }
    port = https ? 443 : 80;
    if (p[0] == '[') {
        char *tutup = strchr(p, ']');
        if (tutup == NULL || tutup == p + 1) {
            isi_galat(galat, ukuran_galat, "alamat IPv6 tidak valid");
            return 0;
        }
        if (tutup[1] == ':') {
            if (!bagian_port(tutup + 2, &port)) {
                isi_galat(galat, ukuran_galat, "port tidak valid");
                return 0;
            }
        } else if (tutup[1] != '\0') {
            isi_galat(galat, ukuran_galat, "alamat IPv6 tidak valid");
            return 0;
        }
        *tutup = '\0';
        if (!salin_batas(alamat->nama_host, sizeof alamat->nama_host, p + 1)) {
            isi_galat(galat, ukuran_galat, "host terlalu panjang");
            return 0;
        }
        *tutup = ']';
    } else {
        char *titik_dua = strchr(p, ':');
        if (titik_dua != NULL) {
            if (strchr(titik_dua + 1, ':') != NULL) {
                isi_galat(galat, ukuran_galat, "IPv6 harus memakai tanda kurung siku");
                return 0;
            }
            if (!bagian_port(titik_dua + 1, &port)) {
                isi_galat(galat, ukuran_galat, "port tidak valid");
                return 0;
            }
            *titik_dua = '\0';
            if (!salin_batas(alamat->nama_host, sizeof alamat->nama_host, p)) {
                isi_galat(galat, ukuran_galat, "host terlalu panjang");
                return 0;
            }
            *titik_dua = ':';
        } else {
            if (!salin_batas(alamat->nama_host, sizeof alamat->nama_host, p)) {
                isi_galat(galat, ukuran_galat, "host terlalu panjang");
                return 0;
            }
        }
        if (alamat->nama_host[0] == '\0') {
            isi_galat(galat, ukuran_galat, "host kosong");
            return 0;
        }
    }
    if (!salin_batas(alamat->host_dan_port, sizeof alamat->host_dan_port, p)) {
        isi_galat(galat, ukuran_galat, "host terlalu panjang");
        return 0;
    }
    alamat->https = https;
    alamat->port = port;
    return 1;
}

#ifndef _WIN32
static char *salin_teks(const char *teks)
{
    size_t n = strlen(teks) + 1;
    char *salinan = malloc(n);

    if (salinan != NULL) {
        memcpy(salinan, teks, n);
    }
    return salinan;
}
#endif

/* Mengembalikan salinan nilai (UTF-8) atau NULL bila belum diatur atau kosong. */
static char *ambil_lingkungan(const char *nama)
{
#ifdef _WIN32
    wchar_t nama_lebar[64];
    const wchar_t *nilai;
    size_t i;
    int n;
    char *keluar;

    for (i = 0; nama[i] != '\0' && i < 63; i++) {
        nama_lebar[i] = (wchar_t)(unsigned char)nama[i];
    }
    nama_lebar[i] = 0;
    nilai = _wgetenv(nama_lebar);
    if (nilai == NULL || nilai[0] == 0) {
        return NULL;
    }
    n = WideCharToMultiByte(CP_UTF8, 0, nilai, -1, NULL, 0, NULL, NULL);
    if (n <= 1) {
        return NULL;
    }
    keluar = malloc((size_t)n);
    if (keluar == NULL) {
        return NULL;
    }
    WideCharToMultiByte(CP_UTF8, 0, nilai, -1, keluar, n, NULL, NULL);
    return keluar;
#else
    const char *nilai = getenv(nama);
    if (nilai == NULL || nilai[0] == '\0') {
        return NULL;
    }
    return salin_teks(nilai);
#endif
}

static void hapus_aman(char *teks)
{
    volatile char *p = teks;

    if (teks == NULL) {
        return;
    }
    while (*p != '\0') {
        *p++ = '\0';
    }
    free(teks);
}

void konfigurasi_bebaskan(konfigurasi_t *konfigurasi)
{
    hapus_aman(konfigurasi->nama_pengguna);
    hapus_aman(konfigurasi->kata_sandi);
    memset(konfigurasi, 0, sizeof *konfigurasi);
}

static void tambah_nama(char *daftar, size_t ukuran, const char *nama)
{
    size_t n = strlen(daftar);

    if (n > 0 && n + 2 < ukuran) {
        daftar[n++] = ',';
        daftar[n++] = ' ';
        daftar[n] = '\0';
    }
    if (n + strlen(nama) + 1 <= ukuran) {
        memcpy(daftar + n, nama, strlen(nama) + 1);
    }
}

int konfigurasi_muat(konfigurasi_t *konfigurasi, char *galat, size_t ukuran_galat)
{
    char *teks_api;
    char *teks_nas;
    char kurang[160] = "";
    char sebab[96];

    memset(konfigurasi, 0, sizeof *konfigurasi);
    teks_api = ambil_lingkungan(LINGKUNGAN_SERVER_API);
    teks_nas = ambil_lingkungan(LINGKUNGAN_SERVER_NAS);
    konfigurasi->nama_pengguna = ambil_lingkungan(LINGKUNGAN_NAMA_PENGGUNA);
    konfigurasi->kata_sandi = ambil_lingkungan(LINGKUNGAN_KATA_SANDI);

    if (teks_api == NULL) {
        tambah_nama(kurang, sizeof kurang, LINGKUNGAN_SERVER_API);
    }
    if (teks_nas == NULL) {
        tambah_nama(kurang, sizeof kurang, LINGKUNGAN_SERVER_NAS);
    }
    if (konfigurasi->nama_pengguna == NULL) {
        tambah_nama(kurang, sizeof kurang, LINGKUNGAN_NAMA_PENGGUNA);
    }
    if (konfigurasi->kata_sandi == NULL) {
        tambah_nama(kurang, sizeof kurang, LINGKUNGAN_KATA_SANDI);
    }
    if (kurang[0] != '\0') {
        snprintf(sebab, sizeof sebab, "variabel lingkungan belum diatur: ");
        if (galat != NULL && ukuran_galat > 0) {
            snprintf(galat, ukuran_galat, "%s%s", sebab, kurang);
        }
        goto gagal;
    }
    {
        char rincian[96];
        if (!alamat_urai(teks_api, 0, &konfigurasi->server_api, rincian, sizeof rincian)) {
            if (galat != NULL && ukuran_galat > 0) {
                snprintf(galat, ukuran_galat, "%s tidak valid: %s", LINGKUNGAN_SERVER_API,
                         rincian);
            }
            goto gagal;
        }
        if (!alamat_urai(teks_nas, 1, &konfigurasi->server_nas, rincian, sizeof rincian)) {
            if (galat != NULL && ukuran_galat > 0) {
                snprintf(galat, ukuran_galat, "%s tidak valid: %s", LINGKUNGAN_SERVER_NAS,
                         rincian);
            }
            goto gagal;
        }
    }
    free(teks_api);
    free(teks_nas);
    return 1;

gagal:
    free(teks_api);
    free(teks_nas);
    konfigurasi_bebaskan(konfigurasi);
    return 0;
}
