/* catat: log ke stderr (stdout dipakai protokol MCP). Jangan pernah mencatat kredensial. */
#ifndef CATAT_H
#define CATAT_H

void catat_info(const char *format, ...);
void catat_galat(const char *format, ...);

#endif
