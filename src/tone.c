#include "tone.h"
#include <math.h>
static float approach(float value,float target,float step) {
    if(value<target) return fminf(value+step,target);
    return fmaxf(value-step,target);
}
void tone_render(Tone *t,int16_t *samples,size_t frames,unsigned rate,
                 unsigned frequency,unsigned channel,bool playing) {
    /* Five-millisecond fades prevent clicks on stop/start and channel changes. */
    float step=1.0f/(rate*.005f);
    float left=playing && channel!=2 ? 1 : 0;
    float right=playing && channel!=0 ? 1 : 0;
    for(size_t i=0;i<frames;i++) {
        t->left=approach(t->left,left,step);
        t->right=approach(t->right,right,step);
        float value=sinf(t->phase*6.283185307f)*6553.0f; /* 20% full-scale peak. */
        samples[i*2]=(int16_t)(value*t->left);
        samples[i*2+1]=(int16_t)(value*t->right);
        t->phase+=(float)frequency/rate;
        if(t->phase>=1) t->phase-=floorf(t->phase);
    }
}
