#include "pengodean.h"

int pengodean_persen(penyangga_t *penyangga, const char *teks)
{
    static const char heks[] = "0123456789ABCDEF";
    const unsigned char *s = (const unsigned char *)teks;

    for (; *s != '\0'; s++) {
        unsigned char c = *s;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '.' || c == '_' || c == '~') {
            penyangga_tambah(penyangga, (const char *)s, 1);
        } else {
            char keluaran[3];
            keluaran[0] = '%';
            keluaran[1] = heks[c >> 4];
            keluaran[2] = heks[c & 0x0F];
            penyangga_tambah(penyangga, keluaran, 3);
        }
    }
    return !penyangga->gagal;
}
