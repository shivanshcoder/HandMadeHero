#pragma once
#include<stdint.h>


struct game_offscreen_buffer {
	void *Memory;
	int Width;
	int Height;
	static const int BytesPerPixel = 4;
	int Pitch;
};
//FOUR THINGS --> timing, controller/keyboard input, bitmap buffer, to use, sound buffer to user
void GameUpdateAndRender(game_offscreen_buffer *Buffer);

