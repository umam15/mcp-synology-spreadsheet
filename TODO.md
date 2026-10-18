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
- [x] Uji pencabutan token ke NAS: saat server berhenti, token dicabut (HTTP 200) setelah perbaikan; uji pencabutan saat `config.json` berubah belum
- [x] Uji dengan klien MCP nyata (Claude) dan panggilan alat baca ke NAS (`ambil_spreadsheet`, `baca_nilai`)
- [x] Verifikasi versi protokol MCP 2025-11-25 di sumber resmi; ditambahkan di `main.c`
- [x] CI hemat menit ditulis (`.github/workflows/ci.yml`): tag `vX.Y.Z` = Release, `vX.Y.Z-rc.N` = pre-release, PR dan manual = artifact
- [x] CI dijalankan lewat `workflow_dispatch` di GitHub: lulus (±41 detik, artifact 112 KB)
- [x] Audit hemat token: ukuran `tools/list` 7025 → 6372 byte (batas tes 6400); keluaran `ambil_spreadsheet` dan `baca_nilai` diringkas; hasil dibatasi 24000 byte; tes ukuran ditambah
- [x] Pre-release `v0.1.0-rc.1` lewat tag: CI dan job `rilis` lulus; aset `.exe` + `SHA256SUMS.txt` terpasang, checksum cocok, exe berjalan (masuk, baca, cabut token)
- [x] Uji alat tulis, struktur, dan hapus pada spreadsheet uji baru (lewat Claude Desktop, exe rc.1): semua berhasil; alat merusak menolak tanpa `konfirmasi`
- [x] Audit hemat token tahap 2: keluaran `tulis_nilai`, `tambah_baris`, `tambah_lembar`, `ganti_nama_lembar`, `baca_gaya` diringkas (`baca_gaya` ±3200 → ±560 karakter), alat gaya/hapus/ubah massal membalas `Berhasil`; tes isi dan penolakan ditambah
- [x] Pesan galat jelas untuk spreadsheet terhapus: "Galat API (HTTP 500): Unexpected error - kemungkinan ID spreadsheet tidak ada atau sudah dihapus"
- [x] Pesan galat HTTP diringkas menjadi satu baris tanpa HTML (JSON `message`/`error`, judul HTML, teks polos; maks 320 karakter); petunjuk untuk 502/503/504
- [ ] Uji ulang exe v0.1.0 ke NAS setelah layanan Spreadsheet API pulih dari 502 (v0.1.0 memakai kode yang sama dengan rc.2, yang lulus uji NAS)
- [ ] Uji masuk ulang otomatis saat token ditolak di tengah sesi
- [x] Rilis v0.1.0

## Berikutnya

- [ ] Klien HTTP untuk platform selain Windows (libcurl), rencana v0.3.0; sementara `http_tanpa_dukungan.c` melaporkan galat jelas
- [ ] Dukungan transport selain stdio bila dibutuhkan
- [ ] Uji akun dengan 2FA, masa berlaku token, batas laju (belum terkonfirmasi, lihat vault `Riset Fase 1`)
