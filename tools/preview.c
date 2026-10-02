/* Host layout proof using the same UI code; not an N64 emulator. */
#include "app.h"
#include "ui.h"
#include "draw.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
void rect(float x,float y,float w,float h,uint32_t c) {
    printf("<rect x='%g' y='%g' width='%g' height='%g' fill='#%06x'/>\n",x,y,w,h,c);
}
void triangle(float x,float y,float xx,float yy,float xxx,float yyy,uint32_t c) {
    printf("<path d='M%g %g L%g %g L%g %gZ' fill='#%06x'/>\n",x,y,xx,yy,xxx,yyy,c);
}
void label(float x,float y,int style,const char *s) {
    const unsigned colors[]={0xe8eff4,0x8e9daa,0x69e0bb,0x101722};
    printf("<text x='%g' y='%g' font-family='monospace' font-size='7' fill='#%06x' stroke='#%06x' stroke-width='0.6' paint-order='stroke'>",x,y,colors[style],style==3?0xe8eff4:0x101722);
    for(;*s;s++) {
        if(*s=='&') fputs("&amp;",stdout);
        else if(*s=='<') fputs("&lt;",stdout);
        else if(*s=='>') fputs("&gt;",stdout);
        else putchar(*s);
    }
    puts("</text>");
}
void button_label(float x,float y,int style,char glyph) {
    const unsigned colors[]={0xe8eff4,0x8e9daa,0x69e0bb,0x101722};
    printf("<text x='%g' y='%g' text-anchor='middle' "
           "font-family='monospace' font-size='7' fill='#%06x' stroke='#%06x' "
           "stroke-width='0.6' paint-order='stroke'>%c</text>\n",
           x,y+2.5f,colors[style],style==3?0xe8eff4:0x101722,glyph);
}
static void update(App *a,bool connected,uint16_t buttons,int x,int y,unsigned ms) {
    Input inputs[PORT_COUNT]={{connected,buttons,x,y},{true,0,0,0}};
    app_update(a,inputs,ms);
}
int main(int argc,char **argv) {
    App a={0};
    update(&a,true,0,0,0,16);
    if(argc>1 && strcmp(argv[1],"menu")) {
        update(&a,true,BTN_A,0,0,16);
        for(int i=0;i<128;i++) {
            float t=i*6.2831853f/128;
            update(&a,true,0,85*cosf(t),85*sinf(t),16);
        }
        update(&a,true,BTN_A|BTN_CU|BTN_R,60,60,16);
        if(!strcmp(argv[1],"disconnected")) update(&a,false,0,0,0,16);
        if(!strcmp(argv[1],"all")) update(&a,true,0x3fff,-128,127,16);
    }
    if(argc>1 && !strcmp(argv[1],"switch")) {
        Input inputs[PORT_COUNT]={{true,0,60,60},{true,BTN_START,-40,20}};
        for(int i=0;i<7;i++) app_update(&a,inputs,100);
    }
    if(argc>1 && (!strcmp(argv[1],"rumble") || !strcmp(argv[1],"rumble-missing"))) {
        a.tool=TOOL_RUMBLE;
        a.controls_armed=true;
        a.rumble_supported[0]=strcmp(argv[1],"rumble-missing")!=0;
        a.pulse_ms=850;
    }
    if(argc>1 && !strcmp(argv[1],"audio")) {
        a.tool=TOOL_AUDIO;
        a.audio_channel=0;
        a.audio_playing=true;
    }
    if(argc>1 && !strncmp(argv[1],"pak",3)) {
        a.tool=TOOL_PAK;
        a.pak.status=PAK_READY;
        a.pak.free_blocks=99;
        a.pak.count=8;
        for(unsigned i=0;i<8;i++) {
            a.pak.notes[i]=(PakNote){.vendor=0x4E4445,.game_id=0x3031,.region=0x45,
                .blocks=3,.slot=i,.valid=true};
            snprintf(a.pak.notes[i].name,19,"RACING SAVE %u",i+1);
        }
        if(!strcmp(argv[1],"pak-details")) a.pak.details=true;
        if(!strcmp(argv[1],"pak-empty")) { a.pak.count=0; a.pak.free_blocks=123; }
        if(!strcmp(argv[1],"pak-error")) a.pak.status=PAK_IO_ERROR;
        if(!strcmp(argv[1],"pak-invalid")) {
            a.pak.status=PAK_INVALID; a.pak.error_code=-3;
            a.pak.scan_number=3; a.pak.feedback_ms=600;
            a.pak.diagnostic_stage=3; a.pak.header_matches=true; a.pak.all_ff=true;
        }
        if(!strcmp(argv[1],"pak-reading")) { a.pak.status=PAK_READING; a.pak.stage=11; }
        if(!strcmp(argv[1],"pak-last")) a.pak.selected=7;
    }
    if(argc>1 && !strcmp(argv[1],"pak-format")) {
        a.pak_write=(PakWrite){.action=PAK_ACTION_FORMAT,.phase=WRITE_CONFIRM,.hold_ms=800};
    }
    puts("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 320 240' width='960' height='720'>");
    ui_draw(&a);
    puts("</svg>");
}
