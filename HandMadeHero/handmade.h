#pragma once
#include<stdint.h>

float const Pi = 3.14159265359;

struct game_Offscreen_Buffer {
	void *Memory;
	int Width;
	int Height;
	static const int BytesPerPixel = 4;
	int Pitch;
};


struct game_Sound_Output_Buffer {
	int SamplesPerSecond;
	int SampleCount;
	int16_t *Samples;

};
//FOUR THINGS --> timing, controller/keyboard input, bitmap buffer, to use, sound buffer to user
void GameUpdateAndRender(game_Offscreen_Buffer *Buffer, game_Sound_Output_Buffer *SoundBuffer);

