
#include <stdlib.h>
#include <string.h>

#include "alat.h"
#include "json.h"
#include "klien_api.h"
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
        besar[i] = '\xC3';
        besar[i + 1] = '\xA9';
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

typedef int (*fungsi_ringkas_t)(const char *, size_t, penyangga_t *);

static void periksa_ringkas(fungsi_ringkas_t fn, const char *masukan, const char *harapan)
{
    penyangga_t hasil;

    penyangga_siapkan(&hasil);
    PERIKSA(fn(masukan, strlen(masukan), &hasil) == 1);
    PERIKSA_TEKS(penyangga_isi(&hasil), harapan);
    PERIKSA(hasil.panjang * 2 < strlen(masukan) || strlen(masukan) < 120);
    penyangga_bebaskan(&hasil);
}

static void periksa_ringkas_ditolak(fungsi_ringkas_t fn)
{
    static const char *const bentuk[] = {"bukan json", "{\"x\":1}", "[1]"};
    size_t i;

    for (i = 0; i < sizeof bentuk / sizeof bentuk[0]; i++) {
        penyangga_t hasil;

        penyangga_siapkan(&hasil);
        PERIKSA(fn(bentuk[i], strlen(bentuk[i]), &hasil) == 0);
        PERIKSA(hasil.panjang == 0);
        penyangga_bebaskan(&hasil);
    }
}

/* Hemat token tahap 2: keluaran ringkas alat tulis, lembar, dan gaya. */
static void periksa_ringkas_tahap2(void)
{
    penyangga_t hasil;

    periksa_ringkas(alat_ringkas_tulis,
                    "{\"range\":\"A1:C3\",\"majorDimension\":\"ROWS\",\"values\":[[\"nama\",\"qty\","
                    "\"total\"],[\"Baut\",10,\"=B2*1500\"],[\"Oli\",3,\"=B3*45000\"]]}",
                    "{\"rentang\":\"A1:C3\",\"baris\":3,\"kolom\":3}");
    periksa_ringkas(alat_ringkas_tambah_baris,
                    "{\"spreadsheetId\":\"x\",\"tableRange\":\"Sheet1!A1:C3\",\"updates\":{"
                    "\"updateRange\":\"Sheet1!A4:C4\",\"updateRows\":1,\"updateColumns\":3}}",
                    "{\"rentang\":\"Sheet1!A4:C4\",\"baris\":1,\"kolom\":3}");
    periksa_ringkas(alat_ringkas_tambah_lembar,
                    "{\"addSheet\":{\"properties\":{\"sheetId\":\"sh_2\",\"title\":\"Cadangan\","
                    "\"index\":1}}}",
                    "{\"id_lembar\":\"sh_2\",\"nama\":\"Cadangan\"}");
    periksa_ringkas(alat_ringkas_ganti_nama,
                    "{\"spreadsheetId\":\"x\",\"sheetId\":\"sh_2\",\"sheetName\":\"Arsip\"}",
                    "{\"id_lembar\":\"sh_2\",\"nama\":\"Arsip\"}");
    periksa_ringkas(alat_ringkas_berhasil, "{\"spreadsheetId\":\"x\"}", "Berhasil");
    periksa_ringkas(
        alat_ringkas_gaya,
        "{\"range\":\"A1:B2\",\"rows\":[{\"values\":[{\"userEnteredValue\":\"nama\","
        "\"effectiveValue\":\"nama\",\"formattedValue\":\"nama\",\"effectiveFormat\":{"
        "\"numberFormat\":{\"type\":\"DEFAULT\",\"pattern\":\"General\"},\"verticalAlignment\":"
        "\"bottom\",\"textFormat\":{\"name\":\"Arial\",\"size\":10,\"bold\":true,\"italic\":false,"
        "\"color\":\"000000\"},\"bg\":\"fffffe\"},\"userEnteredFormat\":{\"textFormat\":{"
        "\"bold\":true}}},{\"userEnteredValue\":\"=B2*2\",\"effectiveValue\":6,"
        "\"formattedValue\":\"6\",\"effectiveFormat\":{},\"userEnteredFormat\":{}}]},"
        "{\"values\":[{}]}]}",
        "{\"rentang\":\"A1:B2\",\"baris\":[[{\"v\":\"nama\",\"f\":{\"textFormat\":{\"bold\":true}}},"
        "{\"v\":\"=B2*2\",\"h\":6}],[null]]}");

    /* bentuk tak dikenal ditolak tanpa mengubah hasil */
    periksa_ringkas_ditolak(alat_ringkas_tulis);
    periksa_ringkas_ditolak(alat_ringkas_tambah_baris);
    periksa_ringkas_ditolak(alat_ringkas_tambah_lembar);
    periksa_ringkas_ditolak(alat_ringkas_ganti_nama);
    periksa_ringkas_ditolak(alat_ringkas_gaya);

    /* ringkasan tidak memuat bidang bawaan yang tidak berguna */
    penyangga_siapkan(&hasil);
    PERIKSA(alat_ringkas_gaya("{\"range\":\"A1\",\"rows\":[{\"values\":[{\"userEnteredValue\":1,"
                              "\"effectiveValue\":1,\"effectiveFormat\":{\"textFormat\":{"
                              "\"name\":\"Arial\"}}}]}]}",
                              strlen("{\"range\":\"A1\",\"rows\":[{\"values\":[{\"userEnteredValue\":1,"
                                     "\"effectiveValue\":1,\"effectiveFormat\":{\"textFormat\":{"
                                     "\"name\":\"Arial\"}}}]}]}"),
                              &hasil) == 1);
    PERIKSA(strstr(penyangga_isi(&hasil), "Arial") == NULL);
    penyangga_bebaskan(&hasil);
}

