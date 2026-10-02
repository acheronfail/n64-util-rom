#include "app.h"
#include "tone.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static void press(App *a,Input *in,uint16_t buttons) {
    in[0].buttons=0; app_update(a,in,16);
    in[0].buttons=buttons; app_update(a,in,16);
}
int main(void) {
    App a={0};
    Input in[PORT_COUNT]={{true,0,0,0},{true,0,0,0}};
    press(&a,in,BTN_DU);
    assert(a.menu_index==TOOL_PAK);
    press(&a,in,BTN_DD);
    assert(a.menu_index==TOOL_CONTROLLER);
    press(&a,in,BTN_DD);
    a.rumble_supported[0]=true;
    press(&a,in,BTN_A);
    assert(a.tool==TOOL_RUMBLE && !app_rumble_active(&a,0));
    press(&a,in,BTN_A);
    assert(app_rumble_active(&a,0) && !app_rumble_active(&a,1));
    in[0].buttons=0; app_update(&a,in,16);
    assert(!app_rumble_active(&a,0));
    press(&a,in,BTN_B);
    assert(a.pulse_ms==900 && app_rumble_active(&a,0));
    in[0].buttons=0;
    for(unsigned segment=1;segment<=6;segment++) {
        app_update(&a,in,150);
        assert(app_rumble_active(&a,0)==(segment<6 && segment%2==0));
    }
    assert(!a.pulse_ms);
    press(&a,in,BTN_B);
    press(&a,in,BTN_Z);
    assert(a.pulse_ms>0 && app_rumble_active(&a,0)); /* Z no longer stops pulses. */
    in[0].buttons=0;
    app_update(&a,in,900);
    assert(!a.pulse_ms && !app_rumble_active(&a,0));
    press(&a,in,BTN_A);
    a.rumble_supported[0]=false; app_update(&a,in,16);
    assert(!app_rumble_active(&a,0));
    a.rumble_supported[0]=true; app_update(&a,in,16);
    assert(!app_rumble_active(&a,0)); /* Reinserted Pak cannot start a held A. */
    press(&a,in,BTN_A);
    in[1].buttons=BTN_START;
    for(int i=0;i<12;i++) app_update(&a,in,100);
    assert(a.active_port==1 && !app_rumble_active(&a,0) && !app_rumble_active(&a,1));
    a=(App){0}; in[0].buttons=in[1].buttons=0;
    press(&a,in,BTN_DU);
    press(&a,in,BTN_DU);
    press(&a,in,BTN_A);
    assert(a.tool==TOOL_AUDIO && !a.audio_playing && app_tone_hz(&a)==440);
    press(&a,in,BTN_A); assert(a.audio_playing);
    press(&a,in,BTN_DL); assert(a.audio_channel==0);
    press(&a,in,BTN_DR); assert(a.audio_channel==1);
    press(&a,in,BTN_DR); assert(a.audio_channel==2);
    press(&a,in,BTN_DU); assert(app_tone_hz(&a)==1000);
    press(&a,in,BTN_DU); assert(app_tone_hz(&a)==220);
    press(&a,in,BTN_B); assert(a.audio_playing); /* B is no longer an audio control. */
    press(&a,in,BTN_A); assert(!a.audio_playing);
    press(&a,in,BTN_A); assert(a.audio_playing);
    in[0].connected=false; app_update(&a,in,16); assert(!a.audio_playing);
    in[0].connected=true;
    press(&a,in,BTN_A); assert(a.audio_playing);
    in[1].buttons=BTN_START;
    for(int i=0;i<12;i++) app_update(&a,in,100);
    assert(a.active_port==1 && !a.audio_playing);
    in[1].buttons=0; app_update(&a,in,16);
    in[1].buttons=BTN_A; app_update(&a,in,16); assert(a.audio_playing);
    in[1].buttons=BTN_START;
    for(int i=0;i<12;i++) app_update(&a,in,100);
    assert(!a.testing && !a.audio_playing);

    /* Validate actual generated PCM, not just the UI's requested channel. */
    static int16_t pcm[32000*2];
    for(unsigned channel=0;channel<3;channel++) {
        Tone t={0};
        tone_render(&t,pcm,32000,32000,440,channel,true);
        unsigned crossings=0;
        for(unsigned i=1;i<32000;i++) {
            assert(abs(pcm[2*i])<=6553 && abs(pcm[2*i+1])<=6553);
            if(channel==0) assert(pcm[2*i+1]==0);
            if(channel==2) assert(pcm[2*i]==0);
            if(channel==1) assert(pcm[2*i]==pcm[2*i+1]);
            unsigned c=channel==2?1:0;
            if(pcm[2*(i-1)+c]<=0 && pcm[2*i+c]>0) crossings++;
        }
        assert(crossings>=439 && crossings<=441);
        tone_render(&t,pcm,1000,32000,440,channel,false);
        for(unsigned i=170;i<1000;i++) assert(!pcm[2*i] && !pcm[2*i+1]);
    }
    puts("utility controls, rumble sequencing and stereo PCM tests passed");
}
