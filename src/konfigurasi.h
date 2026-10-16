/* konfigurasi: kredensial dari variabel lingkungan. Hanya empat nilai;
   id_spreadsheet bukan kredensial dan diberikan per panggilan alat. */
#ifndef KONFIGURASI_H
#define KONFIGURASI_H

#include <stddef.h>

#define LINGKUNGAN_SERVER_API "SYNO_SHEET_API_SERVER"
#define LINGKUNGAN_SERVER_NAS "SYNO_SERVER"
#define LINGKUNGAN_NAMA_PENGGUNA "SYNO_USER"
#define LINGKUNGAN_KATA_SANDI "SYNO_PASS"

typedef struct {
    int https;
    int port;
    char nama_host[256];     /* tanpa kurung untuk IPv6 */
    char host_dan_port[280]; /* seperti dimasukkan, tanpa protokol */
} alamat_t;

typedef struct {
    alamat_t server_api;
    alamat_t server_nas;
    char *nama_pengguna;
    char *kata_sandi;
} konfigurasi_t;

/* Mengurai "[http(s)://]host[:port][/]". Mengembalikan 1 bila berhasil; 0 bila gagal. */
int alamat_urai(const char *teks, int https_bawaan, alamat_t *alamat, char *galat,
                size_t ukuran_galat);

/* Memuat keempat variabel. Pesan galat hanya memuat nama variabel, tidak pernah nilainya. */
int konfigurasi_muat(konfigurasi_t *konfigurasi, char *galat, size_t ukuran_galat);
void konfigurasi_bebaskan(konfigurasi_t *konfigurasi);

#endif
