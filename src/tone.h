#ifndef TONE_H
#define TONE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { float phase, left, right; } Tone;
/* Stereo interleaved signed PCM; channels: 0 left, 1 both, 2 right. */
void tone_render(Tone *tone,int16_t *samples,size_t frames,unsigned sample_rate,
                 unsigned frequency,unsigned channel,bool playing);
#endif
