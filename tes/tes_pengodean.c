#include "pengodean.h"
#include "penyangga.h"
#include "tes.h"

static void periksa_kode(const char *masukan, const char *harapan)
{
    penyangga_t p;

    penyangga_siapkan(&p);
    PERIKSA(pengodean_persen(&p, masukan));
    PERIKSA_TEKS(penyangga_isi(&p), harapan);
    penyangga_bebaskan(&p);
}

void tes_periksa_pengodean(void)
{
    periksa_kode("", "");
    periksa_kode("abcXYZ019-._~", "abcXYZ019-._~");
    periksa_kode("a b", "a%20b");
    periksa_kode("Lembar1!A1:C2", "Lembar1%21A1%3AC2");
    periksa_kode("../etc", "..%2Fetc");
    periksa_kode("a?b=c&d#e%f", "a%3Fb%3Dc%26d%23e%25f");
    periksa_kode("caf\xC3\xA9", "caf%C3%A9");
    periksa_kode("\x01\x7F", "%01%7F");
}
