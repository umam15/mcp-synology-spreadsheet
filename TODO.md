# TODO

Rencana kerja. Perubahan yang dirilis dicatat di `CHANGELOG.md`.

## Fase 1 — server MCP v0.1.0

- [x] Riset API Synology Spreadsheet, MCP, dan risiko keamanan
- [x] `GLOSARIUM.md`
- [x] Modul inti di `src/`: `penyangga`, `json`, `pengodean`, `konfigurasi`, `catat`, `http`, `klien_api`, `alat`
- [ ] Perbaiki peringatan kompilasi di `konfigurasi.c`: validasi panjang alamat (host maks. 299 byte), hapus `salin_teks` bila tak terpakai
- [ ] `main.c`: loop JSON-RPC stdio (`initialize`, `tools/list`, `tools/call`)
- [ ] `CMakeLists.txt`, kompilasi tanpa peringatan (`-Wall -Wextra -Wpedantic`)
- [ ] Tes dasar di `tes/` (json, pengodean, penyangga, konfigurasi, alat)
- [ ] `README.md` (tanpa istilah teknis), `CHANGELOG.md`, `CONTRIBUTING.md`; keterangan lengkap di `docs/`
- [ ] Uji masuk dan cabut token ke NAS (hanya masuk dan cabut; kredensial dari pemilik, tidak disimpan)
- [ ] CI hemat menit: tag `vX.Y.Z` = Release, `vX.Y.Z-rc.N` = pre-release, PR dan manual = artifact
- [ ] Rilis v0.1.0

## Berikutnya

- [ ] Dukungan transport selain stdio bila dibutuhkan
- [ ] Uji akun dengan 2FA, masa berlaku token, batas laju (belum terkonfirmasi, lihat vault `Riset Fase 1`)
