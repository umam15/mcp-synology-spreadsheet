/* klien_api: sesi masuk dan pemanggilan Synology Spreadsheet API. */
#ifndef KLIEN_API_H
#define KLIEN_API_H

#include <stddef.h>

#include "http.h"
#include "konfigurasi.h"

/* Masuk (POST /spreadsheets/authorize) dan menyimpan token di memori. */
int api_masuk(char *galat, size_t ukuran_galat);

/* Memanggil API dengan token sesi; masuk otomatis bila belum, dan masuk ulang sekali
   bila menerima HTTP 401. Mengembalikan 1 bila respons diterima (apa pun kode statusnya).
   respons->badan harus disiapkan dan dibebaskan pemanggil. */
int api_panggil(const char *metode, const char *jalur, const char *badan, size_t panjang_badan,
                respons_http_t *respons, char *galat, size_t ukuran_galat);

/* Konfigurasi yang sudah dimuat, atau NULL bila belum/tidak valid. */
const konfigurasi_t *api_konfigurasi(void);

/* Mencabut token (upaya terbaik) dan membersihkan kredensial dari memori. */
void api_cabut(void);

#endif
