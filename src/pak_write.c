#include "pak_write.h"
#include <string.h>
static unsigned be16(const uint8_t *p) { return ((unsigned)p[0]<<8)|p[1]; }
static bool valid_header(const uint8_t *p) {
    unsigned sum=0;
    for(unsigned i=0;i<28;i+=2) sum+=be16(p+i);
    sum&=65535;
    return sum==be16(p+28) && (int)sum==0xfff2-(int)be16(p+30);
}
static bool valid_table(const uint8_t *p) {
    unsigned sum=0;
    for(unsigned i=5;i<128;i++) {
        unsigned link=be16(p+i*2);
        if(link!=1 && link!=3 && (link<5 || link>127)) return false;
        sum+=link;
    }
    return (sum&255)==p[1];
}
PakWriteResult pak_repair_plan(const uint8_t data[5][256],uint8_t repaired[3][256],unsigned *mask) {
    const unsigned offsets[]={0x20,0x60,0x80,0xc0};
    const uint8_t *source=NULL;
    *mask=0; memcpy(repaired,data,3*256);
    for(unsigned i=0;i<4;i++) {
        const uint8_t *candidate=data[0]+offsets[i];
        if(!valid_header(candidate)) continue;
        if(source && memcmp(source,candidate,32)) return WRITE_UNSAFE;
        source=candidate;
    }
    if(!source) return WRITE_UNSAFE;
    for(unsigned i=0;i<4;i++) {
        if(!valid_header(data[0]+offsets[i])) {
            memcpy(repaired[0]+offsets[i],source,32); *mask|=1;
        }
    }
    bool first=valid_table(data[1]), second=valid_table(data[2]);
    if(!first && !second) return WRITE_UNSAFE;
    if(first && second && memcmp(data[1],data[2],256)) return WRITE_UNSAFE;
    if(!first) { memcpy(repaired[1],data[2],256); *mask|=2; }
    if(!second) { memcpy(repaired[2],data[1],256); *mask|=4; }
    return *mask ? WRITE_OK : WRITE_NOTHING;
}
static void finish(PakWrite *j,PakWriteResult result) {
    j->result=result; j->phase=WRITE_DONE; j->hold_ms=0;
}
void pak_write_prepare(PakWrite *j,const PakInspector *pak,const PakReader *r) {
    if(j->phase!=WRITE_PREPARE) return;
    for(unsigned i=0;i<5;i++)
        if(r->sector(j->port,i,j->before[i])) { finish(j,WRITE_IO); return; }
    if(j->action==PAK_ACTION_DELETE) {
        if(pak->status!=PAK_READY || j->slot>=16 ||
           memcmp(pak->directory+j->slot*32,j->before[3+j->slot/8]+j->slot%8*32,32)) {
            finish(j,WRITE_CHANGED); return;
        }
        PakNote note={0};
        if(r->validate(j->port) || r->note(j->port,j->slot,&note) || !note.valid) {
            finish(j,WRITE_UNSAFE); return;
        }
        memcpy(j->name,note.name,18); j->name[18]=0;
        for(unsigned i=0;i<18 && j->name[i];i++)
            if((unsigned char)j->name[i]<32 || (unsigned char)j->name[i]>126 ||
                j->name[i]=='$' || j->name[i]=='^') j->name[i]='?';
    } else if(j->action==PAK_ACTION_REPAIR) {
        PakWriteResult result=pak_repair_plan((const uint8_t (*)[256])j->before,j->repaired,&j->repair_mask);
        if(result!=WRITE_OK) { finish(j,result); return; }
    } else if(j->action!=PAK_ACTION_FORMAT) { finish(j,WRITE_UNSAFE); return; }
    j->armed=false; j->hold_ms=0; j->phase=WRITE_CONFIRM;
}
void pak_write_execute(PakWrite *j,const PakReader *r,const PakWriter *w) {
    if(j->phase!=WRITE_EXECUTE) return;
    uint8_t data[256];
    /* Verify all reviewed metadata immediately before the first write. */
    for(unsigned i=0;i<5;i++) {
        if(r->sector(j->port,i,data)) { finish(j,WRITE_IO); return; }
        if(memcmp(data,j->before[i],256)) { finish(j,WRITE_CHANGED); return; }
    }
    if(j->action==PAK_ACTION_REPAIR) {
        for(unsigned i=0;i<3;i++) if(j->repair_mask&(1u<<i)) {
            if(w->sector(j->port,i,j->repaired[i])) { finish(j,WRITE_IO); return; }
            if(r->sector(j->port,i,data) || memcmp(data,j->repaired[i],256)) {
                finish(j,WRITE_VERIFY); return;
            }
        }
        if(r->validate(j->port)) { finish(j,WRITE_VERIFY); return; }
    } else if(j->action==PAK_ACTION_DELETE) {
        if(w->delete_note(j->port,j->slot)) { finish(j,WRITE_IO); return; }
        if(r->sector(j->port,3+j->slot/8,data)) { finish(j,WRITE_VERIFY); return; }
        for(unsigned i=0;i<32;i++) if(data[j->slot%8*32+i]) { finish(j,WRITE_VERIFY); return; }
        if(r->validate(j->port)) { finish(j,WRITE_VERIFY); return; }
    } else if(j->action==PAK_ACTION_FORMAT) {
        if(w->format(j->port)) { finish(j,WRITE_IO); return; }
        if(r->validate(j->port) || r->free_blocks(j->port)!=123) { finish(j,WRITE_VERIFY); return; }
        for(unsigned sector=3;sector<=4;sector++) {
            if(r->sector(j->port,sector,data)) { finish(j,WRITE_VERIFY); return; }
            for(unsigned i=0;i<256;i++) if(data[i]) { finish(j,WRITE_VERIFY); return; }
        }
    } else { finish(j,WRITE_UNSAFE); return; }
    finish(j,WRITE_OK);
}
