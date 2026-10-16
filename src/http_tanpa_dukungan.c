/* Pengganti sementara untuk platform selain Windows: kompilasi berhasil, tetapi
   permintaan HTTP gagal dengan pesan jelas. Backend libcurl direncanakan di v0.3.0. */
#include "http.h"

#include <stdio.h>

int http_kirim(const alamat_t *alamat, const char *metode, const char *jalur, const char *token,
               const char *badan, size_t panjang_badan, respons_http_t *respons, char *galat,
               size_t ukuran_galat)
{
    (void)alamat;
    (void)metode;
    (void)jalur;
    (void)token;
    (void)badan;
    (void)panjang_badan;
    respons->kode_status = 0;
    if (galat != NULL && ukuran_galat > 0) {
        snprintf(galat, ukuran_galat,
                 "Klien HTTP belum tersedia di platform ini (rencana v0.3.0, lihat TODO.md)");
    }
    return 0;
}

void http_tutup(void)
{
}
