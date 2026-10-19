#!/usr/bin/env python3
"""Uji sesi dengan server API tiruan lokal (tanpa NAS dan tanpa kredensial nyata).

Memeriksa: masuk ulang otomatis saat HTTP 401, pencabutan token lama saat config.json
berubah, pencabutan token saat server berhenti, dan tidak adanya rahasia di keluaran.

Pemakaian: python tes/uji_sesi.py <jalur exe>
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

KATA_SANDI = "rahasia-uji-123"
ID_SPREADSHEET = "0123456789abcdef0123456789abcdef"


class Keadaan:
    def __init__(self):
        self.kunci = threading.Lock()
        self.jumlah_token = 0
        self.masuk = []  # (pengguna, token)
        self.dicabut = []  # token
        self.kedaluwarsa = set()
        self.panggilan = []  # (jalur, token, kode)


keadaan = Keadaan()


class Penangan(BaseHTTPRequestHandler):
    def log_message(self, *argumen):
        pass

    def _kirim(self, kode, objek):
        data = json.dumps(objek).encode()
        self.send_response(kode)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _badan(self):
        n = int(self.headers.get("Content-Length") or 0)
        return self.rfile.read(n) if n else b""

    def _token(self):
        a = self.headers.get("Authorization", "")
        return a[7:] if a.startswith("Bearer ") else None

    def do_POST(self):
        badan = self._badan()
        with keadaan.kunci:
            if self.path == "/spreadsheets/authorize":
                d = json.loads(badan)
                keadaan.jumlah_token += 1
                token = "tok-%d" % keadaan.jumlah_token
                keadaan.masuk.append((d.get("username"), token))
                return self._kirim(200, {"token": token})
            if self.path == "/spreadsheets/authorize/token/revoke":
                keadaan.dicabut.append(self._token())
                return self._kirim(200, {"success": True})
        self._kirim(404, {"message": "tidak ada"})

    def do_GET(self):
        token = self._token()
        with keadaan.kunci:
            if token is None or token in keadaan.kedaluwarsa or token in keadaan.dicabut:
                keadaan.panggilan.append((self.path, token, 401))
                return self._kirim(401, {"message": "Unauthorized"})
            keadaan.panggilan.append((self.path, token, 200))
        self._kirim(
            200,
            {
                "id": ID_SPREADSHEET,
                "properties": {"title": "uji", "locale": "en_US"},
                "sheets": [
                    {
                        "properties": {"title": "Sheet1", "sheetId": "sh_1", "index": 0, "hidden": False},
                        "rowCount": 4,
                        "colCount": 29,
                        "mergeCells": [],
                    }
                ],
            },
        )


gagal = []


def periksa(kondisi, pesan):
    print(("lulus  " if kondisi else "GAGAL  ") + pesan)
    if not kondisi:
        gagal.append(pesan)


def main():
    if len(sys.argv) != 2:
        sys.exit("Pemakaian: python tes/uji_sesi.py <jalur exe>")
    exe = os.path.abspath(sys.argv[1])

    server = ThreadingHTTPServer(("127.0.0.1", 0), Penangan)
    port = server.server_address[1]
    threading.Thread(target=server.serve_forever, daemon=True).start()

    rumah = tempfile.mkdtemp(prefix="uji_sesi_")
    folder = os.path.join(rumah, ".config", "mcp-synology-spreadsheet")
    os.makedirs(folder)
    berkas = os.path.join(folder, "config.json")

    def tulis_konfigurasi(pengguna):
        with open(berkas, "w", encoding="utf-8") as f:
            json.dump(
                {
                    "server_api": "http://127.0.0.1:%d" % port,
                    "server_nas": "https://nas.uji:5001",
                    "nama_pengguna": pengguna,
                    "kata_sandi": KATA_SANDI,
                },
                f,
            )

    tulis_konfigurasi("pengguna_a")
    env = {k: v for k, v in os.environ.items() if not k.startswith("SYNO_")}
    env["USERPROFILE"] = rumah
    env["HOME"] = rumah

    proses = subprocess.Popen(
        [exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env
    )
    stderr_baris = []
    threading.Thread(
        target=lambda: [stderr_baris.append(b) for b in proses.stderr], daemon=True
    ).start()
    pengaman = threading.Timer(60, proses.kill)  # pengaman bila macet
    pengaman.daemon = True
    pengaman.start()

    def kirim(objek):
        proses.stdin.write((json.dumps(objek) + "\n").encode())
        proses.stdin.flush()

    keluaran = []

    def terima(id_):
        while True:
            baris = proses.stdout.readline()
            if not baris:
                raise RuntimeError("exe berhenti sebelum membalas id %s" % id_)
            keluaran.append(baris)
            balasan = json.loads(baris)
            if balasan.get("id") == id_:
                return balasan

    def panggil(id_):
        kirim(
            {
                "jsonrpc": "2.0",
                "id": id_,
                "method": "tools/call",
                "params": {"name": "ambil_spreadsheet", "arguments": {"id_spreadsheet": ID_SPREADSHEET}},
            }
        )
        return terima(id_)

    try:
        kirim(
            {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "initialize",
                "params": {
                    "protocolVersion": "2025-11-25",
                    "capabilities": {},
                    "clientInfo": {"name": "uji_sesi", "version": "0"},
                },
            }
        )
        terima(1)
        kirim({"jsonrpc": "2.0", "method": "notifications/initialized"})

        # 1. panggilan pertama: masuk otomatis lalu berhasil
        r = panggil(2)
        periksa(r["result"]["isError"] is False, "panggilan pertama berhasil")
        periksa(keadaan.masuk == [("pengguna_a", "tok-1")], "masuk sekali sebagai pengguna_a (tok-1)")

        # 2. token kedaluwarsa di server: HTTP 401 lalu masuk ulang otomatis sekali
        with keadaan.kunci:
            keadaan.kedaluwarsa.add("tok-1")
        r = panggil(3)
        periksa(r["result"]["isError"] is False, "panggilan berhasil setelah token kedaluwarsa")
        periksa(len(keadaan.masuk) == 2 and keadaan.masuk[1] == ("pengguna_a", "tok-2"), "masuk ulang tepat sekali (tok-2)")
        periksa(
            [p for p in keadaan.panggilan if p[1] == "tok-1" and p[2] == 401] != []
            and [p for p in keadaan.panggilan if p[1] == "tok-2" and p[2] == 200] != [],
            "tok-1 ditolak 401, tok-2 diterima 200",
        )

        # 3. config.json berubah: token lama dicabut, masuk dengan konfigurasi baru
        time.sleep(1.2)
        tulis_konfigurasi("pengguna_b")
        r = panggil(4)
        periksa(r["result"]["isError"] is False, "panggilan berhasil setelah config.json berubah")
        periksa("tok-2" in keadaan.dicabut, "token lama (tok-2) dicabut saat konfigurasi berubah")
        periksa(keadaan.masuk[-1] == ("pengguna_b", "tok-3"), "masuk ulang sebagai pengguna_b (tok-3)")

        # 4. server berhenti: token terakhir dicabut
        proses.stdin.close()
        proses.wait(timeout=15)
        periksa(proses.returncode == 0, "exe berhenti normal (kode 0)")
        periksa("tok-3" in keadaan.dicabut, "token terakhir (tok-3) dicabut saat server berhenti")

        # 5. tidak ada rahasia di keluaran
        semua = b"".join(keluaran) + b"".join(stderr_baris)
        periksa(KATA_SANDI.encode() not in semua, "kata sandi tidak muncul di stdout atau stderr")
        periksa(b"tok-" not in semua, "token tidak muncul di stdout atau stderr")
    except Exception as e:  # noqa: BLE001
        periksa(False, "uji berhenti karena galat: %r" % (e,))
        if stderr_baris:
            print("stderr exe:", b"".join(stderr_baris)[-500:].decode(errors="replace"))
    finally:
        pengaman.cancel()
        proses.kill()
        server.shutdown()
        shutil.rmtree(rumah, ignore_errors=True)

    print("\n%s: %d pemeriksaan gagal" % ("GAGAL" if gagal else "LULUS", len(gagal)))
    sys.exit(1 if gagal else 0)


if __name__ == "__main__":
    main()
