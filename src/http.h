/* http: antarmuka klien HTTP. Implementasi dipilih saat build per platform:
   http_windows.c (WinHTTP) atau http_tanpa_dukungan.c (sementara). */
#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#include "konfigurasi.h"
#include "penyangga.h"

#define BATAS_RESPONS_BYTE ((size_t)8 * 1024 * 1024)

typedef struct {
    int kode_status;
    penyangga_t badan;
} respons_http_t;

/* Mengirim satu permintaan. token boleh NULL; badan boleh NULL (tanpa badan).
   Badan dikirim sebagai application/json. Mengembalikan 1 bila respons diterima
   (apa pun kode statusnya); 0 bila gagal jaringan (pesan di galat).
   respons->badan harus disiapkan pemanggil dan dibebaskan pemanggil. */
int http_kirim(const alamat_t *alamat, const char *metode, const char *jalur, const char *token,
               const char *badan, size_t panjang_badan, respons_http_t *respons, char *galat,
               size_t ukuran_galat);

/* Menutup sumber daya HTTP global; aman dipanggil berulang. */
void http_tutup(void);

#endif
