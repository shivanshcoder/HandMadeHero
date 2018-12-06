
#include<stdint.h>
#include<math.h>
#include<malloc.h>


#define global_variable static
#define local_persist static

global_variable bool Running;

#include"handmade.cpp"

#include<windows.h>
#include<Xinput.h>
#include<dsound.h>
#include<stdio.h>

#pragma region __SCREENBUFFER

struct win32_Window_Dimension {
	int Width;
	int Height;
};

struct win32_Offscreen_Buffer {
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	static const int BytesPerPixel = 4;
	int Pitch;
};


static win32_Offscreen_Buffer BackBuffer;

win32_Window_Dimension win32_GetWinDimension(HWND Window) {
	win32_Window_Dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

static void win32_RenderGradient(win32_Offscreen_Buffer *Buffer, int xOffset, int yOffset) {
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
			uint8_t Blue = x + xOffset;
			uint8_t Green = y + yOffset;
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


/*
	Resizes and Initilizes the Information for our Back Bitmap Buffer
*/
static void win32_WinResizeDIBSection(win32_Offscreen_Buffer *Buffer, int Width, int Height) {


	//Free the Memory if already allocated
	if (Buffer->Memory)
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);


	Buffer->Height = Height;
	Buffer->Width = Width;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);

	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	//Negative because we want bitmap to be Top down
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;

	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	//Number of Pixels we need 
	//Bytes per pixel Represent
	// BB GG RR xx 4 bytes
	int BitMapMemorySize = (Buffer->Width* Buffer->Height)*Buffer->BytesPerPixel;

	Buffer->Memory = VirtualAlloc(0, BitMapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	//RenderGradient (Buffer, 100, 100);

}

void win32_UpdateWin(win32_Offscreen_Buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight, int X, int Y/*, int Width, int Height*/) {

	StretchDIBits(DeviceContext,

		//Bitmap gets stretched according to it
		0, 0, WindowWidth, WindowHeight,

		//BufferWidth remains same
		0, 0, Buffer->Width, Buffer->Height,

		Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}


#pragma endregion

#pragma region __XINPUT

//GetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)

typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub) {
	return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;

#define XInputGetState XInputGetState_


//SetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub) {
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputSetState XInputSetState_

void Win32LoadXInput(void) {

	//TODO Check which is needed for Windows 10 or Windows 7!
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");

	if (!XInputLibrary)
		XInputLibrary = LoadLibraryA("xinput1_4.dll");

	if (XInputLibrary) {
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState) { XInputGetState = XInputGetStateStub; }

		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState) { XInputSetState = XInputSetStateStub; }

	}
	else {
		//TODO Diagnostic
	}
}

#pragma endregion

#pragma region __DIRECTSOUND

global_variable LPDIRECTSOUNDBUFFER SecondaryBuffer;

struct win32_Sound_Output {
	int SamplesPerSecond;
	uint32_t RunningSampleIndex;
	//int ToneHz;
	//int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	//int ToneVolume;
	int LatencySampleCount;
	float tSine;
}; 

void win32_InitSound_Output(win32_Sound_Output *SoundOutput) {
	SoundOutput->SamplesPerSecond = 48000;
	SoundOutput->RunningSampleIndex = 0;

	//Near Note MIDDLE C LOl
	//SoundOutput->ToneHz = 256;
	//SoundOutput->SquareWavePeriod = SoundOutput->SamplesPerSecond / SoundOutput->ToneHz;

	SoundOutput->BytesPerSample = sizeof(int16_t) * 2;
	SoundOutput->SecondaryBufferSize = SoundOutput->SamplesPerSecond * SoundOutput->BytesPerSample;
	//SoundOutput->ToneVolume = 10000;
	//SoundOutput->WavePeriod = SoundOutput->SamplesPerSecond / SoundOutput->ToneHz;
	SoundOutput->tSine = 0;
	//NOTE Need to confirm data type
	SoundOutput->LatencySampleCount = SoundOutput->SamplesPerSecond / 60;

}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter)

typedef DIRECT_SOUND_CREATE(direct_sound_create);


