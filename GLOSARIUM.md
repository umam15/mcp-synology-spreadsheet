# Glosarium

Sumber tunggal istilah proyek. Semua nama berkas, fungsi, variabel, tipe, konstanta, alat MCP, argumen, dan pesan memakai istilah di sini. Satu konsep = satu istilah. Bila istilah belum ada, tambahkan di sini **sebelum** dipakai di kode (lihat [CONTRIBUTING.md](CONTRIBUTING.md)).

## Aturan penamaan

| Hal | Aturan | Contoh |
| --- | --- | --- |
| Berkas | `huruf_kecil_garis_bawah`, bahasa Indonesia | `klien_api.c`, `penyangga.h` |
| Fungsi | `<modul>_<kata_kerja>[_<objek>]` | `api_masuk`, `json_cari`, `penyangga_tambah_teks` |
| Variabel dan argumen | `huruf_kecil_garis_bawah` | `kode_status`, `id_spreadsheet` |
| Tipe | `<nama>_t` | `penyangga_t`, `sesi_api_t` |
| Konstanta dan makro | `HURUF_BESAR_GARIS_BAWAH` | `BATAS_RESPONS_BYTE` |
| Alat MCP dan argumennya | `huruf_kecil_garis_bawah` | `baca_nilai`, `rentang` |
| Pesan log, galat, dan alat | Bahasa Indonesia, tanpa kredensial | `Galat API (HTTP 401)` |
| Komentar dan dokumen | Bahasa Indonesia | — |

Kata kerja baku untuk fungsi: `buat`, `bebaskan`, `baca`, `tulis`, `ambil`, `cari`, `urai`, `kirim`, `tambah`, `hapus`, `periksa`, `muat`, `tangani`. Jangan memakai sinonim lain (mis. `get`, `fetch`, `remove`).

## Istilah umum

| Istilah | Padanan Inggris | Dipakai untuk |
| --- | --- | --- |
| alat | tool | alat MCP yang dipanggil model |
| argumen | argument | masukan sebuah alat |
| badan | body | isi permintaan atau respons HTTP |
| batas_waktu | timeout | batas waktu jaringan |
| galat | error | kegagalan; pesan galat |
| hasil | result | keluaran fungsi atau alat |
| jalur | path | bagian alamat setelah host |
| kata_sandi | password | kredensial |
| kode_status | status code | angka status HTTP |
| konfigurasi | configuration | pengaturan dari variabel lingkungan |
| konfirmasi | confirmation | persetujuan sebelum aksi merusak |
| kredensial | credentials | empat nilai akses (lihat bawah) |
| larik | array | larik JSON |
| log / catat | log | catatan ke stderr (`catat_*`) |
| metode | method | metode HTTP atau metode JSON-RPC |
| nama_pengguna | username | akun NAS |
| objek | object | objek JSON |
| id | id | pengenal permintaan JSON-RPC (dikembalikan apa adanya pada respons) |
| kode_galat | error code | angka galat JSON-RPC, mis. -32601 |
| notifikasi | notification | pesan JSON-RPC tanpa `id`; tidak dibalas |
| pesan | message | satu pesan JSON-RPC (satu baris di stdin/stdout) |
| versi | version | versi server, mis. `0.1.0` |
| versi_protokol | protocol version | versi MCP hasil negosiasi |
| penangan | handler | fungsi pelaksana sebuah alat |
| penyangga | buffer | wadah teks yang tumbuh |
| permintaan | request | permintaan HTTP atau JSON-RPC |
| protokol | protocol/scheme | `http` atau `https` |
| respons | response | jawaban HTTP atau JSON-RPC |
| sesi | session | keadaan masuk (token) |
| simpul | node | elemen pohon JSON |
| skema_masukan | input schema | skema JSON argumen alat |
| tajuk | header | tajuk HTTP |
| tes | test | pengujian otomatis |
| teks | string | rangkaian karakter |
| token | token | token JWT sesi |
| urai | parse | mengurai teks menjadi struktur |
| host | host | nama/alamat jaringan beserta port (istilah jaringan baku) |
| server | server | komputer layanan (istilah yang dipakai pemilik) |

## Istilah spreadsheet

| Istilah | Padanan Inggris | Dipakai untuk |
| --- | --- | --- |
| spreadsheet | spreadsheet | dokumen Synology Spreadsheet (nama produk; tidak diterjemahkan) |
| id_spreadsheet | spreadsheetId | ID dokumen, dari URL `.../oo/r/<id>` |
| lembar | sheet | tab di dalam spreadsheet |
| id_lembar | sheetId | ID lembar, mis. `sh_1` |
| nama_lembar | sheet name | judul lembar |
| sel | cell | satu kotak data |
| rentang | range | notasi A1, mis. `Lembar1!A1:C2` |
| baris | row | — |
| kolom | column | — |
| nilai | value | isi sel (angka, teks, logika, teks kaya) |
| rumus | formula | nilai berawalan `=` |
| gaya | style | format sel (huruf, warna, perataan, batas) |
| tambah_baris | append | menambah baris di akhir tabel |
| ubah_massal | batchUpdate | menyisipkan/menghapus baris atau kolom sekaligus |
| dimensi | dimension | `ROWS` (baris) atau `COLUMNS` (kolom) |
| ekspor | export | mengambil sebagai CSV atau XLSX |

## Kredensial dan konfigurasi

Hanya empat nilai, dari variabel lingkungan. Nama variabel sama dengan `.syno-creds` di proyek Synology Skills (nama warisan, tidak diubah agar tak ada istilah ganda):

| Variabel | Istilah kode | Arti |
| --- | --- | --- |
| `SYNO_SHEET_API_SERVER` | `server_api` | alamat layanan Spreadsheet API |
| `SYNO_SERVER` | `server_nas` | alamat NAS (DSM) |
| `SYNO_USER` | `nama_pengguna` | akun NAS |
| `SYNO_PASS` | `kata_sandi` | kata sandi akun |

`id_spreadsheet` **bukan** kredensial: diberikan pada percakapan dan diteruskan sebagai argumen alat.

## Istilah luar yang tidak diterjemahkan

Dikirim atau diterima apa adanya oleh protokol atau API, jadi tetap asli: `jsonrpc`, `initialize`, `tools/list`, `tools/call`, `isError`, `inputSchema`, `Authorization`, `Bearer`, `spreadsheetId`, `sheetId`, `values`, `range`, `token`, `username`, `password`, `host`, `protocol`, `insertDimension`, `deleteDimension`. Istilah ini hanya muncul sebagai literal teks, bukan sebagai nama identifier kode.

## Mengubah glosarium

Satu perubahan istilah = satu commit terpisah yang juga memperbarui semua pemakaiannya. Cari seluruh pemakaian lama sebelum mengganti nama.
