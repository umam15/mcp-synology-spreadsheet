# Panduan teknis

## Ringkasan

Server MCP berbahasa C11 (transport stdio, JSON-RPC satu pesan per baris) yang memanggil Synology Spreadsheet API lewat DSM. Hanya bergantung pada pustaka sistem (Windows: WinHTTP). Istilah baku: [GLOSARIUM.md](../GLOSARIUM.md).

## Membangun

```text
cmake -G Ninja -S . -B bangun -DPERINGATAN_SEBAGAI_GALAT=ON
cmake --build bangun
ctest --test-dir bangun
```

Opsi: `PERINGATAN_SEBAGAI_GALAT` (CI), `BANGUN_TES` (bawaan ON). Klien HTTP baru tersedia di Windows; platform lain melaporkan galat jelas (libcurl direncanakan v0.3.0, lihat [TODO.md](../TODO.md)).

## Konfigurasi

Berkas `%USERPROFILE%\.config\mcp-synology-spreadsheet\config.json` (non-Windows: `$HOME/.config/mcp-synology-spreadsheet/config.json`), maks 16 KiB:

```json
{
  "server_api": "",
  "server_nas": "",
  "nama_pengguna": "",
  "kata_sandi": ""
}
```

- Berkas dibaca pada setiap panggilan alat; bila isinya berubah, token lama dicabut dan konfigurasi dimuat ulang.
- Bila berkas tidak ada, dipakai variabel lingkungan `SYNO_SHEET_API_SERVER`, `SYNO_SERVER`, `SYNO_USER`, `SYNO_PASS`. Bila berkas ada, hanya berkas yang dipakai.
- Jangan simpan berkas ini di repo.

## Pendaftaran di klien MCP

Lokasi bawaan exe: `%LOCALAPPDATA%\Programs\mcp-synology-spreadsheet\mcp-synology-spreadsheet.exe` (khusus pengguna, tanpa hak administrator, terpisah dari konfigurasi; jangan di folder yang disinkronkan karena exe terkunci saat server berjalan). Konfigurasi tetap di `.config` (bagian di atas), sehingga exe bisa diganti tanpa menyentuh kredensial. Memperbarui: tutup klien, timpa exe, buka klien.

Daftarkan exe langsung tanpa argumen, mis. pada `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "synology-spreadsheet": {
      "command": "C:\\Users\\<pengguna>\\AppData\\Local\\Programs\\mcp-synology-spreadsheet\\mcp-synology-spreadsheet.exe"
    }
  }
}
```

## Alat (14)

| Kelompok | Alat |
| --- | --- |
| Sambungan | `periksa_sambungan` |
| Baca | `ambil_spreadsheet`, `baca_nilai`, `baca_gaya`, `ekspor_csv` |
| Tulis | `tulis_nilai`, `tulis_gaya`, `tambah_baris`, `ubah_massal` |
| Struktur | `buat_spreadsheet`, `tambah_lembar`, `ganti_nama_lembar` |
| Merusak (wajib konfirmasi) | `hapus_lembar`, `hapus_spreadsheet` |

Alat diberi anotasi baca/merusak. `id_spreadsheet` selalu diteruskan sebagai argumen.

Keluaran ringkas (hemat token):

| Alat | Keluaran |
| --- | --- |
| `ambil_spreadsheet` | `{"judul","lembar":[{"id_lembar","nama","baris","kolom"}]}` |
| `baca_nilai` | larik nilai saja |
| `baca_gaya` | `{"rentang","baris":[[sel]]}`; sel `{"v":nilai,"h":hasil rumus,"f":gaya dimasukkan}` atau `null` bila kosong (gaya efektif bawaan dibuang) |
| `tulis_nilai`, `tambah_baris` | `{"rentang","baris","kolom"}` (rentang yang ditulis; nilai tidak dipantulkan) |
| `tambah_lembar`, `ganti_nama_lembar` | `{"id_lembar","nama"}` |
| `tulis_gaya`, `ubah_massal`, `hapus_lembar`, `hapus_spreadsheet` | `Berhasil` |

Hasil alat dipotong pada 24000 byte dengan catatan "terpotong"; baca rentang atau lembar yang lebih kecil.

Galat HTTP dilaporkan satu baris (maks 320 karakter), mis. `Galat API (HTTP 502): 502 Bad Gateway - layanan Spreadsheet API di balik proxy tidak menjawab; periksa layanan di NAS`. Isi diambil dari `message` atau `error` JSON, atau judul atau teks HTML tanpa tag.

## Protokol

Versi MCP yang didukung: 2025-11-25, 2025-06-18, 2025-03-26, 2024-11-05. Notifikasi tidak dibalas; log hanya ke stderr tanpa kredensial; batas pesan 16 MiB.

## Struktur kode

| Modul | Fungsi |
| --- | --- |
| `main.c` | loop JSON-RPC |
| `alat.c` | daftar dan penangan alat |
| `klien_api.c` | masuk, token, panggilan API |
| `konfigurasi.c` | muat dan muat ulang `config.json` |
| `http_windows.c` / `http_tanpa_dukungan.c` | klien HTTP |
| `json.c`, `pengodean.c`, `penyangga.c`, `catat.c` | utilitas |
