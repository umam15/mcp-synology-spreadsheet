/* json: pengurai JSON kecil dan ketat. Simpul menyimpan rentang mentah pada
   teks sumber, sehingga bagian (mis. larik nilai sel) bisa diteruskan apa adanya. */
#ifndef JSON_H
#define JSON_H

#include <stddef.h>

typedef enum {
    JSON_KOSONG,
    JSON_LOGIKA,
    JSON_ANGKA,
    JSON_TEKS,
    JSON_LARIK,
    JSON_OBJEK
} jenis_json_t;

typedef struct {
    jenis_json_t jenis;
    size_t awal;  /* awal teks mentah (teks: termasuk tanda kutip) */
    size_t akhir; /* akhir teks mentah (eksklusif) */
    int anak_pertama;
    int saudara_berikut;
    int punya_kunci;
    size_t kunci_awal; /* isi kunci tanpa tanda kutip */
    size_t kunci_akhir;
} simpul_json_t;

typedef struct {
    const char *sumber; /* tidak dimiliki; harus tetap hidup selama dokumen dipakai */
    size_t panjang;
    simpul_json_t *simpul;
    int jumlah;
    int kapasitas;
    char galat[96];
} dokumen_json_t;

#define JSON_KEDALAMAN_MAKS 64
#define JSON_SIMPUL_MAKS 1000000

/* Mengembalikan 1 bila berhasil; 0 bila gagal (lihat dok->galat). */
int json_urai(dokumen_json_t *dok, const char *sumber, size_t panjang);
void json_bebaskan(dokumen_json_t *dok);

const simpul_json_t *json_akar(const dokumen_json_t *dok);
/* NULL bila objek bukan JSON_OBJEK atau kunci tidak ada. Objek boleh NULL. */
const simpul_json_t *json_cari(const dokumen_json_t *dok, const simpul_json_t *objek,
                               const char *kunci);

/* Teks terbuka (unescape) hasil malloc; NULL bila bukan JSON_TEKS. U+0000 diganti U+FFFD. */
char *json_teks_salin(const dokumen_json_t *dok, const simpul_json_t *simpul);
/* Teks mentah simpul pada sumber. */
void json_mentah(const dokumen_json_t *dok, const simpul_json_t *simpul, const char **awal,
                 size_t *panjang);
/* 1 bila simpul logika; nilainya di *hasil. */
int json_logika(const dokumen_json_t *dok, const simpul_json_t *simpul, int *hasil);
/* 1 bila simpul angka bulat yang muat di long; nilainya di *hasil. */
int json_bulat(const dokumen_json_t *dok, const simpul_json_t *simpul, long *hasil);

#endif
