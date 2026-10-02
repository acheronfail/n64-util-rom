#ifndef PAK_N64_H
#define PAK_N64_H
#include "pak.h"
#include "pak_write.h"
extern const PakWriter pak_n64_writer;
extern const PakReader pak_n64_reader;
PakPresence pak_n64_presence(unsigned port);
#endif
