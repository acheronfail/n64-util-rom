#include "pak.h"
#include <string.h>
static uint32_t hash(uint32_t value,const uint8_t *bytes) {
    for(unsigned i=0;i<256;i++) value=(value^bytes[i])*16777619u;
    return value;
}
static unsigned be16(const uint8_t *p) { return ((unsigned)p[0]<<8)|p[1]; }
void pak_diagnose(PakInspector *p,unsigned sector,const uint8_t *data) {
    if(sector==0) {
        const unsigned offsets[]={0x20,0x60,0x80,0xc0};
        p->all_zero=p->all_ff=p->header_matches=true;
        p->header_copies=0;
        for(unsigned i=0;i<256;i++) {
            p->all_zero &= data[i]==0;
            p->all_ff &= data[i]==255;
        }
        for(unsigned i=0;i<4;i++) {
            const uint8_t *copy=data+offsets[i];
            unsigned sum=0;
            for(unsigned j=0;j<28;j+=2) sum+=be16(copy+j);
            sum &= 0xffff;
            /* Report the pinned library's exact validation criteria. */
            if(sum==be16(copy+28) && (int)sum==0xfff2-(int)be16(copy+30)) p->header_copies++;
            if(memcmp(data+0x20,copy,16)) p->header_matches=false;
        }
    } else if(sector<=2) {
        unsigned sum=0;
        for(unsigned i=5;i<128;i++) sum+=data[i*2+1];
        p->toc_valid[sector-1]=(sum&255)==data[1];
    }
}
void pak_reset(PakInspector *p) { memset(p,0,sizeof(*p)); }
static void begin(PakInspector *p,unsigned port) {
    pak_reset(p); p->port=port; p->status=PAK_READING;
    p->feedback_ms=750;
    p->signature=2166136261u;
}
static void failure(PakInspector *p,int error) {
    unsigned port=p->port, feedback=p->feedback_ms;
    pak_reset(p); p->port=port;
    p->feedback_ms=feedback; p->error_code=error;
    p->status=error==-3 ? PAK_INVALID : PAK_IO_ERROR;
}
void pak_move(PakInspector *p,int direction) {
    if(p->status!=PAK_READY || !p->count) return;
    if(direction<0 && p->selected) p->selected--;
    if(direction>0 && p->selected+1<p->count) p->selected++;
}
void pak_tick(PakInspector *p,const PakReader *r,bool enabled,unsigned port,
              PakPresence presence,bool refresh,unsigned ms) {
    if(!enabled) { pak_reset(p); return; }
    p->feedback_ms=ms>=p->feedback_ms ? 0 : p->feedback_ms-ms;
    if(presence!=PAK_PRESENT) {
        unsigned feedback=refresh?750:p->feedback_ms;
        pak_reset(p); p->port=port;
        p->feedback_ms=feedback;
        p->status=presence==PAK_NO_CONTROLLER ? PAK_MISSING_CONTROLLER :
                  presence==PAK_ABSENT ? PAK_MISSING : PAK_WRONG;
        return;
    }
    if(refresh || p->port!=port || p->status==PAK_IDLE ||
       p->status==PAK_MISSING || p->status==PAK_WRONG || p->status==PAK_MISSING_CONTROLLER)
        begin(p,port);
    uint8_t data[256];
    int result;
    if(p->status==PAK_READY) return;
    if(p->status==PAK_INVALID && p->diagnostic_stage<3) {
        unsigned sector=p->diagnostic_stage++;
        result=r->sector(port,sector,data);
        if(result) p->diagnostic_error=true;
        else pak_diagnose(p,sector,data);
        return;
    }
    if(p->status!=PAK_READING) return; /* Errors wait for A or reinsertion. */
    if(p->stage==0) {
        result=r->validate(port);
        if(result) { failure(p,result); return; }
    } else if(p->stage==1) {
        result=r->free_blocks(port);
        if(result<0) { failure(p,-2); return; }
        if(result>PAK_BLOCKS) { failure(p,-3); return; }
        p->free_blocks=(unsigned)result;
    } else if(p->stage<=4) {
        unsigned sector=p->stage==2 ? 0 : p->stage;
        result=r->sector(port,sector,data);
        if(result) { failure(p,-2); return; }
        p->signature=hash(p->signature,data);
        if(sector>=3) memcpy(p->directory+(sector-3)*256,data,256);
    } else if(p->stage<5+PAK_SLOTS) {
        unsigned slot=p->stage-5;
        const uint8_t *raw=p->directory+slot*32;
        bool game=raw[0] || raw[1] || raw[2] || raw[3];
        bool company=raw[4] || raw[5];
        /* SDK deletion/repair can leave names and other bytes behind.
           Both identity fields zero marks an empty directory slot. Keep
           partially cleared identities visible as malformed entries. */
        if(game || company) {
            PakNote note={0};
            result=r->note(port,slot,&note);
            if(result) { failure(p,-2); return; }
            note.slot=slot;
            note.name[18]=0;
            /* Font escape codes and control characters are never interpreted. */
            for(unsigned i=0;i<18 && note.name[i];i++)
                if((unsigned char)note.name[i]<32 || (unsigned char)note.name[i]>126 ||
                   note.name[i]=='$' || note.name[i]=='^') note.name[i]='?';
            if(!game || !company || !note.blocks || note.blocks>PAK_BLOCKS) note.valid=false;
            if(!note.valid) p->invalid_count++;
            p->notes[p->count++]=note;
        }
    } else {
        /* Verify the directory/header did not change while reading notes. */
        unsigned phase=p->stage-(5+PAK_SLOTS);
        if(phase==0) p->probe_signature=2166136261u;
        unsigned sector=phase==0 ? 0 : phase+2;
        result=r->sector(port,sector,data);
        if(result) { failure(p,-2); return; }
        p->probe_signature=hash(p->probe_signature,data);
        if(phase==2) {
            if(p->probe_signature!=p->signature) { begin(p,port); return; }
            unsigned total=0;
            for(unsigned i=0;i<p->count;i++) if(p->notes[i].valid) total+=p->notes[i].blocks;
            p->inconsistent=p->invalid_count || total+p->free_blocks!=PAK_BLOCKS;
            p->status=PAK_READY;
        }
    }
    p->stage++;
}
