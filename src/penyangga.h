/* penyangga: wadah teks yang tumbuh otomatis; isinya selalu berakhir NUL. */
#ifndef PENYANGGA_H
#define PENYANGGA_H

#include <stddef.h>

#define BATAS_PENYANGGA_BYTE ((size_t)64 * 1024 * 1024)

typedef struct {
    char *data;
    size_t panjang;
    size_t kapasitas;
    int gagal; /* 1 bila memori habis atau melewati batas */
} penyangga_t;

void penyangga_siapkan(penyangga_t *penyangga);
void penyangga_bebaskan(penyangga_t *penyangga);
void penyangga_kosongkan(penyangga_t *penyangga);

/* Semua fungsi tambah mengembalikan 1 bila berhasil, 0 bila gagal. */
int penyangga_tambah(penyangga_t *penyangga, const char *data, size_t panjang);
int penyangga_tambah_teks(penyangga_t *penyangga, const char *teks);
int penyangga_tambah_format(penyangga_t *penyangga, const char *format, ...);

/* Menambah teks UTF-8 sebagai teks JSON berkutip. Byte UTF-8 tidak valid
   diganti U+FFFD agar keluaran selalu JSON valid. */
int penyangga_tambah_teks_json(penyangga_t *penyangga, const char *teks, size_t panjang);

/* Isi sebagai teks C; "" bila kosong. */
const char *penyangga_isi(const penyangga_t *penyangga);

#endif
