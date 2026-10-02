#include "pak.h"
#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t card[5][256];
static PakNote entries[16];
static int free_count, validation, fail_sector=-1, fail_note=-1;
static unsigned reads, last_port;
static int valid(unsigned port) { reads++; last_port=port; return validation; }
static int space(unsigned port) { reads++; last_port=port; return free_count; }
static int sector(unsigned port,unsigned index,uint8_t *data) {
    reads++; last_port=port;
    assert(index<5);
    if((int)index==fail_sector) return -2;
    memcpy(data,card[index],256); return 0;
}
static int note(unsigned port,unsigned slot,PakNote *out) {
    reads++; last_port=port;
    if((int)slot==fail_note) return -2;
    *out=entries[slot]; return 0;
}
static const PakReader reader={valid,space,sector,note};
static void fixture(unsigned count) {
    memset(card,0,sizeof(card)); memset(entries,0,sizeof(entries));
    free_count=123-(int)count*2; validation=0; fail_sector=fail_note=-1; reads=0;
    for(unsigned i=0;i<count;i++) {
        card[3+i/8][i%8*32]=1;
        entries[i]=(PakNote){.name="SAVE",.valid=true,.blocks=2,.region=0x45,.game_id=1};
    }
}
static void step(PakInspector *p,unsigned port,bool refresh) {
    unsigned before=reads;
    pak_tick(p,&reader,true,port,PAK_PRESENT,refresh,16);
    assert(reads-before<=1); /* A single bounded operation per frame. */
}
static void finish(PakInspector *p,unsigned port) {
    for(unsigned i=0;i<30 && p->status==PAK_READING;i++) step(p,port,false);
    assert(p->status==PAK_READY);
}
int main(void) {
    PakInspector p={0}; fixture(16);
    step(&p,2,false); finish(&p,2);
    assert(last_port==2 && p.count==16 && p.free_blocks==91 && !p.inconsistent);
    for(unsigned i=0;i<16;i++) assert(p.notes[i].slot==i);
    for(int i=0;i<20;i++) pak_move(&p,1);
    assert(p.selected==15);
    for(int i=0;i<20;i++) pak_move(&p,-1);
    assert(!p.selected);
    /* Same accessory type, different header or directory: discard and rescan. */
    card[0][2]=99;
    pak_tick(&p,&reader,true,2,PAK_PRESENT,false,1000);
    for(int i=0;i<3;i++) step(&p,2,false);
    assert(p.status==PAK_READING && !p.count);
    finish(&p,2);
    /* Switching port never displays the previous port's entries. */
    step(&p,1,false); assert(p.status==PAK_READING && !p.count && p.port==1);
    finish(&p,1);
    pak_tick(&p,&reader,true,1,PAK_ABSENT,false,16);
    assert(p.status==PAK_MISSING && !p.count);
    step(&p,1,false); finish(&p,1);
    pak_tick(&p,&reader,true,1,PAK_OTHER,false,16); assert(p.status==PAK_WRONG && !p.count);
    pak_tick(&p,&reader,true,1,PAK_NO_CONTROLLER,false,16); assert(p.status==PAK_MISSING_CONTROLLER);
    fixture(0); step(&p,0,false); finish(&p,0);
    assert(!p.count && p.free_blocks==123 && !p.inconsistent);
    /* Errors never leave partially scanned saves or misleading totals. */
    fixture(3); fail_note=1;
    step(&p,0,true);
    for(int i=0;i<24;i++) step(&p,0,false);
    assert(p.status==PAK_IO_ERROR && !p.count && !p.free_blocks);
    fail_note=-1; step(&p,0,true); finish(&p,0);
    validation=-3;
    pak_tick(&p,&reader,true,0,PAK_PRESENT,false,1000);
    assert(p.status==PAK_INVALID && !p.count);
    unsigned scan=p.scan_number;
    step(&p,0,true);
    assert(p.status==PAK_INVALID && p.scan_number==scan+1 && p.feedback_ms==750);
    assert(p.error_code==-3);
    step(&p,0,false); assert(p.feedback_ms==734);
    step(&p,0,true); assert(p.scan_number==scan+2 && p.feedback_ms==750);
    fixture(1); fail_sector=3; step(&p,0,true);
    for(int i=0;i<5;i++) step(&p,0,false);
    assert(p.status==PAK_IO_ERROR);
    fixture(1); free_count=124; step(&p,0,true); step(&p,0,false);
    assert(p.status==PAK_INVALID);
    fixture(2); entries[0].valid=false;
    strcpy(entries[1].name,"BAD^01$02\001NAME");
    step(&p,0,true); finish(&p,0);
    assert(p.count==2 && p.invalid_count==1 && p.inconsistent);
    assert(!strchr(p.notes[1].name,'^') && !strchr(p.notes[1].name,'$'));
    fixture(1); step(&p,0,true);
    for(int i=0;i<19;i++) step(&p,0,false);
    card[4][1]=42; /* Changed during scan: don't publish a mixed snapshot. */
    for(int i=0;i<4;i++) step(&p,0,false);
    assert(p.status==PAK_READING && !p.count);
    pak_tick(&p,&reader,false,0,PAK_PRESENT,false,16);
    assert(p.status==PAK_IDLE && !p.count);
    /* Inspector menu entry, detail toggle, selection and rescan controls. */
    App a={0}; Input in[4]={{true,BTN_DU,0,0}};
    app_update(&a,in,16); assert(a.menu_index==TOOL_PAK);
    in[0].buttons=BTN_A; app_update(&a,in,16);
    assert(a.testing && a.tool==TOOL_PAK && !a.pak_refresh);
    a.pak.status=PAK_READY; a.pak.count=2;
    in[0].buttons=0; app_update(&a,in,16);
    in[0].buttons=BTN_B; app_update(&a,in,16); assert(a.pak.details);
    in[0].buttons=BTN_DD; app_update(&a,in,16); assert(a.pak.selected==1);
    in[0].buttons=BTN_A; app_update(&a,in,16); assert(a.pak_refresh);
    app_update(&a,in,16); assert(!a.pak_refresh);
    uint8_t header[256]={0};
    const unsigned offsets[]={0x20,0x60,0x80,0xc0};
    for(unsigned i=0;i<4;i++) {
        header[offsets[i]+30]=0xff; header[offsets[i]+31]=0xf2;
    }
    pak_diagnose(&p,0,header);
    assert(p.header_copies==4 && p.header_matches && !p.all_zero && !p.all_ff);
    header[0x60]=1; pak_diagnose(&p,0,header);
    assert(p.header_copies==3 && !p.header_matches);
    memset(header,0,256); pak_diagnose(&p,0,header);
    assert(p.all_zero && !p.header_copies);
    memset(header,255,256); pak_diagnose(&p,0,header);
    assert(p.all_ff && !p.header_copies);
    memset(header,0,256); header[11]=3; header[1]=3;
    pak_diagnose(&p,1,header); assert(p.toc_valid[0]);
    header[1]=4; pak_diagnose(&p,2,header); assert(!p.toc_valid[1]);
    puts("Controller Pak scan, error, hot-swap and UI controls tests passed");
}
