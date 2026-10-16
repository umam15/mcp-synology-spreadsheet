
#include <stdlib.h>
#include <string.h>

#include "alat.h"
#include "json.h"
#include "penyangga.h"
#include "tes.h"

#define JUMLAH_ALAT 14
/* Hemat token: batas atas ukuran tools/list (byte); naikkan hanya dengan alasan kuat. */
#define BATAS_DAFTAR_BYTE 6400

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

/* Hemat token: ukuran daftar alat dan keluaran ringkas. */
static void periksa_hemat_token(void)
{
    penyangga_t daftar;
    penyangga_t hasil;
    char *besar;
    const char *isi;
    const char *potong;
    size_t i;

    penyangga_siapkan(&daftar);
    alat_tulis_daftar(&daftar);
    PERIKSA(daftar.panjang <= BATAS_DAFTAR_BYTE);
    penyangga_bebaskan(&daftar);

    /* ambil_spreadsheet: hanya bidang berguna */
    penyangga_siapkan(&hasil);
    PERIKSA(alat_ringkas_spreadsheet(
                "{\"id\":\"abc\",\"properties\":{\"title\":\"dev\",\"locale\":\"en_US\"},\"sheets\":["
                "{\"properties\":{\"title\":\"Sheet1\",\"sheetId\":\"sh_1\",\"index\":0,"
                "\"hidden\":false},\"rowCount\":4,\"colCount\":29,\"fixedColumnLeft\":0,"
                "\"fixedRowTop\":0,\"mergeCells\":[]},"
                "{\"properties\":{\"title\":\"Arsip\",\"sheetId\":\"sh_2\",\"index\":1,"
                "\"hidden\":true},\"rowCount\":2,\"colCount\":3,\"mergeCells\":[]}]}",
                strlen("{\"id\":\"abc\",\"properties\":{\"title\":\"dev\",\"locale\":\"en_US\"},"
                       "\"sheets\":[{\"properties\":{\"title\":\"Sheet1\",\"sheetId\":\"sh_1\","
                       "\"index\":0,\"hidden\":false},\"rowCount\":4,\"colCount\":29,"
                       "\"fixedColumnLeft\":0,\"fixedRowTop\":0,\"mergeCells\":[]},"
                       "{\"properties\":{\"title\":\"Arsip\",\"sheetId\":\"sh_2\",\"index\":1,"
                       "\"hidden\":true},\"rowCount\":2,\"colCount\":3,\"mergeCells\":[]}]}"),
                &hasil) == 1);
    PERIKSA_TEKS(penyangga_isi(&hasil),
                 "{\"judul\":\"dev\",\"lembar\":[{\"id_lembar\":\"sh_1\",\"nama\":\"Sheet1\","
                 "\"baris\":4,\"kolom\":29},{\"id_lembar\":\"sh_2\",\"nama\":\"Arsip\","
                 "\"baris\":2,\"kolom\":3,\"tersembunyi\":true}]}");
    penyangga_bebaskan(&hasil);

    /* bentuk tak dikenal: hasil tidak diubah */
    penyangga_siapkan(&hasil);
    PERIKSA(alat_ringkas_spreadsheet("{\"x\":1}", 7, &hasil) == 0);
    PERIKSA(alat_ringkas_spreadsheet("bukan json", 10, &hasil) == 0);
    PERIKSA(hasil.panjang == 0);
    penyangga_bebaskan(&hasil);

    /* baca_nilai: hanya larik nilai */
    penyangga_siapkan(&hasil);
    PERIKSA(alat_ringkas_nilai("{\"range\":\"A1:B1\",\"majorDimension\":\"ROWS\","
                               "\"values\":[[\"a\",1]]}",
                               strlen("{\"range\":\"A1:B1\",\"majorDimension\":\"ROWS\","
                                      "\"values\":[[\"a\",1]]}"),
                               &hasil) == 1);
    PERIKSA_TEKS(penyangga_isi(&hasil), "[[\"a\",1]]");
    penyangga_bebaskan(&hasil);

    penyangga_siapkan(&hasil);
    PERIKSA(alat_ringkas_nilai("{\"range\":\"A1\"}", 14, &hasil) == 1);
    PERIKSA_TEKS(penyangga_isi(&hasil), "[]");
    penyangga_bebaskan(&hasil);

    penyangga_siapkan(&hasil);
    PERIKSA(alat_ringkas_nilai("[1]", 3, &hasil) == 0);
    PERIKSA(alat_ringkas_nilai("bukan json", 10, &hasil) == 0);
    PERIKSA(hasil.panjang == 0);
    penyangga_bebaskan(&hasil);

    /* batas hasil: tepat di batas tidak dipotong */
    besar = malloc(BATAS_HASIL_BYTE + 2000);
    if (besar == NULL) {
        tes_jumlah_periksa++;
        tes_catat_gagal(__FILE__, __LINE__, "memori tes tidak cukup");
        return;
    }
    memset(besar, 'x', BATAS_HASIL_BYTE);
    penyangga_siapkan(&hasil);
    alat_tambah_dibatasi(&hasil, besar, BATAS_HASIL_BYTE);
    PERIKSA(hasil.panjang == BATAS_HASIL_BYTE);
    penyangga_bebaskan(&hasil);

    /* lebih besar: dipotong dengan catatan, dan ukuran terbatas */
    memset(besar, 'x', BATAS_HASIL_BYTE + 1500);
    penyangga_siapkan(&hasil);
    alat_tambah_dibatasi(&hasil, besar, BATAS_HASIL_BYTE + 1500);
    PERIKSA(hasil.panjang < BATAS_HASIL_BYTE + 200);
    PERIKSA_MENGANDUNG(penyangga_isi(&hasil), "[terpotong:");
    penyangga_bebaskan(&hasil);

    /* pemotongan tidak membelah karakter UTF-8 (a + pasangan 2 byte; batas jatuh di tengah) */
    besar[0] = 'a';
    for (i = 1; i + 1 < BATAS_HASIL_BYTE + 1500; i += 2) {
        besar[i] = (char)0xC3;
        besar[i + 1] = (char)0xA9;
    }
    besar[BATAS_HASIL_BYTE + 1499] = 'z';
    penyangga_siapkan(&hasil);
    alat_tambah_dibatasi(&hasil, besar, BATAS_HASIL_BYTE + 1500);
    isi = penyangga_isi(&hasil);
    potong = strstr(isi, "\n[terpotong:");
    PERIKSA(potong != NULL);
    if (potong != NULL) {
        PERIKSA((unsigned char)potong[-1] == 0xA9);
    }
    penyangga_bebaskan(&hasil);
    free(besar);
}

void tes_periksa_alat(void)
{
    penyangga_t keluar;
    char id_panjang[80];

    periksa_hemat_token();
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
