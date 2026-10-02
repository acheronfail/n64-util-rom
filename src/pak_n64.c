#include <libdragon.h>
#include <string.h>
#include "pak_n64.h"
static int validate(unsigned port) { return validate_mempak(port); }
static int free_blocks(unsigned port) { return get_mempak_free_space(port); }
static int sector(unsigned port,unsigned index,uint8_t *data) {
    return read_mempak_sector(port,index,data);
}
static int note(unsigned port,unsigned slot,PakNote *out) {
    entry_structure_t entry={0};
    int result=get_mempak_entry(port,slot,&entry);
    if(result) return result;
    memcpy(out->name,entry.name,sizeof(out->name));
    out->vendor=entry.vendor; out->game_id=entry.game_id;
    out->region=entry.region; out->blocks=entry.blocks; out->valid=entry.valid;
    return 0;
}
const PakReader pak_n64_reader={validate,free_blocks,sector,note};
PakPresence pak_n64_presence(unsigned port) {
    if(joypad_get_style(port)!=JOYPAD_STYLE_N64) return PAK_NO_CONTROLLER;
    joypad_accessory_type_t type=joypad_get_accessory_type(port);
    if(type==JOYPAD_ACCESSORY_TYPE_NONE) return PAK_ABSENT;
    if(type==JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) return PAK_PRESENT;
    return PAK_OTHER;
}

static int write_sector(unsigned port,unsigned index,const uint8_t *data) {
    return write_mempak_sector(port,index,(uint8_t *)data);
}
static int delete_note(unsigned port,unsigned slot) {
    entry_structure_t entry={0};
    if(get_mempak_entry(port,slot,&entry) || !entry.valid) return -1;
    return delete_mempak_entry(port,&entry);
}
static int format(unsigned port) { return format_mempak(port); }
const PakWriter pak_n64_writer={write_sector,delete_note,format};
