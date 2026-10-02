#include "app.h"
#include <string.h>

static void reset_history(Controller *p) {
    p->count = p->next = 0;
    p->min_x = p->max_x = p->x;
    p->min_y = p->max_y = p->y;
}

static void reset_gesture(App *a) {
    a->gesture_port = a->hold_ms = 0;
    for (unsigned i=0; i<PORT_COUNT; i++)
        a->ports[i].start_armed = !(a->ports[i].buttons & BTN_START);
}

static void stop_outputs(App *a) {
    a->pulse_ms = 0;
    a->audio_playing = false;
    a->controls_armed = false;
}

unsigned app_tone_hz(const App *a) {
    static const unsigned frequencies[]={220,440,1000};
    return frequencies[a->tone_index];
}

bool app_rumble_active(const App *a,unsigned port) {
    if(!a->testing || a->tool!=TOOL_RUMBLE || port!=a->active_port ||
       !a->ports[port].connected || !a->rumble_supported[port] ||
       !a->controls_armed || (a->ports[port].buttons & BTN_START)) return false;
    return (a->ports[port].buttons & BTN_A) ||
           (a->pulse_ms && ((900-a->pulse_ms)/150)%2==0);
}

void app_update(App *a, const Input inputs[PORT_COUNT], unsigned ms) {
    a->pak_refresh=false;
    PakWrite *job=&a->pak_write;
    uint16_t down[PORT_COUNT] = {0};
    for (unsigned i=0; i<PORT_COUNT; i++) {
        Controller *p = &a->ports[i];
        const Input *in = &inputs[i];
        if (!in->connected) {
            memset(p, 0, sizeof(*p));
            continue;
        }
        bool reconnect = !p->connected;
        down[i] = in->buttons & ~p->buttons;
        p->connected = true;
        p->buttons = in->buttons;
        p->x = in->x; p->y = in->y;
        if (reconnect) reset_history(p);
        if (!(p->buttons & BTN_START)) p->start_armed = true;
    }
    if (!a->testing) {
        /* One navigation action per frame, from the first participating port. */
        for(unsigned i=0;i<PORT_COUNT;i++) {
            if(down[i] & BTN_DU) {
                a->menu_index=(a->menu_index+TOOL_COUNT-1)%TOOL_COUNT;
                break;
            }
            if(down[i] & BTN_DD) {
                a->menu_index=(a->menu_index+1)%TOOL_COUNT;
                break;
            }
        }
        /* Any connected controller can open the test and become active. */
        for (unsigned i=0; i<PORT_COUNT; i++) {
            if (!(down[i] & (BTN_A | BTN_START))) continue;
            a->testing = true;
            a->tool = (Tool)a->menu_index;
            a->tone_index = 1;
            a->audio_channel = 1;
            stop_outputs(a);
            a->active_port = i;
            for (unsigned j=0; j<PORT_COUNT; j++) reset_history(&a->ports[j]);
            reset_gesture(a);
            break;
        }
        if (!a->testing) return;
    }
    Controller *active=&a->ports[a->active_port];
    if(!active->connected) stop_outputs(a);
    uint16_t action_buttons=BTN_A | (a->tool==TOOL_AUDIO ? 0 : BTN_B);
    if(active->connected && !(active->buttons & action_buttons))
        a->controls_armed=true;
    if(a->tool==TOOL_RUMBLE) {
        if(!a->rumble_supported[a->active_port]) {
            a->pulse_ms=0;
            a->controls_armed=false;
        }
        a->pulse_ms = ms>=a->pulse_ms ? 0 : a->pulse_ms-ms;
        if(a->controls_armed && (down[a->active_port]&BTN_B)) a->pulse_ms=900;
        if(active->buttons & BTN_START) {
            a->pulse_ms=0;
            a->controls_armed=false;
        }
    } else if(a->tool==TOOL_AUDIO) {
        unsigned pressed=down[a->active_port];
        if(pressed&BTN_DL) a->audio_channel=(a->audio_channel+2)%3;
        if(pressed&BTN_DR) a->audio_channel=(a->audio_channel+1)%3;
        if(pressed&BTN_DU) a->tone_index=(a->tone_index+1)%3;
        if(pressed&BTN_DD) a->tone_index=(a->tone_index+2)%3;
        if(a->controls_armed && (pressed&BTN_A)) a->audio_playing=!a->audio_playing;
        if(active->buttons & BTN_START) a->audio_playing=false;
    }
    if(a->tool==TOOL_PAK) {
        unsigned pressed=down[a->active_port];
        if(job->phase!=WRITE_IDLE) {
            if(!active->connected || (pressed&(BTN_B|BTN_START))) {
                *job=(PakWrite){0}; a->pak_refresh=true;
            } else if(job->phase==WRITE_CONFIRM) {
                if(!(active->buttons&BTN_A)) { job->armed=true; job->hold_ms=0; }
                else if(job->armed) {
                    job->hold_ms+=ms>100?100:ms;
                    if(job->hold_ms>=PAK_CONFIRM_MS) job->phase=WRITE_EXECUTE;
                }
            }
        } else {
            if(pressed&BTN_DU) pak_move(&a->pak,-1);
            if(pressed&BTN_DD) pak_move(&a->pak,1);
            if(a->controls_armed && (pressed&BTN_A)) a->pak_refresh=true;
            if(a->controls_armed && (pressed&BTN_B) && a->pak.status==PAK_READY && a->pak.count)
                a->pak.details=!a->pak.details;
            PakAction action=PAK_ACTION_NONE;
            if(pressed&BTN_CU) action=PAK_ACTION_REPAIR;
            if(pressed&BTN_CR) action=PAK_ACTION_FORMAT;
            if((pressed&BTN_CD) && a->pak.status==PAK_READY && a->pak.count &&
               a->pak.notes[a->pak.selected].valid) action=PAK_ACTION_DELETE;
            if(action && a->pak.status!=PAK_READING) {
                *job=(PakWrite){.phase=WRITE_PREPARE,.action=action,.port=a->active_port};
                if(action==PAK_ACTION_DELETE) job->slot=a->pak.notes[a->pak.selected].slot;
            }
        }
    }
    /* Continue sampling each controller while another port is displayed. */
    for (unsigned i=0; i<PORT_COUNT; i++) {
        Controller *p = &a->ports[i];
        if (!p->connected) continue;
        if (p->x < p->min_x) p->min_x = p->x;
        if (p->x > p->max_x) p->max_x = p->x;
        if (p->y < p->min_y) p->min_y = p->y;
        if (p->y > p->max_y) p->max_y = p->y;
        p->trail[p->next] = (Point){p->x,p->y};
        p->next = (p->next + 1) % TRAIL_SIZE;
        if (p->count < TRAIL_SIZE) p->count++;
    }
    if (a->gesture_port) {
        Controller *p = &a->ports[a->gesture_port-1];
        if (!p->connected || !(p->buttons & BTN_START)) {
            a->gesture_port = a->hold_ms = 0;
            return;
        }
    } else {
        /* First hold wins. Simultaneous holds prefer the active port, then
           ascending port number. Keep that choice until release/completion. */
        for (unsigned n=0; n<=PORT_COUNT; n++) {
            unsigned i = n==0 ? a->active_port : n-1;
            Controller *p = &a->ports[i];
            if (p->connected && p->start_armed && (p->buttons & BTN_START)) {
                a->gesture_port = i+1;
                break;
            }
        }
    }
    if (!a->gesture_port) return;
    a->hold_ms += ms > 100 ? 100 : ms;
    if (a->hold_ms < HOLD_MS) return;
    unsigned target = a->gesture_port-1;
    if (target == a->active_port) a->testing = false;
    else a->active_port = target;
    /* All currently held Start buttons must be released before another action. */
    a->pak_write=(PakWrite){0};
    stop_outputs(a);
    reset_gesture(a);
}
