# Berkontribusi

## Alur

1. Satu pull request = satu pekerjaan; ubah hanya berkas dan baris yang diperlukan. Perubahan di luar lingkup (refaktor, format ulang, ganti nama) dicatat sebagai isu terpisah.
2. Pekerjaan besar dipecah menjadi PR kecil berurutan; tiap PR harus lulus build dan tes sendiri.
3. Sebelum membuka PR, periksa `git diff --stat`: buang berkas tak terkait, hasil build (`bangun/`), dan data sensitif.
4. Pesan commit memakai [Conventional Commits](https://www.conventionalcommits.org/id/v1.0.0/), mis. `fix: tolak host terlalu panjang`.

## Bahasa dan istilah

Kode, komentar, pesan, dan dokumen berbahasa Indonesia. Istilah baru ditambahkan ke [GLOSARIUM.md](GLOSARIUM.md) **sebelum** dipakai di kode. Satu konsep = satu istilah.

## Kode

- C11, kompilasi tanpa peringatan (`-Wall -Wextra -Wpedantic`; CI memakai `PERINGATAN_SEBAGAI_GALAT=ON`).
- Ringan dan tangguh; utamakan optimasi daripada fitur baru; offline first.
- Setiap perubahan perilaku disertai tes di `tes/`; jalankan `ctest` sebelum PR.

## Hemat token (syarat utama)

Model membayar setiap karakter yang dikirim server ini, jadi keluaran harus sekecil mungkin tanpa mengurangi kebenaran.

- Deskripsi alat dan skema masukan singkat; satu alat = satu tugas jelas. Daftar `tools/list` tidak boleh membengkak.
- Keluaran alat hanya memuat bidang yang berguna bagi model; buang bidang kosong, bawaan, atau metadata API yang tak dipakai.
- Hasil besar dibatasi (rentang atau jumlah baris) dan menyebut cara melanjutkan, bukan dikirim utuh.
- Pesan galat satu kalimat, tanpa jejak panjang dan tanpa kredensial.
- JSON ringkas tanpa spasi atau indentasi. Perubahan keluaran disertai tes yang memeriksa ukurannya.
- Keselamatan tidak dikorbankan: peringatan konfirmasi untuk alat merusak tetap ada.

## Keamanan

- Jangan memasukkan kredensial (akun, kata sandi, token) ke repo, log, atau pesan galat.
- Operasi yang mengubah atau menghapus data spreadsheet harus meminta konfirmasi.

## Versi dan rilis

- Patch: perbaikan, keamanan, atau perubahan pemakaian bagi pengguna akhir. Minor: sesuai roadmap di [TODO.md](TODO.md).
- Catat perubahan di [CHANGELOG.md](CHANGELOG.md).
- Tag `vX.Y.Z` = build dan Release otomatis; `vX.Y.Z-rc.N` = pre-release uji. PR dan build manual hanya menghasilkan artifact.
