# TODO

Rencana kerja. Perubahan yang dirilis dicatat di `CHANGELOG.md`.

## Fase 1 — server MCP v0.1.0

- [x] Riset API Synology Spreadsheet, MCP, dan risiko keamanan
- [x] `GLOSARIUM.md`
- [x] Modul inti di `src/`: `penyangga`, `json`, `pengodean`, `konfigurasi`, `catat`, `http`, `klien_api`, `alat`
- [x] Perbaiki peringatan kompilasi di `konfigurasi.c`: alamat terlalu panjang ditolak (tidak dipotong), `salin_teks` hanya dikompilasi di non-Windows
- [x] `main.c`: loop JSON-RPC stdio (`initialize`, `ping`, `tools/list`, `tools/call`)
- [ ] `CMakeLists.txt`, kompilasi tanpa peringatan (`-Wall -Wextra -Wpedantic`)
- [ ] Commit modul `src/` yang belum tercatat (`alat`, `catat`, `http_*`, `json`, `klien_api`, `pengodean`, `penyangga`) bersama `CMakeLists.txt`
- [ ] Tes dasar di `tes/` (json, pengodean, penyangga, konfigurasi, alat)
- [ ] `README.md` (tanpa istilah teknis), `CHANGELOG.md`, `CONTRIBUTING.md`; keterangan lengkap di `docs/`
- [ ] Uji masuk dan cabut token ke NAS (hanya masuk dan cabut; kredensial dari pemilik, tidak disimpan)
- [ ] Uji dengan klien MCP nyata (mis. Claude Desktop) dan satu panggilan alat baca ke NAS
- [ ] Verifikasi versi protokol MCP 2025-11-25 di sumber resmi; tambahkan di `main.c` bila sesuai
- [ ] CI hemat menit: tag `vX.Y.Z` = Release, `vX.Y.Z-rc.N` = pre-release, PR dan manual = artifact
- [ ] Rilis v0.1.0

## Berikutnya

- [ ] Dukungan transport selain stdio bila dibutuhkan
- [ ] Uji akun dengan 2FA, masa berlaku token, batas laju (belum terkonfirmasi, lihat vault `Riset Fase 1`)
