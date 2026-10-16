#include "konfigurasi.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

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


/* ---------- berkas konfigurasi ---------- */

#define BERKAS_UKURAN_MAKS (16 * 1024)
#define TANDA_BERKAS_BERMASALAH 2ULL /* genap, sehingga tidak bisa sama dengan tanda isi */

static void hapus_aman_n(char *data, size_t panjang)
{
    volatile char *p = data;
    size_t i;

    for (i = 0; i < panjang; i++) {
        p[i] = '\0';
    }
}

/* Membaca berkas konfigurasi. 0 = tidak ada; 1 = terbaca (*isi milik pemanggil, berakhir NUL);
   -1 = ada tetapi tidak dapat dibaca atau terlalu besar (galat terisi). */
static int baca_berkas(char **isi, size_t *panjang, char *galat, size_t ukuran_galat)
{
    FILE *f;
    char *buf;
    size_t n;

#ifdef _WIN32
    static const wchar_t sisa[] = L"\\.config\\mcp-synology-spreadsheet\\config.json";
    wchar_t jalur[1024];
    const wchar_t *profil = _wgetenv(L"USERPROFILE");

    if (profil == NULL || profil[0] == 0 || wcslen(profil) + wcslen(sisa) + 1 > 1024) {
        return 0;
    }
    wcscpy(jalur, profil);
    wcscat(jalur, sisa);
    f = _wfopen(jalur, L"rb");
#else
    static const char sisa[] = "/.config/mcp-synology-spreadsheet/config.json";
    char jalur[1024];
    const char *profil = getenv("HOME");

    if (profil == NULL || profil[0] == '\0' || strlen(profil) + strlen(sisa) + 1 > sizeof jalur) {
        return 0;
    }
    strcpy(jalur, profil);
    strcat(jalur, sisa);
    f = fopen(jalur, "rb");
#endif
    if (f == NULL) {
        if (errno == ENOENT || errno == ENOTDIR) {
            return 0;
        }
        isi_galat(galat, ukuran_galat, "berkas konfigurasi ada tetapi tidak dapat dibuka");
        return -1;
    }
    buf = malloc(BERKAS_UKURAN_MAKS + 1);
    if (buf == NULL) {
        fclose(f);
        isi_galat(galat, ukuran_galat, "memori tidak cukup");
        return -1;
    }
    n = fread(buf, 1, BERKAS_UKURAN_MAKS + 1, f);
    if (ferror(f) || n > BERKAS_UKURAN_MAKS) {
        const char *sebab = ferror(f) ? "berkas konfigurasi tidak dapat dibaca"
                                      : "berkas konfigurasi terlalu besar";

        hapus_aman_n(buf, BERKAS_UKURAN_MAKS + 1);
        free(buf);
        fclose(f);
        isi_galat(galat, ukuran_galat, sebab);
        return -1;
    }
    fclose(f);
    buf[n] = '\0';
    *isi = buf;
    *panjang = n;
    return 1;
}

static unsigned long long hitung_tanda(const char *data, size_t panjang)
{
    unsigned long long h = 14695981039346656037ULL; /* FNV-1a 64 bit */
    size_t i;

    for (i = 0; i < panjang; i++) {
        h ^= (unsigned char)data[i];
        h *= 1099511628211ULL;
    }
    return h | 1ULL;
}

unsigned long long konfigurasi_tanda(void)
{
    char *isi = NULL;
    size_t n = 0;
    unsigned long long tanda;
    int status = baca_berkas(&isi, &n, NULL, 0);

    if (status == 0) {
        return 0;
    }
    if (status < 0) {
        return TANDA_BERKAS_BERMASALAH;
    }
    tanda = hitung_tanda(isi, n);
    hapus_aman_n(isi, n);
    free(isi);
    return tanda;
}

/* Salinan nilai teks atau NULL bila kosong/tidak ada; salah jenis dicatat di salah. */
static char *teks_kunci(const dokumen_json_t *dok, const simpul_json_t *akar, const char *kunci,
                        char *salah, size_t ukuran_salah)
{
    const simpul_json_t *s = json_cari(dok, akar, kunci);
    char *teks;

    if (s == NULL || s->jenis == JSON_KOSONG) {
        return NULL;
    }
    if (s->jenis != JSON_TEKS) {
        tambah_nama(salah, ukuran_salah, kunci);
        return NULL;
    }
    teks = json_teks_salin(dok, s);
    if (teks != NULL && teks[0] == '\0') {
        free(teks);
        return NULL;
    }
    return teks;
}