static void InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize) {

	// Load the library

	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if (DSoundLibrary) {

		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		// Get a DirectSound object

		//TODO Check if it works properly on windows 7 or Windows 10
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {


			//Don't Jumble up the Initializing lines in API
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = (WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign);
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {

				// Create a primary buffer

				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;

				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {

					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
						OutputDebugStringA("Primary Buffer Created succesfully");
						//NOTE Format Set!!!
					}
					else {
						//NOTE Diagnostic
					}

				}
				else {
					//NOTE Diagnostic
				}

			}
			else {
				//NOTE Diagnostic
			}

			// Create secondary buffer

			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			//LPDIRECTSOUNDBUFFER SecondaryBuffer;

			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0))) {
				OutputDebugStringA("Secondary Buffer Created succesfully");

			}
			else {
				//NOTE Diagnostic
			}
		}
		else {
			//NOTE Diagnostic
		}

	}

	// Start playing it
}

static void win32_ClearSoundBuffer(win32_Sound_Output *SoundOutput) {

	void *Region1;
	DWORD Region1Size;
	void *Region2;
	DWORD Region2Size;


	/*
	
	*/
	if (SUCCEEDED(SecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize , &Region1, &Region1Size, &Region2, &Region2Size, 0))) {
		//TODO assert that RegionSizes are valid
		uint8_t *DestSample = (uint8_t *)Region1;

		for (DWORD Index = 0; Index < Region1Size; ++Index) {

			*DestSample++ = 0;
		}

		DestSample = (uint8_t *)Region2;

		for (DWORD Index = 0; Index < Region2Size; ++Index) {

			*DestSample++ = 0;
		}
		SecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}

}

static void win32_FillSoundBuffer(win32_Sound_Output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
	game_Sound_Output_Buffer *SourceBuffer) {
	 
	void *Region1;
	DWORD Region1Size;
	void *Region2;
	DWORD Region2Size;


	if (SUCCEEDED(SecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0))) {
		//TODO assert that RegionSizes are valid

		int16_t *SourceSample = SourceBuffer->Samples;

		/*         Region 1                 */

		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		
		int16_t *DestSample = (int16_t *)Region1;

		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {

			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			SoundOutput->RunningSampleIndex++;
		}

		/*            Region 2              */
		DestSample = (int16_t *)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;

		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {

			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			SoundOutput->RunningSampleIndex++;
		}

		SecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}

}

#pragma endregion


LRESULT CALLBACK MainProc (HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) {
	LRESULT Result = 0;

	switch (Message) {

	case WM_PAINT: {
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint (Window, &Paint);

		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - X;
		int Height = Paint.rcPaint.bottom - Y;


		win32_Window_Dimension Dimension = win32_GetWinDimension (Window);
		win32_UpdateWin (&BackBuffer, DeviceContext, Dimension.Width, Dimension.Height, X, Y/*, Width, Height*/);

		EndPaint (Window, &Paint);

	}break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP: {
		uint32_t VKCode = wParam;
		bool WasDown = ((lParam & (1 << 30)) != 0);
		bool IsDown = ((lParam & (1 << 31)) == 0);
		if (IsDown != WasDown) {
			switch (VKCode) {
			case 'W': {

			}break;

			case 'A': {
			}break;

			case 'S': {

			}break;

			case 'D': {

			}break;

			case 'E': {

			}break;

			case 'Q': {

			}break;

			case VK_UP: {

			}break;

			case VK_DOWN: {

			}break;

			case VK_LEFT: {

			}break;

			case VK_RIGHT: {

			}break;

			case VK_ESCAPE: {
				if (IsDown)
					OutputDebugStringA ("Escape is down\n");
				if (WasDown)
					OutputDebugStringA ("Escape was down\n");
			}break;

			case VK_SPACE: {

			}break;

			}
		}

		bool AltKeyWasDown = ((lParam & (1 << 29)) != 0);
		if (VKCode == VK_F4 && AltKeyWasDown)
			Running = false;
	}break;
	case WM_DESTROY: {
		PostQuitMessage (0);
		return 0;
	}break;


	default: {
		return DefWindowProc (Window, Message, wParam, lParam);
	}
	}
	return Result;
}


