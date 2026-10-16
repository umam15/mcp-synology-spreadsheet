/* pengodean: pengodean persen untuk bagian jalur URL. */
#ifndef PENGODEAN_H
#define PENGODEAN_H

#include "penyangga.h"

/* Menambah teks ke penyangga; semua byte selain A-Z a-z 0-9 - . _ ~ menjadi %XX. */
int pengodean_persen(penyangga_t *penyangga, const char *teks);

#endif
