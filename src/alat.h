/* alat: definisi dan penangan alat MCP. */
#ifndef ALAT_H
#define ALAT_H

#include "json.h"
#include "penyangga.h"

/* Mengembalikan 1 bila berhasil; 0 bila galat. Teks untuk model ditulis ke hasil
   (isi hasil atau pesan galat). argumen boleh NULL. */
typedef int (*penangan_alat_t)(const dokumen_json_t *dok, const simpul_json_t *argumen,
                               penyangga_t *hasil);

typedef struct {
    const char *nama;
    const char *deskripsi;
    const char *skema_masukan; /* JSON Schema sebagai teks */
    int hanya_baca;
    int merusak;
    penangan_alat_t penangan;
} definisi_alat_t;

int alat_jumlah(void);
const definisi_alat_t *alat_cari(const char *nama);
/* Menulis larik JSON berisi seluruh alat (untuk tools/list). */
void alat_tulis_daftar(penyangga_t *keluar);

/* Hemat token: hasil alat dibatasi sebesar BATAS_HASIL_BYTE (dengan catatan bila terpotong). */
#define BATAS_HASIL_BYTE 24000
/* Menambah badan ke hasil; dipotong pada batas di tepi karakter UTF-8. */
void alat_tambah_dibatasi(penyangga_t *hasil, const char *badan, size_t panjang);
/* Meringkas respons API menjadi JSON padat. Mengembalikan 1 bila hasil ditulis; 0 bila badan
   tak dikenali (hasil tidak diubah). */
int alat_ringkas_spreadsheet(const char *badan, size_t panjang, penyangga_t *hasil);
int alat_ringkas_nilai(const char *badan, size_t panjang, penyangga_t *hasil);

#endif