static int muat_dari_json(konfigurasi_t *konfigurasi, const char *isi, size_t panjang, char *galat,
                          size_t ukuran_galat)
{
    dokumen_json_t dok;
    const simpul_json_t *akar;
    char *teks_api;
    char *teks_nas;
    char salah[160] = "";
    char kurang[160] = "";
    char rincian[96];
    int ok = 0;

    if (panjang >= 3 && (unsigned char)isi[0] == 0xEF && (unsigned char)isi[1] == 0xBB &&
        (unsigned char)isi[2] == 0xBF) {
        isi += 3; /* BOM UTF-8 dari beberapa editor */
        panjang -= 3;
    }
    if (!json_urai(&dok, isi, panjang)) {
        if (galat != NULL && ukuran_galat > 0) {
            snprintf(galat, ukuran_galat, "berkas konfigurasi bukan JSON valid: %s", dok.galat);
        }
        return 0;
    }
    akar = json_akar(&dok);
    if (akar->jenis != JSON_OBJEK) {
        json_bebaskan(&dok);
        isi_galat(galat, ukuran_galat, "berkas konfigurasi harus berupa objek JSON");
        return 0;
    }
    teks_api = teks_kunci(&dok, akar, KUNCI_SERVER_API, salah, sizeof salah);
    teks_nas = teks_kunci(&dok, akar, KUNCI_SERVER_NAS, salah, sizeof salah);
    konfigurasi->nama_pengguna = teks_kunci(&dok, akar, KUNCI_NAMA_PENGGUNA, salah, sizeof salah);
    konfigurasi->kata_sandi = teks_kunci(&dok, akar, KUNCI_KATA_SANDI, salah, sizeof salah);
    json_bebaskan(&dok);

    if (salah[0] != '\0') {
        if (galat != NULL && ukuran_galat > 0) {
            snprintf(galat, ukuran_galat, "berkas konfigurasi: nilai harus berupa teks: %s",
                     salah);
        }
        goto selesai;
    }
    if (teks_api == NULL) {
        tambah_nama(kurang, sizeof kurang, KUNCI_SERVER_API);
    }
    if (teks_nas == NULL) {
        tambah_nama(kurang, sizeof kurang, KUNCI_SERVER_NAS);
    }
    if (konfigurasi->nama_pengguna == NULL) {
        tambah_nama(kurang, sizeof kurang, KUNCI_NAMA_PENGGUNA);
    }
    if (konfigurasi->kata_sandi == NULL) {
        tambah_nama(kurang, sizeof kurang, KUNCI_KATA_SANDI);
    }
    if (kurang[0] != '\0') {
        if (galat != NULL && ukuran_galat > 0) {
            snprintf(galat, ukuran_galat, "berkas konfigurasi belum diisi: %s", kurang);
        }
        goto selesai;
    }
    if (!alamat_urai(teks_api, 0, &konfigurasi->server_api, rincian, sizeof rincian)) {
        if (galat != NULL && ukuran_galat > 0) {
            snprintf(galat, ukuran_galat, "%s tidak valid: %s", KUNCI_SERVER_API, rincian);
        }
        goto selesai;
    }
    if (!alamat_urai(teks_nas, 1, &konfigurasi->server_nas, rincian, sizeof rincian)) {
        if (galat != NULL && ukuran_galat > 0) {
            snprintf(galat, ukuran_galat, "%s tidak valid: %s", KUNCI_SERVER_NAS, rincian);
        }
        goto selesai;
    }
    ok = 1;

selesai:
    free(teks_api);
    free(teks_nas);
    if (!ok) {
        konfigurasi_bebaskan(konfigurasi);
    }
    return ok;
}

int konfigurasi_muat_semua(konfigurasi_t *konfigurasi, unsigned long long *tanda, char *galat,
                           size_t ukuran_galat)
{
    char *isi = NULL;
    size_t n = 0;
    int status;
    int ok;

    status = baca_berkas(&isi, &n, galat, ukuran_galat);
    if (status == 0) {
        if (tanda != NULL) {
            *tanda = 0;
        }
        return konfigurasi_muat(konfigurasi, galat, ukuran_galat);
    }
    memset(konfigurasi, 0, sizeof *konfigurasi);
    if (status < 0) {
        if (tanda != NULL) {
            *tanda = TANDA_BERKAS_BERMASALAH;
        }
        return 0;
    }
    if (tanda != NULL) {
        *tanda = hitung_tanda(isi, n);
    }
    ok = muat_dari_json(konfigurasi, isi, n, galat, ukuran_galat);
    hapus_aman_n(isi, n);
    free(isi);
    return ok;
}
