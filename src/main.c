/* main: server MCP lewat stdio (JSON-RPC 2.0, satu pesan per baris di stdin/stdout). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "alat.h"
#include "catat.h"
#include "json.h"
#include "klien_api.h"
#include "penyangga.h"

#define NAMA_SERVER "mcp-synology-spreadsheet"
#ifndef VERSI_SERVER
#define VERSI_SERVER "0.0.0-pengembangan" /* build resmi: diisi CMakeLists.txt */
#endif
#define BATAS_PESAN_BYTE ((size_t)16 * 1024 * 1024)

#define KODE_JSON_TIDAK_VALID (-32700)
#define KODE_PERMINTAAN_TIDAK_VALID (-32600)
#define KODE_METODE_TIDAK_ADA (-32601)
#define KODE_PARAMETER_TIDAK_VALID (-32602)

/* Urutan: yang terbaru lebih dulu. */
static const char *const VERSI_PROTOKOL_DIDUKUNG[] = {"2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05"};
#define JUMLAH_VERSI_PROTOKOL 4

#define RESPONS_MEMORI_HABIS                                                              \
    "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":"            \
    "\"Memori tidak cukup\"}}"

/* Menulis satu pesan ke stdout. Pesan JSON valid tidak memuat baris baru di luar teks
   (di dalam teks sudah di-escape), jadi pengganti spasi di sini aman. */
