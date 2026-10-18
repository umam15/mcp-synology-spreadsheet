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

/* Memuat konfigurasi bila belum, atau memuat ulang (dan mencabut token lama) bila berkas
   konfigurasi berubah. Dipanggil otomatis oleh api_masuk dan api_panggil; mengembalikan 1
   bila konfigurasi valid. */
int api_pastikan_konfigurasi(char *galat, size_t ukuran_galat);

/* Hemat token: meringkas badan galat HTTP menjadi satu baris pendek di keluar (JSON "message"
   atau "error", judul atau teks HTML tanpa tag, atau teks polos; dipotong pada ukuran-1 byte di
   tepi UTF-8). Badan kosong menghasilkan "(tanpa isi)". */
void api_ringkas_galat(const char *badan, size_t panjang, char *keluar, size_t ukuran);

/* Petunjuk singkat (diawali " - ") untuk kode galat tertentu, atau "" bila tidak ada. */
const char *api_petunjuk_galat(int kode_status, const char *ringkas);

/* Mencabut token (upaya terbaik) dan membersihkan kredensial dari memori. */
void api_cabut(void);

#endif
