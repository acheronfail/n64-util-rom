#ifndef PAK_WRITE_H
#define PAK_WRITE_H
#include "pak.h"
#define PAK_CONFIRM_MS 2000
typedef enum { PAK_ACTION_NONE, PAK_ACTION_DELETE, PAK_ACTION_REPAIR, PAK_ACTION_FORMAT } PakAction;
typedef enum { WRITE_IDLE, WRITE_PREPARE, WRITE_CONFIRM, WRITE_EXECUTE, WRITE_DONE } PakWritePhase;
typedef enum { WRITE_OK, WRITE_IO, WRITE_CHANGED, WRITE_UNSAFE, WRITE_VERIFY, WRITE_NOTHING } PakWriteResult;
typedef struct {
    int (*sector)(unsigned port,unsigned index,const uint8_t *data);
    int (*delete_note)(unsigned port,unsigned slot);
    int (*format)(unsigned port);
} PakWriter;
typedef struct {
    PakAction action;
    PakWritePhase phase;
    PakWriteResult result;
    unsigned port, slot, hold_ms, repair_mask;
    bool armed;
    char name[19];
    uint8_t before[5][256], repaired[3][256];
} PakWrite;
/* Plan repairs in RAM only. Ambiguous sources are never chosen. */
PakWriteResult pak_repair_plan(const uint8_t data[5][256],uint8_t repaired[3][256],unsigned *mask);
void pak_write_prepare(PakWrite *job,const PakInspector *pak,const PakReader *reader);
void pak_write_execute(PakWrite *job,const PakReader *reader,const PakWriter *writer);
#endif
