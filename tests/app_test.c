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
    in[2].buttons=0;
    in[0]=(Input){true,0,-128,127};
    tick(&a,in,1);
    in[0].x=127; in[0].y=-128;
    tick(&a,in,300);
    assert(a.ports[0].count==TRAIL_SIZE);
    assert(a.ports[0].min_x==-128 && a.ports[0].max_x==127);
    assert(a.ports[0].min_y==-128 && a.ports[0].max_y==127);
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
    assert(a.testing && a.active_port==1 && a.ports[2].count==1);
    assert(a.ports[0].min_x==0 && a.ports[0].max_x==0);
    puts("four-controller app tests passed");
}
