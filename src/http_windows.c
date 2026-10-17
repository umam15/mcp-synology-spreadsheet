/* Klien HTTP untuk Windows memakai WinHTTP (bawaan sistem; tanpa pustaka tambahan).
   Validasi sertifikat TLS tidak pernah dimatikan. */
#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winhttp.h>

#define BATAS_WAKTU_RESOLUSI_MS 5000
#define BATAS_WAKTU_SAMBUNG_MS 5000
#define BATAS_WAKTU_KIRIM_MS 30000
#define BATAS_WAKTU_TERIMA_MS 120000

static HINTERNET sesi_global = NULL;

static void isi_galat(char *galat, size_t ukuran, const char *pesan, unsigned long kode)
{
    const char *uraian;

    if (galat == NULL || ukuran == 0) {
        return;
    }
    switch (kode) {
    case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        uraian = "nama host tidak ditemukan";
        break;
    case ERROR_WINHTTP_CANNOT_CONNECT:
        uraian = "tidak dapat terhubung";
        break;
    case ERROR_WINHTTP_TIMEOUT:
        uraian = "waktu habis";
        break;
    case ERROR_WINHTTP_SECURE_FAILURE:
    case ERROR_WINHTTP_SECURE_CERT_CN_INVALID:
    case ERROR_WINHTTP_SECURE_CERT_DATE_INVALID:
    case ERROR_WINHTTP_SECURE_INVALID_CA:
    case ERROR_WINHTTP_SECURE_INVALID_CERT:
        uraian = "kegagalan TLS (sertifikat atau protokol)";
        break;
    case 0:
        uraian = "";
        break;
    default:
        uraian = "galat jaringan";
        break;
    }
    if (kode == 0) {
        snprintf(galat, ukuran, "%s", pesan);
    } else {
        snprintf(galat, ukuran, "%s: %s (kode %lu)", pesan, uraian, kode);
    }
}

static wchar_t *ke_lebar(const char *teks, size_t panjang)
{
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, teks, (int)panjang, NULL, 0);
    wchar_t *lebar;

    if (n < 0 || (n == 0 && panjang != 0)) {
        return NULL;
    }
    lebar = malloc(((size_t)n + 1) * sizeof *lebar);
    if (lebar == NULL) {
        return NULL;
    }
    if (n > 0) {
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, teks, (int)panjang, lebar, n);
    }
    lebar[n] = 0;
    return lebar;
}

static int token_valid(const char *token)
{
    size_t i;

    if (token[0] == '\0' || strlen(token) > 8192) {
        return 0;
    }
    for (i = 0; token[i] != '\0'; i++) {
        unsigned char c = (unsigned char)token[i];
        if (c < 0x21 || c > 0x7E) {
            return 0;
        }
    }
    return 1;
}

