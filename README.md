# mcp-synology-spreadsheet

Penghubung agar asisten AI (mis. Claude Desktop) dapat membaca dan mengisi Synology Spreadsheet di NAS Anda.

## Apa yang bisa dilakukan

- Melihat isi spreadsheet, lembar, dan sel.
- Menulis nilai, menambah baris, dan mengatur tampilan sel.
- Membuat spreadsheet atau lembar baru, dan mengekspor lembar sebagai CSV.
- Menghapus hanya bila Anda memintanya dan menyetujuinya.

## Yang Anda butuhkan

- NAS Synology dengan Spreadsheet API aktif.
- Akun NAS: alamat layanan, alamat NAS, nama pengguna, dan kata sandi.
- Aplikasi AI yang mendukung MCP.

Nomor identitas spreadsheet tidak disimpan; sebutkan saja saat meminta tugas.

## Cara memakai

1. Unduh programnya dari halaman Release dan simpan di `%LOCALAPPDATA%\Programs\mcp-synology-spreadsheet\` (tidak perlu hak administrator).
2. Isi berkas `config.json` di folder `.config\mcp-synology-spreadsheet` pada folder pengguna Anda.
3. Daftarkan program ke aplikasi AI Anda.
4. Minta asisten bekerja pada spreadsheet Anda.

Perubahan pada `config.json` langsung berlaku tanpa memulai ulang aplikasi.

## Keamanan

Kredensial hanya disimpan di komputer Anda, di luar proyek ini, dan tidak pernah ditampilkan di pesan galat.

## Lebih lanjut

Panduan lengkap: [docs/PANDUAN.md](docs/PANDUAN.md). Rencana: [TODO.md](TODO.md). Riwayat: [CHANGELOG.md](CHANGELOG.md). Kontribusi: [CONTRIBUTING.md](CONTRIBUTING.md). Istilah baku: [GLOSARIUM.md](GLOSARIUM.md).

Lisensi: lihat [LICENSE](LICENSE).
