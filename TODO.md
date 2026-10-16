# TODO

Rencana kerja. Perubahan yang dirilis dicatat di `CHANGELOG.md`.

## Fase 1 — server MCP v0.1.0

- [x] Riset API Synology Spreadsheet, MCP, dan risiko keamanan
- [x] `GLOSARIUM.md`
- [x] Modul inti di `src/`: `penyangga`, `json`, `pengodean`, `konfigurasi`, `catat`, `http`, `klien_api`, `alat`
- [x] Perbaiki peringatan kompilasi di `konfigurasi.c`: alamat terlalu panjang ditolak (tidak dipotong), `salin_teks` hanya dikompilasi di non-Windows
- [x] `main.c`: loop JSON-RPC stdio (`initialize`, `ping`, `tools/list`, `tools/call`)
- [x] `CMakeLists.txt`, kompilasi tanpa peringatan (`-Wall -Wextra -Wpedantic`)
- [x] Commit modul `src/` yang belum tercatat bersama `CMakeLists.txt`
- [x] Tes dasar di `tes/` (json, pengodean, penyangga, konfigurasi, alat)
- [x] Kredensial dari `config.json` di `.config` dengan muat ulang otomatis saat berkas berubah (tanpa memulai ulang klien MCP); variabel lingkungan sebagai cadangan
- [x] `README.md` (tanpa istilah teknis), `CHANGELOG.md`, `CONTRIBUTING.md`; keterangan lengkap di `docs/`
- [x] Uji masuk ke NAS (akun uji, lewat `periksa_sambungan`)
- [ ] Uji pencabutan token ke NAS (`api_cabut` saat konfigurasi berubah atau server berhenti; kredensial dari pemilik, tidak disimpan)
- [x] Uji dengan klien MCP nyata (Claude) dan panggilan alat baca ke NAS (`ambil_spreadsheet`, `baca_nilai`)
- [ ] Verifikasi versi protokol MCP 2025-11-25 di sumber resmi; tambahkan di `main.c` bila sesuai
- [ ] CI hemat menit: tag `vX.Y.Z` = Release, `vX.Y.Z-rc.N` = pre-release, PR dan manual = artifact
- [ ] Rilis v0.1.0

## Berikutnya

- [ ] Klien HTTP untuk platform selain Windows (libcurl), rencana v0.3.0; sementara `http_tanpa_dukungan.c` melaporkan galat jelas
- [ ] Dukungan transport selain stdio bila dibutuhkan
- [ ] Uji akun dengan 2FA, masa berlaku token, batas laju (belum terkonfirmasi, lihat vault `Riset Fase 1`)
