# Changelog

Mengikuti [Keep a Changelog](https://keepachangelog.com/id-ID/1.1.0/) dan [Semantic Versioning](https://semver.org/lang/id/).

## [Belum dirilis]

### Ditambahkan

- Server MCP stdio berbahasa C dengan 14 alat untuk Synology Spreadsheet.
- Konfigurasi dari `config.json` di `.config` dengan muat ulang otomatis; variabel lingkungan sebagai cadangan.
- Klien HTTP Windows (WinHTTP); platform lain melaporkan galat jelas.
- Build CMake dan tes otomatis (CTest).
- Dukungan versi protokol MCP 2025-11-25.
- Hemat token: deskripsi alat dipersingkat; `ambil_spreadsheet` dan `baca_nilai` mengirim keluaran ringkas; hasil alat dibatasi 24000 byte; tes ukuran daftar alat dan keluaran.
- Dokumen: README, GLOSARIUM, TODO, CONTRIBUTING, panduan teknis.
