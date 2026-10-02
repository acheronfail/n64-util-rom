#ifndef PAK_H
#define PAK_H
#include <stdbool.h>
#include <stdint.h>
#define PAK_SLOTS 16
#define PAK_BLOCKS 123
#define PAK_BLOCK_BYTES 256
/* Accessory presence is supplied by the hardware adapter. */
typedef enum { PAK_NO_CONTROLLER, PAK_ABSENT, PAK_OTHER, PAK_PRESENT } PakPresence;
typedef enum { PAK_IDLE, PAK_MISSING_CONTROLLER, PAK_MISSING, PAK_WRONG,
    PAK_READING, PAK_READY, PAK_INVALID, PAK_IO_ERROR } PakStatus;
typedef struct {
    char name[19];
    uint32_t vendor;
    uint16_t game_id;
    uint8_t slot, region, blocks;
    bool valid;
} PakNote;
/* Read-only interface: no write/format/delete operation is available. */
typedef struct {
    int (*validate)(unsigned port);
    int (*free_blocks)(unsigned port);
    int (*sector)(unsigned port,unsigned sector,uint8_t *data);
    int (*note)(unsigned port,unsigned slot,PakNote *note);
} PakReader;
typedef struct {
    PakStatus status;
    unsigned port, stage, count, selected, free_blocks, invalid_count;
    unsigned poll_ms, probe_stage;
    unsigned scan_number, feedback_ms;
    int error_code;
    unsigned diagnostic_stage, header_copies;
    bool header_matches, toc_valid[2], diagnostic_error, all_zero, all_ff;
    bool details, inconsistent;
    uint32_t signature, probe_signature;
    uint8_t directory[512];
    PakNote notes[PAK_SLOTS];
} PakInspector;
void pak_diagnose(PakInspector *pak,unsigned sector,const uint8_t *data);
void pak_reset(PakInspector *pak);
void pak_tick(PakInspector *pak,const PakReader *reader,bool enabled,unsigned port,
              PakPresence presence,bool refresh,unsigned elapsed_ms);
void pak_move(PakInspector *pak,int direction);
#endif
