/* tes_utama: titik masuk tes inti dan pembantu bersama. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>

#include "tes.h"

int tes_jumlah_periksa = 0;
int tes_jumlah_gagal = 0;

void tes_catat_gagal(const char *berkas, int baris, const char *ekspresi)
{
    tes_jumlah_gagal++;
    fprintf(stderr, "GAGAL %s:%d: %s\n", berkas, baris, ekspresi);
}

void tes_catat_beda_teks(const char *berkas, int baris, const char *sebenarnya,
                         const char *harapan)
{
    tes_jumlah_gagal++;
    fprintf(stderr, "GAGAL %s:%d: teks berbeda\n  sebenarnya: %s\n  harapan:    %s\n", berkas,
            baris, sebenarnya != NULL ? sebenarnya : "(NULL)", harapan);
}

void tes_catat_tanpa_bagian(const char *berkas, int baris, const char *teks, const char *bagian)
{
    tes_jumlah_gagal++;
    fprintf(stderr, "GAGAL %s:%d: teks tidak memuat \"%s\"\n  teks: %s\n", berkas, baris, bagian,
            teks != NULL ? teks : "(NULL)");
}

void tes_atur_lingkungan(const char *nama, const char *nilai)
{
#ifdef _WIN32
    _putenv_s(nama, nilai != NULL ? nilai : ""); /* nilai kosong = hapus */
#else
    if (nilai != NULL) {
        setenv(nama, nilai, 1);
    } else {
        unsetenv(nama);
    }
#endif
}

int main(void)
{
    tes_atur_lingkungan("SYNO_SHEET_API_SERVER", NULL);
    tes_atur_lingkungan("SYNO_SERVER", NULL);
    tes_atur_lingkungan("SYNO_USER", NULL);
    tes_atur_lingkungan("SYNO_PASS", NULL);

    tes_periksa_penyangga();
    tes_periksa_json();
    tes_periksa_pengodean();
    tes_periksa_konfigurasi();
    tes_periksa_alat();

    printf("%d pemeriksaan, %d gagal\n", tes_jumlah_periksa, tes_jumlah_gagal);
    return tes_jumlah_gagal == 0 ? 0 : 1;
}