int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow) {

	WNDCLASSA WindowClass = {};

	win32_WinResizeDIBSection(&BackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;;
	WindowClass.lpfnWndProc = MainProc;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandMadeHero";

	RegisterClassA(&WindowClass);

	HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "HandMadeHero", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

	LARGE_INTEGER PerfCountFrequencyRes;
	QueryPerformanceFrequency(&PerfCountFrequencyRes);
	uint64_t PerfCountFrequency = PerfCountFrequencyRes.QuadPart;

	int64_t LastCycleCount = __rdtsc();

	if (Window) {
		//For Quitting
		Running = true;

		//For Graphics
		int xOffset = 0;
		int yOffset = 0;


		/*         For Sound              */
		win32_Sound_Output SoundOutput;
		win32_InitSound_Output(&SoundOutput);

		//Initialize our Secondary Buffer 
		InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);

		int16_t *Samples = (int16_t*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		//
		win32_ClearSoundBuffer(&SoundOutput);
		//win32_FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);

		//SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
		//bool SoundIsPlaying = false;
		/*         For Sound              */


		bool SoundPlay = false;

		HDC DeviceContext = GetDC(Window);

		LARGE_INTEGER LastCounter;
		QueryPerformanceCounter(&LastCounter);

		while (Running) {


			MSG Message;

			while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
				if (Message.message == WM_QUIT) {
					Running = false;
				}
				TranslateMessage(&Message);
				DispatchMessage(&Message);

			}

			/*        Remote Controller      */
			for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex) {
				XINPUT_STATE ControllerState;
				if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {

					//TODO: See if ControllerState.dwPacketNumber increments too rapidly
					XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

					bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
					bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
					bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
					bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

					bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
					bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);

					bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
					bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

					bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
					bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
					bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
					bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

					int16_t StickX = Pad->sThumbLX;
					int16_t StickY = Pad->sThumbLY;
				}
				else {


				}
			}
			/*        Remote Controller      */

			DWORD BytesToWrite = 0;

			//The starting position of the lock
			DWORD BytesToLock;

			DWORD PlayCursor;
			DWORD WriteCursor;

			bool SoundIsValid = false;


			if (SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
				SoundIsValid = true;

				//Number of bytes we can lock/fill
				BytesToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);

				//Number of bytes we wish to fill
				DWORD TargetCursor;
				TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);

				//The Case when we have to do two chunks i.e. chunk after BytesToLock and 
				//the chunk before PlayCursor
				if (BytesToLock > TargetCursor) {
					BytesToWrite = (SoundOutput.SecondaryBufferSize - BytesToLock);
					BytesToWrite += TargetCursor;
				}
				//When only one chunk is to be filled	
				else {
					BytesToWrite = TargetCursor - BytesToLock;
				}
				//SoundIsValid = true;
			}

			
			game_Sound_Output_Buffer SoundBuffer = {};
			SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
			SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
			SoundBuffer.Samples = Samples;


			game_Offscreen_Buffer buffer = {};
			buffer.Memory = BackBuffer.Memory;
			buffer.Width = BackBuffer.Width;
			buffer.Height = BackBuffer.Height;
			buffer.Pitch = BackBuffer.Pitch;

			GameUpdateAndRender(&buffer, &SoundBuffer);




			/*         For Sound           */

			//if ( SoundIsValid ) {

			win32_FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);

			//}
			if (!SoundPlay) {
				SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
				SoundPlay = true;
			}

			/*         For Sound           */

			win32_Window_Dimension Dimension = win32_GetWinDimension(Window);

			win32_UpdateWin(&BackBuffer, DeviceContext, Dimension.Width, Dimension.Height, 0, 0/*,Dimension.Width,Dimension.Height*/);
			//ReleaseDC (Window, DeviceContext);
			xOffset++;
			yOffset++;

			uint64_t EndCycleCount = __rdtsc();


			LARGE_INTEGER EndCounter;
			QueryPerformanceCounter(&EndCounter);

			uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;

			int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
			float MiliSecsPerFrame = ((1000 * CounterElapsed) / PerfCountFrequency);
			float fps = PerfCountFrequency / CounterElapsed;
			/*
			char buffer[256];
			sprintf(buffer, "Millisecods per Frame : %f\n", MiliSecsPerFrame);
			OutputDebugStringA(buffer);
			sprintf(buffer, "FPS : %f\n", fps);
			OutputDebugStringA(buffer);

			//Mega cycles per frame
			sprintf(buffer, "Cycles per frame : %f\n", (float)CyclesElapsed/(1000*1000));
			OutputDebugStringA(buffer);
			*/
			LastCounter = EndCounter;
			LastCycleCount = EndCycleCount;
		}
	}
	else {
		//NOTE Diagnostics
	}
	return 0;
}