int http_kirim(const alamat_t *alamat, const char *metode, const char *jalur, const char *token,
               const char *badan, size_t panjang_badan, respons_http_t *respons, char *galat,
               size_t ukuran_galat)
{
    HINTERNET koneksi = NULL;
    HINTERNET permintaan = NULL;
    wchar_t *host_lebar = NULL;
    wchar_t *metode_lebar = NULL;
    wchar_t *jalur_lebar = NULL;
    wchar_t *tajuk_lebar = NULL;
    penyangga_t tajuk;
    DWORD kode_status = 0;
    DWORD ukuran_kode = sizeof kode_status;
    int berhasil = 0;
    char potongan[16384];

    penyangga_siapkan(&tajuk);
    respons->kode_status = 0;

    if (sesi_global == NULL) {
        sesi_global = WinHttpOpen(L"mcp-synology-spreadsheet", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (sesi_global == NULL) {
            isi_galat(galat, ukuran_galat, "Gagal menyiapkan WinHTTP", GetLastError());
            return 0;
        }
        WinHttpSetTimeouts(sesi_global, BATAS_WAKTU_RESOLUSI_MS, BATAS_WAKTU_SAMBUNG_MS,
                           BATAS_WAKTU_KIRIM_MS, BATAS_WAKTU_TERIMA_MS);
    }

    /* Content-Type hanya bila ada badan: server menolak (HTTP 500) POST cabut token tanpa badan
       yang berjenis JSON. */
    if (panjang_badan > 0) {
        penyangga_tambah_teks(&tajuk, "Content-Type: application/json\r\n");
    }
    penyangga_tambah_teks(&tajuk, "Accept: */*\r\n");
    if (token != NULL) {
        if (!token_valid(token)) {
            isi_galat(galat, ukuran_galat, "Token sesi tidak valid", 0);
            goto selesai;
        }
        penyangga_tambah_format(&tajuk, "Authorization: Bearer %s\r\n", token);
    }
    host_lebar = ke_lebar(alamat->nama_host, strlen(alamat->nama_host));
    metode_lebar = ke_lebar(metode, strlen(metode));
    jalur_lebar = ke_lebar(jalur, strlen(jalur));
    tajuk_lebar = ke_lebar(penyangga_isi(&tajuk), tajuk.panjang);
    if (host_lebar == NULL || metode_lebar == NULL || jalur_lebar == NULL ||
        tajuk_lebar == NULL) {
        isi_galat(galat, ukuran_galat, "Gagal menyiapkan permintaan (karakter tidak valid)", 0);
        goto selesai;
    }

    koneksi = WinHttpConnect(sesi_global, host_lebar, (INTERNET_PORT)alamat->port, 0);
    if (koneksi == NULL) {
        isi_galat(galat, ukuran_galat, "Gagal menghubungi server", GetLastError());
        goto selesai;
    }
    permintaan = WinHttpOpenRequest(koneksi, metode_lebar, jalur_lebar, NULL, WINHTTP_NO_REFERER,
                                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                                    alamat->https ? WINHTTP_FLAG_SECURE : 0);
    if (permintaan == NULL) {
        isi_galat(galat, ukuran_galat, "Gagal membuat permintaan", GetLastError());
        goto selesai;
    }
    if (!WinHttpSendRequest(permintaan, tajuk_lebar, (DWORD)-1L,
                            (badan != NULL && panjang_badan > 0) ? (LPVOID)badan
                                                                 : WINHTTP_NO_REQUEST_DATA,
                            (DWORD)(badan != NULL ? panjang_badan : 0),
                            (DWORD)(badan != NULL ? panjang_badan : 0), 0)) {
        isi_galat(galat, ukuran_galat, "Gagal mengirim permintaan", GetLastError());
        goto selesai;
    }
    if (!WinHttpReceiveResponse(permintaan, NULL)) {
        isi_galat(galat, ukuran_galat, "Gagal menerima respons", GetLastError());
        goto selesai;
    }
    if (!WinHttpQueryHeaders(permintaan, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &kode_status, &ukuran_kode,
                             WINHTTP_NO_HEADER_INDEX)) {
        isi_galat(galat, ukuran_galat, "Gagal membaca kode status", GetLastError());
        goto selesai;
    }
    respons->kode_status = (int)kode_status;

    for (;;) {
        DWORD tersedia = 0;
        DWORD terbaca = 0;

        if (!WinHttpQueryDataAvailable(permintaan, &tersedia)) {
            isi_galat(galat, ukuran_galat, "Gagal membaca respons", GetLastError());
            goto selesai;
        }
        if (tersedia == 0) {
            break;
        }
        if (tersedia > sizeof potongan) {
            tersedia = sizeof potongan;
        }
        if (!WinHttpReadData(permintaan, potongan, tersedia, &terbaca)) {
            isi_galat(galat, ukuran_galat, "Gagal membaca respons", GetLastError());
            goto selesai;
        }
        if (terbaca == 0) {
            break;
        }
        if (respons->badan.panjang + terbaca > BATAS_RESPONS_BYTE) {
            isi_galat(galat, ukuran_galat, "Respons melebihi batas 8 MiB", 0);
            goto selesai;
        }
        if (!penyangga_tambah(&respons->badan, potongan, terbaca)) {
            isi_galat(galat, ukuran_galat, "Memori tidak cukup", 0);
            goto selesai;
        }
    }
    berhasil = 1;

selesai:
    if (permintaan != NULL) {
        WinHttpCloseHandle(permintaan);
    }
    if (koneksi != NULL) {
        WinHttpCloseHandle(koneksi);
    }
    free(host_lebar);
    free(metode_lebar);
    free(jalur_lebar);
    free(tajuk_lebar);
    penyangga_bebaskan(&tajuk);
    return berhasil;
}

void http_tutup(void)
{
    if (sesi_global != NULL) {
        WinHttpCloseHandle(sesi_global);
        sesi_global = NULL;
    }
}
