# Changelog

Mengikuti [Keep a Changelog](https://keepachangelog.com/id-ID/1.1.0/) dan [Semantic Versioning](https://semver.org/lang/id/).

## [Belum dirilis]

## [0.1.1] - 2026-10-19

### Ditambahkan

- `tes/uji_sesi.py`: uji sesi dengan server API tiruan lokal (masuk ulang otomatis saat HTTP 401, pencabutan token saat `config.json` berubah dan saat server berhenti, tanpa rahasia di keluaran); dijalankan di CI.
- Build gcc memakai `-Wshadow` (menangkap variabel yang menutupi parameter, yang dilaporkan MSVC sebagai C4457).

### Diubah

- Pesan galat HTTP kini satu baris tanpa HTML (mis. `502 Bad Gateway` alih-alih halaman penuh), dengan petunjuk untuk 502/503/504 (layanan di balik proxy) dan 500 "Unexpected error" (ID spreadsheet tidak ada atau terhapus); hemat token.
- Dokumentasi: lokasi bawaan exe `%LOCALAPPDATA%\Programs\mcp-synology-spreadsheet\`; konfigurasi tetap di `.config`.

## [0.1.0] - 2026-10-18

### Ditambahkan

- Server MCP stdio berbahasa C dengan 14 alat untuk Synology Spreadsheet.
- Konfigurasi dari `config.json` di `.config` dengan muat ulang otomatis; variabel lingkungan sebagai cadangan.
- Klien HTTP Windows (WinHTTP); platform lain melaporkan galat jelas.
- Build CMake dan tes otomatis (CTest).
- Dukungan versi protokol MCP 2025-11-25.
- Hemat token: deskripsi alat dipersingkat; `ambil_spreadsheet` dan `baca_nilai` mengirim keluaran ringkas; hasil alat dibatasi 24000 byte; tes ukuran daftar alat dan keluaran.
- CI GitHub Actions hemat menit: build dan tes Windows pada PR dan manual (artifact); Release otomatis pada tag `vX.Y.Z`, pre-release pada `vX.Y.Z-rc.N`.
- Hasil pencabutan token dicatat di log (tanpa token).
- Hemat token tahap 2: keluaran ringkas untuk `tulis_nilai`, `tambah_baris`, `tambah_lembar`, `ganti_nama_lembar`, `baca_gaya` (±80% lebih kecil), dan `Berhasil` untuk `tulis_gaya`, `ubah_massal`, `hapus_lembar`, `hapus_spreadsheet`.
- Dokumen: README, GLOSARIUM, TODO, CONTRIBUTING, panduan teknis.

### Diperbaiki

- Pencabutan token saat server berhenti gagal (HTTP 500) karena `Content-Type: application/json` dikirim tanpa badan; kini hanya dikirim bila ada badan.
