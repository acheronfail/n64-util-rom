#include <libdragon.h>
#include "app.h"
#include "ui.h"
#include "draw.h"
#include "tone.h"
#include "pak_n64.h"
static color_t color(uint32_t c) { return RGBA32(c>>16,(c>>8)&255,c&255,255); }
void rect(float x,float y,float w,float h,uint32_t c) {
    rdpq_set_mode_fill(color(c));
    rdpq_fill_rectangle(x,y,x+w,y+h);
}
void triangle(float x,float y,float xx,float yy,float xxx,float yyy,uint32_t c) {
    float a[]={x,y},b[]={xx,yy},d[]={xxx,yyy};
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(color(c));
    rdpq_triangle(&TRIFMT_FILL,a,b,d);
}
void label(float x,float y,int style,const char *s) {
    rdpq_text_print(&(rdpq_textparms_t){.style_id=style},1,x,y,s);
}
void button_label(float x,float y,int style,char glyph) {
    rdpq_font_gmetrics_t metrics;
    const rdpq_font_t *font=rdpq_text_get_font(1);
    if(!rdpq_font_get_glyph_metrics(font,(unsigned char)glyph,&metrics)) return;
    char text[]={glyph,0};
    label(x-(metrics.x0+metrics.x1)*0.5f,
          y-(metrics.y0+metrics.y1)*0.5f,style,text);
}
static uint16_t read_buttons(joypad_buttons_t b) {
    return (b.a?BTN_A:0)|(b.b?BTN_B:0)|(b.start?BTN_START:0)|(b.z?BTN_Z:0)|
        (b.l?BTN_L:0)|(b.r?BTN_R:0)|(b.d_up?BTN_DU:0)|(b.d_down?BTN_DD:0)|
        (b.d_left?BTN_DL:0)|(b.d_right?BTN_DR:0)|(b.c_up?BTN_CU:0)|
        (b.c_down?BTN_CD:0)|(b.c_left?BTN_CL:0)|(b.c_right?BTN_CR:0);
}
static Tone tone;
static volatile unsigned tone_hz=440, tone_channel=1;
static volatile bool tone_playing;
static unsigned sample_rate;
static void fill_audio(short *buffer,size_t frames) {
    tone_render(&tone,buffer,frames,sample_rate,tone_hz,tone_channel,tone_playing);
}
int main(void) {
    debug_init_isviewer(); debug_init_emulog();
    timer_init(); joypad_init();
    display_init(RESOLUTION_320x240,DEPTH_16_BPP,3,GAMMA_NONE,FILTERS_RESAMPLE);
    rdpq_init();
    audio_init(32000,4);
    sample_rate=audio_get_frequency();
    audio_set_buffer_callback(fill_audio);
    /* Registering a callback does not start AI DMA in this libdragon version.
       Prime the queue once; completion interrupts then keep it supplied. */
    audio_write_silence();
    rdpq_font_t *font=rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_VAR);
    const uint32_t colors[]={0xe8eff4,0x8e9daa,0x69e0bb,0x101722};
    for(int i=0;i<4;i++) {
        rdpq_font_style(font,i,&(rdpq_fontstyle_t){
            .color=color(colors[i]),
            /* Dark pressed lettering needs a light outline on coloured caps. */
            .outline_color=color(i==3 ? 0xe8eff4 : 0x101722),
        });
    }
    rdpq_text_register_font(1,font);
    App app={0};
    uint64_t previous=get_ticks();
    while(1) {
        surface_t *frame=display_get();
        uint64_t now=get_ticks();
        unsigned ms=TIMER_MICROS_LL(now-previous)/1000;
        previous=now;
        joypad_poll();
        Input inputs[PORT_COUNT]={0};
        for(unsigned i=0;i<PORT_COUNT;i++) {
            joypad_port_t port=(joypad_port_t)i;
            joypad_inputs_t in=joypad_get_inputs(port);
            app.rumble_supported[i]=joypad_get_rumble_supported(port);
            inputs[i]=(Input){
                .connected=joypad_get_style(port)==JOYPAD_STYLE_N64,
                .buttons=read_buttons(in.btn), .x=in.stick_x, .y=in.stick_y,
            };
        }
        app_update(&app,inputs,ms);
        PakPresence presence=pak_n64_presence(app.active_port);
        PakWrite *job=&app.pak_write;
        if(job->phase!=WRITE_IDLE && (!app.testing || app.tool!=TOOL_PAK ||
           job->port!=app.active_port || presence!=PAK_PRESENT)) {
            *job=(PakWrite){0}; app.pak_refresh=true;
        }
        if(job->phase==WRITE_PREPARE) pak_write_prepare(job,&app.pak,&pak_n64_reader);
        else if(job->phase==WRITE_EXECUTE) pak_write_execute(job,&pak_n64_reader,&pak_n64_writer);
        if(job->phase==WRITE_IDLE)
            pak_tick(&app.pak,&pak_n64_reader,app.testing && app.tool==TOOL_PAK,
                     app.active_port,presence,app.pak_refresh,ms);
        for(unsigned i=0;i<PORT_COUNT;i++) {
            joypad_port_t port=(joypad_port_t)i;
            bool desired=app_rumble_active(&app,i);
            if(app.rumble_supported[i] && joypad_get_rumble_active(port)!=desired)
                joypad_set_rumble_active(port,desired);
        }
        disable_interrupts();
        tone_hz=app_tone_hz(&app);
        tone_channel=app.audio_channel;
        tone_playing=app.testing && app.tool==TOOL_AUDIO &&
                     app.ports[app.active_port].connected && app.audio_playing;
        enable_interrupts();
        rdpq_attach(frame,NULL);
        ui_draw(&app);
        rdpq_detach_show();
    }
}
