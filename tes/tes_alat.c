
#include <string.h>

#include "alat.h"
#include "json.h"
#include "penyangga.h"
#include "tes.h"

#define JUMLAH_ALAT 14

/* Memanggil penangan alat dengan argumen JSON. Mengembalikan hasil penangan
   (1 berhasil, 0 galat), atau -1 bila alat tidak ada atau argumen bukan JSON. */
static int panggil(const char *nama, const char *argumen_json, penyangga_t *keluar)
{
    const definisi_alat_t *alat = alat_cari(nama);
    dokumen_json_t dok;
    int hasil;

    penyangga_siapkan(keluar);
    PERIKSA(alat != NULL);
    if (alat == NULL) {
        return -1;
    }
    if (!json_urai(&dok, argumen_json, strlen(argumen_json))) {
        tes_jumlah_periksa++;
        tes_catat_gagal(__FILE__, __LINE__, "argumen uji bukan JSON");
        return -1;
    }
    hasil = alat->penangan(&dok, json_akar(&dok), keluar);
    json_bebaskan(&dok);
    return hasil;
}

/* Argumen salah harus ditolak dengan pesan yang memuat bagian_pesan, tanpa menyentuh jaringan. */
static void periksa_ditolak(const char *nama, const char *argumen_json, const char *bagian_pesan)
{
    penyangga_t keluar;

    PERIKSA(panggil(nama, argumen_json, &keluar) == 0);
    PERIKSA_MENGANDUNG(penyangga_isi(&keluar), bagian_pesan);
    penyangga_bebaskan(&keluar);
}

static void periksa_anotasi(const char *nama, int hanya_baca, int merusak)
{
    const definisi_alat_t *alat = alat_cari(nama);

    PERIKSA(alat != NULL);
    if (alat != NULL) {
        PERIKSA(alat->hanya_baca == hanya_baca);
        PERIKSA(alat->merusak == merusak);
    }
}

static int hitung_kemunculan(const char *teks, const char *kata)
{
    int jumlah = 0;
    size_t n = strlen(kata);

    while ((teks = strstr(teks, kata)) != NULL) {
        jumlah++;
        teks += n;
    }
    return jumlah;
}

static void periksa_daftar(void)
{
    static const char *const nama[JUMLAH_ALAT] = {
        "periksa_sambungan", "buat_spreadsheet", "ambil_spreadsheet", "hapus_spreadsheet",
        "baca_nilai",        "tulis_nilai",      "tambah_baris",      "baca_gaya",
        "tulis_gaya",        "tambah_lembar",    "ganti_nama_lembar", "hapus_lembar",
        "ekspor_csv",        "ubah_massal"};
    penyangga_t daftar;
    dokumen_json_t dok;
    int i;

    PERIKSA(alat_jumlah() == JUMLAH_ALAT);
    PERIKSA(alat_cari("alat_tidak_ada") == NULL);

    for (i = 0; i < JUMLAH_ALAT; i++) {
        const definisi_alat_t *alat = alat_cari(nama[i]);
        dokumen_json_t skema;

        PERIKSA(alat != NULL);
        if (alat == NULL) {
            continue;
        }
        PERIKSA(alat->penangan != NULL);
        PERIKSA(alat->deskripsi != NULL && alat->deskripsi[0] != '\0');
        if (json_urai(&skema, alat->skema_masukan, strlen(alat->skema_masukan))) {
            PERIKSA(json_akar(&skema)->jenis == JSON_OBJEK);
            json_bebaskan(&skema);
        } else {
            tes_jumlah_periksa++;
            tes_catat_gagal(__FILE__, __LINE__, "skema masukan bukan JSON valid");
        }
    }

    penyangga_siapkan(&daftar);
    alat_tulis_daftar(&daftar);
    PERIKSA(!daftar.gagal);
    PERIKSA(hitung_kemunculan(penyangga_isi(&daftar), "{\"name\":") == JUMLAH_ALAT);
    if (json_urai(&dok, daftar.data, daftar.panjang)) {
        PERIKSA(json_akar(&dok)->jenis == JSON_LARIK);
        json_bebaskan(&dok);
    } else {
        tes_jumlah_periksa++;
        tes_catat_gagal(__FILE__, __LINE__, "daftar alat bukan JSON valid");
    }
    penyangga_bebaskan(&daftar);

    /* anotasi: hanya alat baca yang hanya_baca; alat yang menimpa atau menghapus merusak */
    periksa_anotasi("periksa_sambungan", 1, 0);
    periksa_anotasi("ambil_spreadsheet", 1, 0);
    periksa_anotasi("baca_nilai", 1, 0);
    periksa_anotasi("baca_gaya", 1, 0);
    periksa_anotasi("ekspor_csv", 1, 0);
    periksa_anotasi("buat_spreadsheet", 0, 0);
    periksa_anotasi("tambah_baris", 0, 0);
    periksa_anotasi("tambah_lembar", 0, 0);
    periksa_anotasi("ganti_nama_lembar", 0, 0);
    periksa_anotasi("hapus_spreadsheet", 0, 1);
    periksa_anotasi("tulis_nilai", 0, 1);
    periksa_anotasi("tulis_gaya", 0, 1);
    periksa_anotasi("hapus_lembar", 0, 1);
    periksa_anotasi("ubah_massal", 0, 1);
}