/* Hemat token: pesan galat HTTP satu baris tanpa HTML. */
static void periksa_galat_ringkas(void)
{
    char k[320];
    char kecil[6];
    static const char html502[] =
        "<html>\r\n<head><title>502 Bad Gateway</title></head>\r\n<body>\r\n<center><h1>502 Bad "
        "Gateway</h1></center>\r\n<hr><center>openresty</center>\r\n</body>\r\n</html>\r\n";

    api_ringkas_galat(html502, sizeof html502 - 1, k, sizeof k);
    PERIKSA_TEKS(k, "502 Bad Gateway");

    api_ringkas_galat("{\"message\":\"Unexpected error\",\"status\":\"INTERNAL_ERROR\",\"code\":500}",
                      strlen("{\"message\":\"Unexpected error\",\"status\":\"INTERNAL_ERROR\","
                             "\"code\":500}"),
                      k, sizeof k);
    PERIKSA_TEKS(k, "Unexpected error");

    api_ringkas_galat("{\"statusCode\":500,\"error\":\"Internal Server Error\"}",
                      strlen("{\"statusCode\":500,\"error\":\"Internal Server Error\"}"), k,
                      sizeof k);
    PERIKSA_TEKS(k, "Internal Server Error");

    api_ringkas_galat("  baris satu\r\n\tbaris   dua  ", strlen("  baris satu\r\n\tbaris   dua  "), k,
                      sizeof k);
    PERIKSA_TEKS(k, "baris satu baris dua");

    api_ringkas_galat("<html><body><h1>Oops</h1> teks</body></html>",
                      strlen("<html><body><h1>Oops</h1> teks</body></html>"), k, sizeof k);
    PERIKSA_TEKS(k, "Oops teks");

    api_ringkas_galat("502", 3, k, sizeof k); /* JSON bukan objek: pakai teks polos */
    PERIKSA_TEKS(k, "502");

    api_ringkas_galat("", 0, k, sizeof k);
    PERIKSA_TEKS(k, "(tanpa isi)");
    api_ringkas_galat("   \r\n ", 6, k, sizeof k);
    PERIKSA_TEKS(k, "(tanpa isi)");

    /* panjang dibatasi */
    {
        char panjang_x[1001];

        memset(panjang_x, 'x', 1000);
        panjang_x[1000] = '\0';
        api_ringkas_galat(panjang_x, 1000, k, sizeof k);
        PERIKSA(strlen(k) == sizeof k - 1);
    }

    /* tidak membelah karakter UTF-8 (setiap karakter 2 byte; ruang 5 byte) */
    api_ringkas_galat("\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9", 8, kecil, sizeof kecil);
    PERIKSA_TEKS(kecil, "\xC3\xA9\xC3\xA9");

    /* petunjuk */
    PERIKSA_MENGANDUNG(api_petunjuk_galat(502, "502 Bad Gateway"), "periksa layanan di NAS");
    PERIKSA_MENGANDUNG(api_petunjuk_galat(504, ""), "periksa layanan di NAS");
    PERIKSA_MENGANDUNG(api_petunjuk_galat(500, "Unexpected error"), "sudah dihapus");
    PERIKSA_TEKS(api_petunjuk_galat(500, "kesalahan lain"), "");
    PERIKSA_TEKS(api_petunjuk_galat(404, "Unexpected error"), "");
}

void tes_periksa_alat(void)
{
    penyangga_t keluar;
    char id_panjang[80];

    periksa_galat_ringkas();
    periksa_ringkas_tahap2();
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
