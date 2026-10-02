#include "pak_write.h"
#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t card[5][256];
static unsigned writes;
static int fail_write=-1;
static int validate(unsigned p) { (void)p; return 0; }
static int free_blocks(unsigned p) { (void)p; return 123; }
static int read_sector(unsigned p,unsigned s,uint8_t *out) { (void)p; memcpy(out,card[s],256); return 0; }
static int read_note(unsigned p,unsigned s,PakNote *out) {
    (void)p; (void)s; *out=(PakNote){.name="TEST",.valid=true,.blocks=1}; return 0;
}
static int write_sector(unsigned p,unsigned s,const uint8_t *in) {
    (void)p; writes++;
    if((int)s==fail_write) return -1;
    memcpy(card[s],in,256); return 0;
}
static int delete_note(unsigned p,unsigned s) { (void)p; writes++; memset(card[3+s/8]+s%8*32,0,32); return 0; }
static int format(unsigned p) { (void)p; writes++; memset(card[3],0,512); return 0; }
static const PakReader reader={validate,free_blocks,read_sector,read_note};
static const PakWriter writer={write_sector,delete_note,format};
static void fixture(void) {
    memset(card,0,sizeof(card)); writes=0; fail_write=-1;
    const unsigned offsets[]={0x20,0x60,0x80,0xc0};
    for(unsigned i=0;i<4;i++) { card[0][offsets[i]+30]=255; card[0][offsets[i]+31]=0xf2; }
    for(unsigned i=5;i<128;i++) card[1][i*2+1]=3;
    card[1][1]=(123*3)&255; memcpy(card[2],card[1],256);
}
int main(void) {
    uint8_t repaired[3][256]; unsigned mask;
    fixture();
    assert(pak_repair_plan((const uint8_t (*)[256])card,repaired,&mask)==WRITE_NOTHING);
    card[0][0x60]=1; card[2][1]=0;
    assert(pak_repair_plan((const uint8_t (*)[256])card,repaired,&mask)==WRITE_OK && mask==5);
    PakInspector pak={.status=PAK_INVALID};
    PakWrite j={.action=PAK_ACTION_REPAIR,.phase=WRITE_PREPARE};
    pak_write_prepare(&j,&pak,&reader);
    assert(j.phase==WRITE_CONFIRM && j.repair_mask==5 && !writes);
    pak_write_execute(&j,&reader,&writer); assert(!writes); /* Review alone never writes. */
    j.phase=WRITE_EXECUTE; pak_write_execute(&j,&reader,&writer);
    assert(j.result==WRITE_OK && writes==2);
    assert(!memcmp(card[0]+0x20,card[0]+0x60,32) && !memcmp(card[1],card[2],256));
    fixture(); card[0][0x20]=1; card[0][0x3c]=1; card[0][0x3e]=0xfe;
    assert(pak_repair_plan((const uint8_t (*)[256])card,repaired,&mask)==WRITE_UNSAFE);
    fixture(); memset(card[0],255,256);
    assert(pak_repair_plan((const uint8_t (*)[256])card,repaired,&mask)==WRITE_UNSAFE);
    fixture(); card[2][11]=1; card[2][1]-=2;
    assert(pak_repair_plan((const uint8_t (*)[256])card,repaired,&mask)==WRITE_UNSAFE);
    fixture(); card[1][1]=card[2][1]=0;
    assert(pak_repair_plan((const uint8_t (*)[256])card,repaired,&mask)==WRITE_UNSAFE);
    fixture(); j=(PakWrite){.action=PAK_ACTION_FORMAT,.phase=WRITE_PREPARE};
    pak_write_prepare(&j,&pak,&reader); card[4][8]=1;
    j.phase=WRITE_EXECUTE; pak_write_execute(&j,&reader,&writer);
    assert(j.result==WRITE_CHANGED && !writes);
    fixture(); card[3][0]=1; pak.status=PAK_READY; memcpy(pak.directory,card[3],512);
    j=(PakWrite){.action=PAK_ACTION_DELETE,.phase=WRITE_PREPARE};
    pak_write_prepare(&j,&pak,&reader); assert(j.phase==WRITE_CONFIRM && !strcmp(j.name,"TEST"));
    j.phase=WRITE_EXECUTE; pak_write_execute(&j,&reader,&writer);
    assert(j.result==WRITE_OK && writes==1 && card[3][0]==0);
    fixture(); card[3][0]=1;
    j=(PakWrite){.action=PAK_ACTION_FORMAT,.phase=WRITE_PREPARE};
    pak_write_prepare(&j,&pak,&reader); j.phase=WRITE_EXECUTE;
    pak_write_execute(&j,&reader,&writer); assert(j.result==WRITE_OK && writes==1);
    fixture(); card[2][1]=0; fail_write=2;
    j=(PakWrite){.action=PAK_ACTION_REPAIR,.phase=WRITE_PREPARE};
    pak_write_prepare(&j,&pak,&reader); j.phase=WRITE_EXECUTE;
    pak_write_execute(&j,&reader,&writer); assert(j.result==WRITE_IO && writes==1);
    /* Holding immediately works; early release resets the hold; B cancels. */
    App a={.testing=true,.tool=TOOL_PAK}; Input in[4]={{true,BTN_A,0,0}};
    a.pak_write=(PakWrite){.phase=WRITE_CONFIRM,.action=PAK_ACTION_FORMAT};
    for(int i=0;i<19;i++) app_update(&a,in,100);
    assert(a.pak_write.phase==WRITE_CONFIRM && a.pak_write.hold_ms==1900);
    in[0].buttons=0; app_update(&a,in,16); assert(!a.pak_write.hold_ms);
    in[0].buttons=BTN_A; for(int i=0;i<20;i++) app_update(&a,in,100);
    assert(a.pak_write.phase==WRITE_EXECUTE);
    in[0].buttons=BTN_B; app_update(&a,in,16); assert(a.pak_write.phase==WRITE_IDLE);
    puts("Pak repair planning, write verification and confirmation tests passed");
}
