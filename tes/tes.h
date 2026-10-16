/* tes: pustaka mini untuk tes inti (tanpa dependensi luar). */
#ifndef TES_H
#define TES_H

#include <string.h>

extern int tes_jumlah_periksa;
extern int tes_jumlah_gagal;
/* Profil pengguna palsu (USERPROFILE/HOME) yang dipasang tes_utama; tidak dibuat di disk. */
extern char tes_dir_profil[512];

void tes_catat_gagal(const char *berkas, int baris, const char *ekspresi);
void tes_catat_beda_teks(const char *berkas, int baris, const char *sebenarnya,
                         const char *harapan);
void tes_catat_tanpa_bagian(const char *berkas, int baris, const char *teks,
                            const char *bagian);

/* Mengatur variabel lingkungan; nilai NULL menghapusnya. */
void tes_atur_lingkungan(const char *nama, const char *nilai);

#define PERIKSA(kondisi)                                   \
    do {                                                   \
        tes_jumlah_periksa++;                              \
        if (!(kondisi)) {                                  \
            tes_catat_gagal(__FILE__, __LINE__, #kondisi); \
        }                                                  \
    } while (0)

#define PERIKSA_TEKS(sebenarnya, harapan)                            \
    do {                                                             \
        const char *tes_s_ = (sebenarnya);                           \
        const char *tes_h_ = (harapan);                              \
        tes_jumlah_periksa++;                                        \
        if (tes_s_ == NULL || strcmp(tes_s_, tes_h_) != 0) {         \
            tes_catat_beda_teks(__FILE__, __LINE__, tes_s_, tes_h_); \
        }                                                            \
    } while (0)

#define PERIKSA_MENGANDUNG(teks, bagian)                                \
    do {                                                                \
        const char *tes_t_ = (teks);                                    \
        const char *tes_b_ = (bagian);                                  \
        tes_jumlah_periksa++;                                           \
        if (tes_t_ == NULL || strstr(tes_t_, tes_b_) == NULL) {         \
            tes_catat_tanpa_bagian(__FILE__, __LINE__, tes_t_, tes_b_); \
        }                                                               \
    } while (0)

#define PERIKSA_TIDAK_MENGANDUNG(teks, bagian)                                     \
    do {                                                                           \
        const char *tes_t_ = (teks);                                               \
        const char *tes_b_ = (bagian);                                             \
        tes_jumlah_periksa++;                                                      \
        if (tes_t_ != NULL && strstr(tes_t_, tes_b_) != NULL) {                    \
            tes_catat_gagal(__FILE__, __LINE__, "teks memuat bagian terlarang");   \
        }                                                                          \
    } while (0)

void tes_periksa_penyangga(void);
void tes_periksa_json(void);
void tes_periksa_pengodean(void);
void tes_periksa_konfigurasi(void);
void tes_periksa_alat(void);

#endif
