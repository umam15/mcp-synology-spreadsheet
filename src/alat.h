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

#endif
