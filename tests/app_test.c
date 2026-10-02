#include "app.h"
#include <assert.h>
#include <stdio.h>
static void tick(App *a,Input *in,unsigned count) {
    for(unsigned i=0;i<count;i++) app_update(a,in,100);
}
int main(void) {
    App a={0};
    Input in[PORT_COUNT]={0};
    in[0]=(Input){false,BTN_A,100,100};
    tick(&a,in,1);
    assert(!a.testing && !a.ports[0].buttons);
    in[2]=(Input){true,BTN_START,10,20};
    tick(&a,in,20);
    assert(a.testing && a.active_port==2 && !a.hold_ms);
    assert(a.calibration_notice);
    in[2].buttons=0;
    tick(&a,in,1); assert(a.calibration_notice);
    in[2].buttons=BTN_START; tick(&a,in,1);
    assert(!a.calibration_notice && !a.gesture_port);
    tick(&a,in,20); assert(a.testing && !a.hold_ms);
    in[2].buttons=0;
    in[0]=(Input){true,0,-128,127};
    tick(&a,in,1);
    in[0].x=127; in[0].y=-128;
    tick(&a,in,300);
    assert(a.ports[0].count==TRAIL_SIZE);
    assert(a.ports[0].min_x==-128 && a.ports[0].max_x==127);
    assert(a.ports[0].min_y==-128 && a.ports[0].max_y==127);
    /* Other utilities retain hold-to-switch. */
    a.tool=TOOL_RUMBLE;
    /* Cancel a switch without affecting the selected controller. */
    in[0].buttons=BTN_START;
    tick(&a,in,7);
    assert(a.active_port==2 && a.gesture_port==1 && a.hold_ms==700);
    in[0].buttons=0;
    tick(&a,in,1);
    assert(a.active_port==2 && !a.hold_ms && !a.gesture_port);
    in[0].buttons=BTN_START;
    tick(&a,in,11);
    assert(a.active_port==2 && a.hold_ms==1100);
    tick(&a,in,1);
    assert(a.active_port==0 && a.testing && !a.hold_ms);
    assert(a.ports[0].min_x==-128 && a.ports[2].min_x==10);
    tick(&a,in,20);
    assert(a.testing && !a.hold_ms); /* Switching hold cannot exit. */
    in[0].buttons=0;
    tick(&a,in,1);
    /* Disconnecting the active controller does not strand the screen. */
    in[0].connected=false;
    in[3]=(Input){true,0,45,-45};
    tick(&a,in,1);
    assert(!a.ports[0].count && !a.ports[0].buttons && a.active_port==0);
    in[3].buttons=BTN_START;
    tick(&a,in,6);
    in[3].connected=false;
    tick(&a,in,1);
    assert(!a.gesture_port && !a.hold_ms && a.active_port==0);
    in[3]=(Input){true,BTN_START,4,5};
    tick(&a,in,20);
    assert(a.active_port==0); /* Reconnected held Start needs a release. */
    in[3].buttons=0;
    tick(&a,in,1);
    in[3].buttons=BTN_START;
    tick(&a,in,12);
    assert(a.active_port==3 && a.ports[3].min_x==4 && a.ports[3].max_y==5);
    /* Simultaneous non-active holds choose the lowest numbered port. */
    in[3].buttons=0; in[0]=(Input){true,0,0,0};
    in[1]=(Input){true,0,0,0};
    tick(&a,in,1);
    in[0].buttons=in[1].buttons=BTN_START;
    tick(&a,in,12);
    assert(a.active_port==0 && a.testing);
    tick(&a,in,20);
    assert(a.active_port==0 && !a.hold_ms); /* No ping-pong from held buttons. */
    in[0].buttons=in[1].buttons=0;
    tick(&a,in,1);
    /* Active-port exit wins a simultaneous hold and does not reopen. */
    in[0].buttons=in[1].buttons=BTN_START;
    tick(&a,in,12);
    assert(!a.testing);
    tick(&a,in,20);
    assert(!a.testing);
    in[1].buttons=0;
    tick(&a,in,1);
    in[1].buttons=BTN_A;
    tick(&a,in,1);
    assert(a.testing && a.active_port==1 && a.calibration_notice);
    in[1].buttons=0; tick(&a,in,1);
    in[2].buttons=BTN_Z; tick(&a,in,1);
    assert(!a.calibration_notice && a.active_port==1);
    assert(a.ports[0].min_x==0 && a.ports[0].max_x==0);
    /* Controller tester switches on the first frame of a fresh Start press. */
    App b={.testing=true,.tool=TOOL_CONTROLLER};
    Input pads[PORT_COUNT]={{true,0,10,20},{true,0,-30,40},{true,0,0,0}};
    tick(&b,pads,1);
    pads[1].buttons=BTN_START;
    tick(&b,pads,1);
    assert(b.active_port==1 && b.testing && !b.hold_ms && !b.gesture_port);
    assert(b.ports[0].min_x==10 && b.ports[1].min_x==-30);
    tick(&b,pads,20);
    assert(b.testing && !b.hold_ms); /* Switching press cannot become exit. */
    pads[1].buttons=0; tick(&b,pads,1);
    pads[0].buttons=pads[2].buttons=BTN_START;
    tick(&b,pads,1);
    assert(b.active_port==0); /* Lowest non-active port wins a tie. */
    tick(&b,pads,20); assert(b.active_port==0 && b.testing);
    pads[0].buttons=pads[2].buttons=0; tick(&b,pads,1);
    pads[0].connected=false;
    pads[2].connected=false; tick(&b,pads,1);
    pads[2]=(Input){true,BTN_START,0,0}; tick(&b,pads,20);
    assert(b.active_port==0); /* Reconnection with Start held needs release. */
    pads[2].buttons=0; tick(&b,pads,1);
    pads[2].buttons=BTN_START; tick(&b,pads,1);
    assert(b.active_port==2 && b.testing);
    pads[2].buttons=0; tick(&b,pads,1);
    pads[2].buttons=BTN_START; tick(&b,pads,11);
    assert(b.testing && b.hold_ms==1100);
    tick(&b,pads,1); assert(!b.testing);
    App nav={0}; Input sticks[PORT_COUNT]={{true,0,0,0},{true,0,0,0}};
    app_update(&nav,sticks,16);
    sticks[0].y=20; app_update(&nav,sticks,16);
    assert(nav.menu_index==0);
    sticks[0].y=-60; app_update(&nav,sticks,16);
    assert(nav.menu_index==1 && !nav.ports[0].buttons);
    app_update(&nav,sticks,399); assert(nav.menu_index==1);
    app_update(&nav,sticks,1); assert(nav.menu_index==2);
    app_update(&nav,sticks,120); assert(nav.menu_index==3);
    sticks[0].y=-35; app_update(&nav,sticks,16);
    sticks[0].y=-60; app_update(&nav,sticks,16);
    assert(nav.menu_index==3); /* Threshold jitter cannot cause a fresh step. */
    sticks[0].y=60; app_update(&nav,sticks,16); assert(nav.menu_index==2);
    sticks[0].buttons=BTN_A; app_update(&nav,sticks,16);
    assert(nav.tool==TOOL_AUDIO && nav.tone_index==1);
    sticks[0].buttons=0; app_update(&nav,sticks,500);
    assert(nav.tone_index==1); /* Held menu direction does not leak into tool. */
    sticks[0].y=0; app_update(&nav,sticks,16);
    sticks[0].x=60; app_update(&nav,sticks,16); assert(nav.audio_channel==2);
    sticks[0].x=0; app_update(&nav,sticks,16);
    sticks[0].y=60; app_update(&nav,sticks,16); assert(nav.tone_index==2);
    nav.tool=TOOL_PAK; nav.pak.status=PAK_READY; nav.pak.count=3;
    sticks[0].y=0; app_update(&nav,sticks,16);
    sticks[1].y=-60; app_update(&nav,sticks,16); assert(nav.pak.selected==0);
    sticks[0].y=-60; app_update(&nav,sticks,16); assert(nav.pak.selected==1);
    app_update(&nav,sticks,400); assert(nav.pak.selected==2);
    app_update(&nav,sticks,120); assert(nav.pak.selected==2);
    nav.pak_write.phase=WRITE_CONFIRM;
    sticks[0].y=60; app_update(&nav,sticks,500);
    assert(nav.pak.selected==2 && !nav.pak_write.hold_ms);
    puts("four-controller app and joystick navigation tests passed");
}
