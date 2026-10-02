#include "ui.h"
#include "draw.h"
#include <math.h>
#include <stdio.h>
#define BG 0x101722
#define PANEL 0x182332
#define GRID 0x263747
#define MUTED 0x657585
#define WHITE 0xe8eff4
#define MINT 0x69e0bb
#define RED 0xff737b
static void line(float x,float y,float xx,float yy,float width,uint32_t c) {
    float dx=xx-x, dy=yy-y, len=sqrtf(dx*dx+dy*dy);
    if (len < .01f) return;
    float nx=-dy/len*width/2, ny=dx/len*width/2;
    triangle(x+nx,y+ny,x-nx,y-ny,xx+nx,yy+ny,c);
    triangle(x-nx,y-ny,xx-nx,yy-ny,xx+nx,yy+ny,c);
}
static void circle(float x,float y,float r,uint32_t c) {
    for(int i=0;i<24;i++) {
        float a=i*6.2831853f/24, b=(i+1)*6.2831853f/24;
        triangle(x,y,x+cosf(a)*r,y+sinf(a)*r,x+cosf(b)*r,y+sinf(b)*r,c);
    }
}
static void button(float x,float y,float r,const char *s,uint32_t c,bool on) {
    circle(x,y+1,r+1,on ? WHITE : 0x394553);
    circle(x,y,r,on ? c : 0x454e59);
    line(x-r/2,y-r/2,x+r/3,y-r/2,1,on ? WHITE : MUTED);
    if(*s) button_label(x,y,on?3:1,*s);
}
static void arrow(float x,float y,int dx,int dy,bool on) {
    uint32_t c=on?0x272412:0x8a929b;
    float px=-dy*3, py=dx*3;
    triangle(x+dx*4,y+dy*4,x-dx*2+px,y-dy*2+py,x-dx*2-px,y-dy*2-py,c);
}
static void buttons(const Controller *a) {
    uint16_t b=a->buttons;
    label(231,43,1,"BUTTONS");
    /* Two aligned columns for paired buttons; directional groups share a centre. */
    for(int i=0;i<2;i++) {
        bool on=b&(i?BTN_R:BTN_L);
        float x=228+i*40;
        rect(x,53,28,14,on?0xbac5d2:0x454e59);
        rect(x+3,51,22,2,on?WHITE:MUTED);
        button_label(x+14,60,on?3:1,i?'R':'L');
    }
    button(242,87,11,"B",0x36be70,b&BTN_B);
    button(282,87,11,"A",0x458bec,b&BTN_A);
    const int dx[]={0,0,-1,1},dy[]={-1,1,0,0};
    const uint16_t bits[]={BTN_CU,BTN_CD,BTN_CL,BTN_CR};
    for(int i=0;i<4;i++) {
        float x=262+dx[i]*15, y=126+dy[i]*15;
        button(x,y,7,"",0xf6ce43,b&bits[i]);
        arrow(x,y,dx[i],dy[i],b&bits[i]);
    }
    /* A 32-pixel cross, with arms proportional to the smaller C buttons. */
    rect(256,155,12,32,0x454e59); rect(246,165,32,12,0x454e59);
    const uint16_t db[]={BTN_DU,BTN_DD,BTN_DL,BTN_DR};
    for(int i=0;i<4;i++) {
        float x=262+dx[i]*10,y=171+dy[i]*10;
        if(b&db[i]) rect(x-5,y-5,10,10,0xc9d5e0);
        arrow(x,y,dx[i],dy[i],b&db[i]);
    }
    button(242,204,9,"S",0xe95858,b&BTN_START);
    bool z=b&BTN_Z;
    rect(274,192,16,25,z?0xbac5d2:0x454e59);
    rect(277,190,10,2,z?WHITE:MUTED);
    button_label(282,204.5f,z?3:1,'Z');
}
static float sx(int x) { return 115+x*.56f; }
static float sy(int y) { return 119-y*.56f; }
/* Five-pixel axis markers stay legible without competing with live values. */
static void axis_label(float x,float y,bool vertical) {
    const uint8_t plus[]={0,2,7,2,0};
    const uint8_t x_glyph[]={5,5,2,5,5};
    const uint8_t y_glyph[]={5,5,2,2,2};
    for(int glyph=0;glyph<2;glyph++) {
        const uint8_t *rows=glyph ? (vertical?y_glyph:x_glyph) : plus;
        for(int row=0;row<5;row++)
            for(int col=0;col<3;col++)
                if(rows[row] & (4>>col)) rect(x+glyph*4+col,y+row,1,1,MUTED);
    }
}
static void plot(const Controller *a, unsigned port) {
    rect(18,34,194,184,PANEL);
    for(int i=-100;i<=100;i+=25) {
        line(sx(i),sy(-125),sx(i),sy(125),1,i==0?MUTED:GRID);
        line(sx(-125),sy(i),sx(125),sy(i),1,i==0?MUTED:GRID);
    }
    /* Illustrative gate only, not a calibration or pass/fail boundary. */
    const int gx[]={0,60,85,60,0,-60,-85,-60};
    const int gy[]={85,60,0,-60,-85,-60,0,60};
    for(int i=0;i<8;i++) line(sx(gx[i]),sy(gy[i]),sx(gx[(i+1)%8]),sy(gy[(i+1)%8]),1.5f,0x476c72);
    label(23,40,1,"RAW STICK");
    axis_label(sx(0)+4,sy(125),true);
    axis_label(sx(125)+4,sy(0)-2,false);
    if(!a->connected) {
        char message[32];
        snprintf(message,sizeof(message),"CONNECT TO PORT %u",port+1);
        label(37,130,0,message);
        return;
    }
    /* Join consecutive samples in time order, including across ring wrap.
       Never join the newest sample back to the oldest one. */
    for(unsigned i=1;i<a->count;i++) {
        unsigned n=(a->next+TRAIL_SIZE-a->count+i)%TRAIL_SIZE;
        Point previous=a->trail[(n+TRAIL_SIZE-1)%TRAIL_SIZE];
        Point current=a->trail[n];
        line(sx(previous.x),sy(previous.y),sx(current.x),sy(current.y),
             1.5f,i>a->count/2?0x428d83:0x2b575c);
    }
    line(115,119,sx(a->x),sy(a->y),1,MINT);
    circle(sx(a->x),sy(a->y),4,WHITE);
    circle(sx(a->x),sy(a->y),2,MINT);
    char s[64];
    snprintf(s,sizeof(s),"X %+4d  Y %+4d",a->x,a->y); label(40,202,2,s);
    snprintf(s,sizeof(s),"Min/max X[%d,%d] Y[%d,%d]",a->min_x,a->max_x,a->min_y,a->max_y);
    label(23,213,1,s);
}
static void rumble_screen(const App *a) {
    const Controller *p=&a->ports[a->active_port];
    bool available=p->connected && a->rumble_supported[a->active_port];
    bool on=app_rumble_active(a,a->active_port);
    rect(34,51,252,87,PANEL);
    label(48,72,available?2:1,!p->connected?"Connect a controller":
          available?"Rumble Pak detected":"No Rumble Pak detected");
    circle(62,104,9,on?MINT:GRID);
    label(82,108,on?2:0,on?"MOTOR ON":a->pulse_ms?"PULSE PAUSE":"MOTOR OFF");
    if(a->pulse_ms) rect(48,126,224.0f*a->pulse_ms/900,2,MINT);
    label(48,161,0,"A  Hold to rumble");
    label(48,179,0,"B  Three short pulses");
}
static void audio_screen(const App *a) {
    bool playing=a->audio_playing && a->ports[a->active_port].connected;
    const char *channels[]={"LEFT","BOTH","RIGHT"};
    rect(34,48,252,102,PANEL);
    char text[40];
    snprintf(text,sizeof(text),"%u Hz / %s",app_tone_hz(a),channels[a->audio_channel]);
    label(48,68,2,text);
    for(int i=0;i<2;i++) {
        bool on=playing && (a->audio_channel==1 || a->audio_channel==(i?2:0));
        rect(48+i*124,84,100,27,on?MINT:GRID);
        button_label(98+i*124,97.5f,on?3:1,i?'R':'L');
    }
    label(48,132,playing?2:1,playing?"PLAYING / SINE / 20%":"STOPPED / SINE / 20%");
    label(34,169,0,playing?"A  Stop":"A  Play");
    label(34,187,1,"Stick / D-pad: left/right Channel");
    label(34,205,1,"Stick / D-pad: up/down Frequency");
}
static void pak_screen(const PakInspector *p) {
    char text[80];
    label(18,43,1,"READ ONLY");
    if(p->feedback_ms) {
        rect(190,33,112,14,GRID);
        label(196,43,2,"Rescan");
    }
    if(p->status!=PAK_READY) {
        const char *message="Reading Controller Pak...";
        switch(p->status) {
            case PAK_MISSING_CONTROLLER: message="Connect a controller"; break;
            case PAK_MISSING: message="Insert a Controller Pak"; break;
            case PAK_WRONG: message="Accessory is not a Controller Pak"; break;
            case PAK_INVALID: message="Invalid or unformatted Pak"; break;
            case PAK_IO_ERROR: message="Could not read Controller Pak"; break;
            default: break;
        }
        rect(18,60,284,77,PANEL);
        label(28,96,0,message);
        if(p->status==PAK_READING) {
            unsigned read=p->stage>5?p->stage-5:0;
            if(read>PAK_SLOTS) read=PAK_SLOTS;
            snprintf(text,sizeof(text),"Directory %u / 16",read);
            label(28,119,1,text);
            rect(28,131,264.0f*p->stage/24,2,MINT);
        } else {
            label(28,119,1,"A  Rescan");
            if(p->error_code) {
                snprintf(text,sizeof(text),"Validation/read code: %d",p->error_code);
                label(28,160,1,text);
                if(p->status==PAK_INVALID) {
                    if(p->diagnostic_stage<3) label(28,179,1,"Reading diagnostic sectors...");
                    else if(p->diagnostic_error) label(28,179,1,"Diagnostic sector read failed");
                    else {
                        snprintf(text,sizeof(text),"Header: %u/4 checks; copies %s",p->header_copies,p->header_matches?"match":"differ");
                        label(28,179,1,text);
                        snprintf(text,sizeof(text),"Allocation checks: %s / %s",p->toc_valid[0]?"OK":"BAD",p->toc_valid[1]?"OK":"BAD");
                        label(28,195,1,text);
                        if(p->all_zero || p->all_ff)
                            label(28,211,1,p->all_zero?"Header data is all 00":"Header data is all FF");
                    }
                }
            }
        }
        return;
    }
    snprintf(text,sizeof(text),"Used %u/123   Free %u blocks",PAK_BLOCKS-p->free_blocks,p->free_blocks);
    label(18,57,0,text);
    rect(18,63,284,3,GRID);
    if(p->free_blocks<PAK_BLOCKS) rect(18,63,284.0f*(PAK_BLOCKS-p->free_blocks)/PAK_BLOCKS,3,MINT);
    if(!p->count) {
        label(36,115,0,"No save notes");
        label(36,134,1,"0 / 16 directory slots used");
    } else if(p->details) {
        const PakNote *n=&p->notes[p->selected];
        snprintf(text,sizeof(text),"Slot %02u  %s",n->slot+1,n->valid?n->name:"INVALID ENTRY");
        label(18,85,2,text);
        if(n->valid) {
            snprintf(text,sizeof(text),"Size: %u blocks / %u bytes",n->blocks,n->blocks*PAK_BLOCK_BYTES);
            label(18,108,0,text);
        } else label(18,108,0,"Invalid note metadata or block chain");
        snprintf(text,sizeof(text),"Game ID: %04X",n->game_id); label(18,130,1,text);
        snprintf(text,sizeof(text),"Vendor:  %06lX",(unsigned long)n->vendor); label(18,148,1,text);
        snprintf(text,sizeof(text),"Region:  %02X",n->region); label(18,166,1,text);
        snprintf(text,sizeof(text),"Note %u / %u",p->selected+1,p->count); label(18,184,1,text);
    } else {
        unsigned first=(p->selected/6)*6;
        for(unsigned i=first;i<p->count && i<first+6;i++) {
            const PakNote *n=&p->notes[i];
            float y=74+(i-first)*18;
            if(i==p->selected) { rect(18,y,284,17,PANEL); rect(18,y,2,17,MINT); }
            snprintf(text,sizeof(text),"%02u %s",n->slot+1,n->valid?(n->name[0]?n->name:"(unnamed)"):"(invalid entry)");
            label(26,y+12,i==p->selected?2:0,text);
            if(n->valid) snprintf(text,sizeof(text),"%3u blk",n->blocks);
            else snprintf(text,sizeof(text),"  ? blk");
            label(252,y+12,1,text);
        }
        snprintf(text,sizeof(text),"Note %u/%u   Slots %u/16",p->selected+1,p->count,p->count);
        label(18,190,1,text);
    }
    if(p->inconsistent) label(18,199,1,"Check notes / allocation mismatch");
    label(18,211,1,p->count?"Stick/D-pad Scroll  B Details  A Rescan":"A Rescan");
}
static void pak_write_screen(const PakWrite *j) {
    const char *names[]={"","DELETE SAVE","REPAIR REDUNDANT COPIES","FORMAT BANK"};
    bool danger=j->action==PAK_ACTION_FORMAT || j->action==PAK_ACTION_DELETE;
    label(18,51,danger?4:2,names[j->action]);
    char text[64];
    snprintf(text,sizeof(text),"Controller P%u / selected bank",j->port+1);
    label(18,72,1,text);
    if(j->phase==WRITE_DONE) {
        const char *results[]={"Complete - verified","I/O error - may be partially written",
            "Pak changed - nothing written","No unambiguous repair / unsafe entry",
            "Verification failed after write","No repair needed"};
        label(18,106,0,results[j->result]);
        label(18,181,1,"B  Return and rescan");
        return;
    }
    if(j->action==PAK_ACTION_DELETE) {
        snprintf(text,sizeof(text),"Slot %02u: %s",j->slot+1,j->name);
        label(18,98,0,text);
        label(18,121,1,"This save will be permanently deleted.");
    } else if(j->action==PAK_ACTION_FORMAT) {
        label(18,101,4,"ALL saves on this bank will be erased.");
    } else {
        snprintf(text,sizeof(text),"Restore:%s%s%s",j->repair_mask&1?" header":"",
                 j->repair_mask&2?" table 1":"",j->repair_mask&4?" table 2":"");
        label(18,101,0,text);
        label(18,122,1,"Copies metadata; does not recover saves.");
    }
    label(18,174,danger?4:0,"Hold A to confirm");
    label(18,193,1,"B  Cancel");
    rect(18,204,284,3,danger?0x542c37:GRID);
    if(j->hold_ms) rect(18,204,284.0f*j->hold_ms/PAK_CONFIRM_MS,3,danger?RED:MINT);
}
void ui_draw(const App *a) {
    rect(0,0,320,240,BG);
    if(!a->testing) {
        const char *items[]={"Controller test","Rumble Pak test","Audio test","Controller Pak"};
        for(unsigned i=0;i<TOOL_COUNT;i++) {
            float y=68+i*26;
            bool selected=i==a->menu_index;
            if(selected) { rect(66,y,188,22,PANEL); rect(66,y,2,22,MINT); }
            label(76,y+14,selected?2:1,selected?">":" ");
            label(89,y+14,selected?2:1,items[i]);
        }
        return;
    }
    const char *titles[]={"CONTROLLER TEST","RUMBLE PAK TEST","AUDIO TEST","CONTROLLER PAK"};
    label(18,22,0,titles[a->tool]);
    for(unsigned i=0;i<PORT_COUNT;i++) {
        float x=218+i*22;
        char port[4];
        snprintf(port,sizeof(port),"P%u",i+1);
        label(x,22,a->ports[i].connected?(i==a->active_port?2:0):1,port);
        if(i==a->active_port) rect(x,25,14,1,MINT);
    }
    rect(18,28,284,1,GRID);
    if(a->tool==TOOL_CONTROLLER) {
        plot(&a->ports[a->active_port],a->active_port);
        buttons(&a->ports[a->active_port]);
    } else if(a->tool==TOOL_RUMBLE) rumble_screen(a);
    else if(a->tool==TOOL_AUDIO) audio_screen(a);
    else if(a->pak_write.phase!=WRITE_IDLE) pak_write_screen(&a->pak_write);
    else {
        pak_screen(&a->pak);
        if(a->pak.status!=PAK_READING)
            label(18,221,1,"C: Up Repair / Down Delete / Right Format");
    }
    if(a->gesture_port && a->gesture_port-1!=a->active_port) {
        char message[32];
        snprintf(message,sizeof(message),"Switch to P%u",a->gesture_port);
        label(18,231,2,message);
    } else label(18,231,1,"Hold START to exit");
    if(a->hold_ms) rect(18,235,284.0f*a->hold_ms/HOLD_MS,2,MINT);
    if(a->calibration_notice) {
        rect(12,45,296,159,BG);
        rect(16,49,288,151,GRID);
        rect(17,50,286,149,PANEL);
        label(30,73,2,"STICK CALIBRATION");
        label(30,97,0,"N64 controllers can reset their");
        label(30,110,0,"stick's neutral position.");
        label(30,132,0,"Let go of the stick, then hold L + R");
        label(30,145,0,"and press Start to reset it.");
        label(30,181,2,"Press any button to continue");
    }
}