void tes_periksa_alat(void)
{
    penyangga_t keluar;
    char id_panjang[80];

    periksa_daftar();

    /* tanpa kredensial: galat jelas, tanpa jaringan dan tanpa nilai rahasia */
    PERIKSA(panggil("periksa_sambungan", "{}", &keluar) == 0);
    PERIKSA_MENGANDUNG(penyangga_isi(&keluar), "belum diatur");
    penyangga_bebaskan(&keluar);

    /* argumen wajib */
    periksa_ditolak("ambil_spreadsheet", "{}", "'id_spreadsheet' wajib diisi");
    periksa_ditolak("ambil_spreadsheet", "{\"id_spreadsheet\":123}", "harus berupa teks");
    periksa_ditolak("baca_nilai", "{\"id_spreadsheet\":\"abc123\"}", "'rentang' wajib diisi");
    periksa_ditolak("tulis_nilai", "{\"id_spreadsheet\":\"abc123\",\"rentang\":\"A1\"}",
                    "wajib berupa larik JSON");
    periksa_ditolak("tulis_nilai",
                    "{\"id_spreadsheet\":\"abc123\",\"rentang\":\"A1\",\"nilai\":\"x\"}",
                    "wajib berupa larik JSON");

    /* bentuk ID: hanya huruf dan angka, maksimal 64 karakter (tanpa garis bawah untuk spreadsheet) */
    periksa_ditolak("ambil_spreadsheet", "{\"id_spreadsheet\":\"a/b\"}", "tidak valid");
    periksa_ditolak("ambil_spreadsheet", "{\"id_spreadsheet\":\"a_b\"}", "tidak valid");
    periksa_ditolak("ambil_spreadsheet", "{\"id_spreadsheet\":\"\"}", "tidak valid");
    memset(id_panjang, 'a', 65);
    id_panjang[65] = '\0';
    {
        char argumen[160];

        strcpy(argumen, "{\"id_spreadsheet\":\"");
        strcat(argumen, id_panjang);
        strcat(argumen, "\"}");
        periksa_ditolak("ambil_spreadsheet", argumen, "tidak valid");
    }

    /* karakter kendali pada teks bebas ditolak */
    periksa_ditolak("baca_nilai", "{\"id_spreadsheet\":\"abc123\",\"rentang\":\"A1\\n\"}",
                    "karakter kendali");

    /* indeks harus bilangan bulat >= 0 */
    periksa_ditolak("tulis_gaya",
                    "{\"id_spreadsheet\":\"abc123\",\"nama_lembar\":\"L\",\"baris_awal\":-1}",
                    "bilangan bulat >= 0");

    /* aksi penghapusan tanpa konfirmasi tidak dijalankan */
    periksa_ditolak("hapus_spreadsheet", "{\"id_spreadsheet\":\"abc123\"}", "konfirmasi=true");
    periksa_ditolak("hapus_spreadsheet", "{\"id_spreadsheet\":\"abc123\",\"konfirmasi\":false}",
                    "konfirmasi=true");
    periksa_ditolak("hapus_spreadsheet", "{\"id_spreadsheet\":\"abc123\",\"konfirmasi\":\"true\"}",
                    "konfirmasi=true");
    periksa_ditolak("hapus_lembar", "{\"id_spreadsheet\":\"abc123\",\"id_lembar\":\"s1\"}",
                    "konfirmasi=true");
}