static void server_kirim(penyangga_t *pesan)
{
    size_t i;

    if (pesan->gagal || pesan->data == NULL) {
        catat_galat("Memori tidak cukup untuk menyusun respons");
        fputs(RESPONS_MEMORI_HABIS "\n", stdout);
        fflush(stdout);
        return;
    }
    for (i = 0; i < pesan->panjang; i++) {
        if (pesan->data[i] == '\n' || pesan->data[i] == '\r') {
            pesan->data[i] = ' ';
        }
    }
    fwrite(pesan->data, 1, pesan->panjang, stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

static void server_kirim_galat(const char *id, int kode_galat, const char *pesan_galat)
{
    penyangga_t pesan;

    penyangga_siapkan(&pesan);
    penyangga_tambah_teks(&pesan, "{\"jsonrpc\":\"2.0\",\"id\":");
    penyangga_tambah_teks(&pesan, id);
    penyangga_tambah_format(&pesan, ",\"error\":{\"code\":%d,\"message\":", kode_galat);
    penyangga_tambah_teks_json(&pesan, pesan_galat, strlen(pesan_galat));
    penyangga_tambah_teks(&pesan, "}}");
    server_kirim(&pesan);
    penyangga_bebaskan(&pesan);
}

/* Menulis awal respons sukses; pemanggil menambah nilai hasil lalu menutup dengan "}". */
static void server_mulai_respons(penyangga_t *pesan, const char *id)
{
    penyangga_siapkan(pesan);
    penyangga_tambah_teks(pesan, "{\"jsonrpc\":\"2.0\",\"id\":");
    penyangga_tambah_teks(pesan, id);
    penyangga_tambah_teks(pesan, ",\"result\":");
}

static void server_kirim_hasil_tetap(const char *id, const char *hasil_json)
{
    penyangga_t pesan;

    server_mulai_respons(&pesan, id);
    penyangga_tambah_teks(&pesan, hasil_json);
    penyangga_tambah_teks(&pesan, "}");
    server_kirim(&pesan);
    penyangga_bebaskan(&pesan);
}

static void server_tangani_inisialisasi(const dokumen_json_t *dok, const simpul_json_t *params,
                                        const char *id)
{
    const char *versi = VERSI_PROTOKOL_DIDUKUNG[0];
    char *diminta = json_teks_salin(dok, json_cari(dok, params, "protocolVersion"));
    penyangga_t pesan;
    int i;

    if (diminta != NULL) {
        for (i = 0; i < JUMLAH_VERSI_PROTOKOL; i++) {
            if (strcmp(diminta, VERSI_PROTOKOL_DIDUKUNG[i]) == 0) {
                versi = VERSI_PROTOKOL_DIDUKUNG[i];
                break;
            }
        }
    }
    free(diminta);
    server_mulai_respons(&pesan, id);
    penyangga_tambah_format(&pesan,
                            "{\"protocolVersion\":\"%s\",\"capabilities\":{\"tools\":{}},"
                            "\"serverInfo\":{\"name\":\"%s\",\"version\":\"%s\"}}}",
                            versi, NAMA_SERVER, VERSI_SERVER);
    server_kirim(&pesan);
    penyangga_bebaskan(&pesan);
}

static void server_tangani_daftar_alat(const char *id)
{
    penyangga_t pesan;

    server_mulai_respons(&pesan, id);
    penyangga_tambah_teks(&pesan, "{\"tools\":");
    alat_tulis_daftar(&pesan);
    penyangga_tambah_teks(&pesan, "}}");
    server_kirim(&pesan);
    penyangga_bebaskan(&pesan);
}

static void server_tangani_panggil_alat(const dokumen_json_t *dok, const simpul_json_t *params,
                                        const char *id)
{
    char *nama = json_teks_salin(dok, json_cari(dok, params, "name"));
    const simpul_json_t *argumen = json_cari(dok, params, "arguments");
    const definisi_alat_t *alat;
    penyangga_t hasil;
    penyangga_t pesan;
    char galat[160];
    int berhasil;

    if (nama == NULL) {
        server_kirim_galat(id, KODE_PARAMETER_TIDAK_VALID, "Parameter name wajib berupa teks");
        return;
    }
    alat = alat_cari(nama);
    if (alat == NULL) {
        snprintf(galat, sizeof galat, "Alat tidak dikenal: %.100s", nama);
        free(nama);
        server_kirim_galat(id, KODE_PARAMETER_TIDAK_VALID, galat);
        return;
    }
    free(nama);
    if (argumen != NULL && argumen->jenis == JSON_KOSONG) {
        argumen = NULL;
    }
    if (argumen != NULL && argumen->jenis != JSON_OBJEK) {
        server_kirim_galat(id, KODE_PARAMETER_TIDAK_VALID,
                           "Parameter arguments harus berupa objek");
        return;
    }
    penyangga_siapkan(&hasil);
    berhasil = alat->penangan(dok, argumen, &hasil);
    if (hasil.gagal) {
        penyangga_kosongkan(&hasil);
        penyangga_tambah_teks(&hasil, "Memori tidak cukup");
        berhasil = 0;
    }
    server_mulai_respons(&pesan, id);
    penyangga_tambah_teks(&pesan, "{\"content\":[{\"type\":\"text\",\"text\":");
    penyangga_tambah_teks_json(&pesan, penyangga_isi(&hasil), hasil.panjang);
    penyangga_tambah_format(&pesan, "}],\"isError\":%s}}", berhasil ? "false" : "true");
    server_kirim(&pesan);
    penyangga_bebaskan(&pesan);
    penyangga_bebaskan(&hasil);
}

static void server_tangani_pesan(const char *teks, size_t panjang)
{
    dokumen_json_t dok;
    const simpul_json_t *akar;
    const simpul_json_t *simpul_id;
    const simpul_json_t *params;
    const char *awal;
    size_t n;
    char *versi_jsonrpc;
    char *metode;
    penyangga_t id;

    if (!json_urai(&dok, teks, panjang)) {
        catat_galat("Pesan bukan JSON valid: %s", dok.galat);
        server_kirim_galat("null", KODE_JSON_TIDAK_VALID, "JSON tidak valid");
        return;
    }
    penyangga_siapkan(&id);
    penyangga_tambah_teks(&id, "null");
    versi_jsonrpc = NULL;
    metode = NULL;
    akar = json_akar(&dok);
    if (akar->jenis != JSON_OBJEK) {
        server_kirim_galat("null", KODE_PERMINTAAN_TIDAK_VALID, "Pesan harus berupa objek JSON");
        goto selesai;
    }
    simpul_id = json_cari(&dok, akar, "id");
    if (simpul_id != NULL) {
        if (simpul_id->jenis != JSON_TEKS && simpul_id->jenis != JSON_ANGKA) {
            server_kirim_galat("null", KODE_PERMINTAAN_TIDAK_VALID,
                               "id harus berupa teks atau angka");
            goto selesai;
        }
        json_mentah(&dok, simpul_id, &awal, &n);
        penyangga_kosongkan(&id);
        penyangga_tambah(&id, awal, n);
    }
    versi_jsonrpc = json_teks_salin(&dok, json_cari(&dok, akar, "jsonrpc"));
    if (versi_jsonrpc == NULL || strcmp(versi_jsonrpc, "2.0") != 0) {
        if (simpul_id != NULL) {
            server_kirim_galat(penyangga_isi(&id), KODE_PERMINTAAN_TIDAK_VALID,
                               "jsonrpc harus \"2.0\"");
        }
        goto selesai;
    }
    metode = json_teks_salin(&dok, json_cari(&dok, akar, "method"));
    if (metode == NULL) {
        /* Tanpa method: respons dari klien (diabaikan) atau permintaan rusak. */
        if (simpul_id != NULL && json_cari(&dok, akar, "result") == NULL &&
            json_cari(&dok, akar, "error") == NULL) {
            server_kirim_galat(penyangga_isi(&id), KODE_PERMINTAAN_TIDAK_VALID,
                               "method wajib berupa teks");
        }
        goto selesai;
    }
    if (simpul_id == NULL) {
        goto selesai; /* notifikasi (mis. notifications/initialized): tanpa respons */
    }
    params = json_cari(&dok, akar, "params");
    if (params != NULL && params->jenis == JSON_KOSONG) {
        params = NULL;
    }
    if (params != NULL && params->jenis != JSON_OBJEK) {
        server_kirim_galat(penyangga_isi(&id), KODE_PARAMETER_TIDAK_VALID,
                           "params harus berupa objek");
        goto selesai;
    }
    if (strcmp(metode, "initialize") == 0) {
        server_tangani_inisialisasi(&dok, params, penyangga_isi(&id));
    } else if (strcmp(metode, "ping") == 0) {
        server_kirim_hasil_tetap(penyangga_isi(&id), "{}");
    } else if (strcmp(metode, "tools/list") == 0) {
        server_tangani_daftar_alat(penyangga_isi(&id));
    } else if (strcmp(metode, "tools/call") == 0) {
        server_tangani_panggil_alat(&dok, params, penyangga_isi(&id));
    } else {
        server_kirim_galat(penyangga_isi(&id), KODE_METODE_TIDAK_ADA, "Metode tidak dikenal");
    }

selesai:
    free(versi_jsonrpc);
    free(metode);
    penyangga_bebaskan(&id);
    json_bebaskan(&dok);
}

/* Membaca satu pesan (satu baris) dari stdin ke pesan. Mengembalikan 0 bila masukan habis,
   1 bila berhasil, 2 bila pesan melewati BATAS_PESAN_BYTE (sisa baris dibuang). */
static int server_baca_pesan(penyangga_t *pesan)
{
    char potongan[4096];
    int ada = 0;
    int terlalu_besar = 0;

    penyangga_kosongkan(pesan);
    while (fgets(potongan, sizeof potongan, stdin) != NULL) {
        size_t n = strlen(potongan);
        int selesai = 0;

        ada = 1;
        if (n > 0 && potongan[n - 1] == '\n') {
            n--;
            selesai = 1;
        }
        if (!terlalu_besar &&
            (pesan->panjang + n > BATAS_PESAN_BYTE || !penyangga_tambah(pesan, potongan, n))) {
            terlalu_besar = 1;
        }
        if (selesai) {
            break;
        }
    }
    if (!ada) {
        return 0;
    }
    if (terlalu_besar) {
        penyangga_kosongkan(pesan);
        return 2;
    }
    if (pesan->panjang > 0 && pesan->data[pesan->panjang - 1] == '\r') {
        pesan->panjang--;
        pesan->data[pesan->panjang] = '\0';
    }
    return 1;
}

int main(void)
{
    penyangga_t pesan;
    int status;

#ifdef _WIN32
    /* Mode biner: tanpa konversi CRLF dan tanpa Ctrl-Z sebagai akhir masukan. */
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    penyangga_siapkan(&pesan);
    catat_info("%s %s dimulai (stdio)", NAMA_SERVER, VERSI_SERVER);
    while ((status = server_baca_pesan(&pesan)) != 0) {
        if (status == 2) {
            catat_galat("Pesan melewati batas %d byte", (int)BATAS_PESAN_BYTE);
            server_kirim_galat("null", KODE_PERMINTAAN_TIDAK_VALID, "Pesan terlalu besar");
            continue;
        }
        if (pesan.panjang == 0) {
            continue;
        }
        server_tangani_pesan(pesan.data, pesan.panjang);
    }
    api_cabut();
    penyangga_bebaskan(&pesan);
    catat_info("Masukan habis, server berhenti");
    return 0;
}
