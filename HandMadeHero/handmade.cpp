

#include"handmade.h"
#include<math.h>

//static float const Pi = 3.14159265359;

static void RenderWeirdGradient(game_Offscreen_Buffer *Buffer, int BlueOffset, int GreenOffset) {
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
	uint8_t *Row = (uint8_t *)Buffer->Memory;

	for (int y = 0; y < Buffer->Height; ++y) {

		/*
		incase you want 64bit filling
		uint64_t *Pixel = (uint64_t *)Row;
		*/
		uint32_t *Pixel = (uint32_t *)Row;

		for (int x = 0; x < Buffer->Width; x++) {
			/*
				Memory
				BB GG RR xx

				When loaded up in memory

				Register
				xx RR GG BB
			*/
			uint8_t Blue = x + BlueOffset;
			uint8_t Green = y + GreenOffset;
			uint8_t Red = 0;//(x + y + xOffset+yOffset) % 255;

			/*

			incase you want 64bit filling
			x++;

			uint8_t Blue2 = x + *xOffset;
			uint8_t Green2 = y + *yOffset;
			uint8_t Red2 = 0;


				BB GG RR xx BB GG RR xx
				xx RR GG BB xx RR GG BB


			uint64_t temp = ((Red2 << 16) | (Green2 << 8) | Blue2);
			temp = temp << 32;
			temp = temp | (Red << 16) | (Green << 8) | (Blue);

			*Pixel = temp;

			*/

			*Pixel = (Red << 16) | (Green << 8) | Blue;
			Pixel++;
		}
		Row += Buffer->Pitch;
	}
}

static void GameOutputSound(game_Sound_Output_Buffer *SoundBuffer, int ToneHz) {
	static float tSine = 0;
	int16_t ToneVolume = 3000;
	//int ToneHz = 256;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16_t *SampleOut = SoundBuffer->Samples;
	

	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {

		float SineValue = sinf(tSine);
		int16_t SampleValue = (int16_t)(SineValue * ToneVolume);;
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2 * Pi * 1.0f / (float)WavePeriod;
	}

}

static void GameUpdateAndRender(game_Offscreen_Buffer *Buffer, game_Sound_Output_Buffer *SoundBuffer) {
	int BlueOffset = 0;
	int GreenOffset = 0;
	int ToneHz = 256;

	//NOTE Allow sample offsets for more robust platform options
	GameOutputSound(SoundBuffer,ToneHz);

	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}
