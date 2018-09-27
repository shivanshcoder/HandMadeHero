#include<Windows.h>
#include<stdint.h>
#include<Xinput.h>
#include<dsound.h>
#include<math.h>

float const Pi = 3.14159265359;

#define global_variable static
#define local_persist static

global_variable bool Running;

#pragma region __SCREENBUFFER

struct Window_Dimension {
	int Width;
	int Height;
};

struct Offscreen_Buffer {
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	static const int BytesPerPixel = 4;
	int Pitch;
};


static Offscreen_Buffer BackBuffer;

Window_Dimension GetWinDimension(HWND Window) {
	Window_Dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

static void RenderGradient(Offscreen_Buffer *Buffer, int xOffset, int yOffset) {
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
			uint8_t Red = 255;//(x + y + xOffset+yOffset) % 255;

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
static void WinResizeDIBSection(Offscreen_Buffer *Buffer, int Width, int Height) {


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

void UpdateWin(Offscreen_Buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight, int X, int Y/*, int Width, int Height*/) {

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

struct Sound_Output {
	int SamplesPerSecond;
	uint32_t RunningSampleIndex;
	int ToneHz;
	int SquareWavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	int ToneVolume;
};

void InitSound_Output(Sound_Output *SoundOutput) {
	SoundOutput->SamplesPerSecond = 48000;
	SoundOutput->RunningSampleIndex = 0;

	//Near Note MIDDLE C LOl
	SoundOutput->ToneHz = 256;
	SoundOutput->SquareWavePeriod = SoundOutput->SamplesPerSecond / SoundOutput->ToneHz;

	SoundOutput->BytesPerSample = sizeof(int16_t) * 2;
	SoundOutput->SecondaryBufferSize = SoundOutput->SamplesPerSecond * SoundOutput->BytesPerSample;
	SoundOutput->ToneVolume = 6000;

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

static void FillSoundBuffer(Sound_Output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite) {
	void *Region1;
	DWORD Region1Size;
	void *Region2;
	DWORD Region2Size;


	//URGENT Run function
	//SecondaryBuffer->Lock()
	if (SUCCEEDED(SecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0))) {
		//TODO assert that RegionSizes are valid


		/*         Region 1                 */
		int16_t *SampleOut = (int16_t *)Region1;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;

		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {

			float t = 2 * Pi * (float)SoundOutput->RunningSampleIndex / (float)SoundOutput->SquareWavePeriod;
			float SineValue = sin(t);
			int16_t SampleValue = (int16_t)(SineValue*SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = -SampleValue;
			++SoundOutput->RunningSampleIndex;
		}

		/*            Region 2              */
		SampleOut = (int16_t *)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;

		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {

			float t = 2 * Pi * (float)SoundOutput->RunningSampleIndex / (float)SoundOutput->SquareWavePeriod;
			float SineValue = sin(t);
			int16_t SampleValue = (int16_t)(SineValue*SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = -SampleValue;
			++SoundOutput->RunningSampleIndex;
			++SoundOutput->RunningSampleIndex;
		}

		//SecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
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


		Window_Dimension Dimension = GetWinDimension (Window);
		UpdateWin (&BackBuffer, DeviceContext, Dimension.Width, Dimension.Height, X, Y/*, Width, Height*/);

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
					OutputDebugStringA ("Escape is down\n");;
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


int CALLBACK WinMain (HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow) {
	
	WNDCLASSA WindowClass = {};

	WinResizeDIBSection (&BackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;;
	WindowClass.lpfnWndProc = MainProc;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandMadeHero";

	RegisterClassA (&WindowClass);

	HWND Window = CreateWindowExA (0, WindowClass.lpszClassName, "HandMadeHero", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
	
	if (Window) {
		//For Quitting
		Running = true;

		//For Graphics
		int xOffset = 0;
		int yOffset = 0;


		/*         For Sound              */
		Sound_Output SoundOutput;
		InitSound_Output(&SoundOutput);
		
		//Initialize our Secondary Buffer 
		InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
		FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
		SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
		//bool SoundIsPlaying = false;
		/*         For Sound              */


		HDC DeviceContext = GetDC(Window);

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

			RenderGradient(&BackBuffer, xOffset, yOffset);


			/*         For Sound           */
			DWORD PlayCursor;
			DWORD WriteCursor;
			if ( SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
				
				DWORD BytesToWrite;
				DWORD BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
				
				//NOTE More accurate check for ByteToLock == PlayCursor
				if (BytesToLock == PlayCursor) {
					//if (!SoundIsPlaying)
						//BytesToWrite = SoundOutput.SecondaryBufferSize;
					//else
						BytesToWrite = 0;
				}

				//The Case when we have to do two chunks i.e. chunk after BytesToLock and 
				//the chunk before PlayCursor
				else if (BytesToLock > PlayCursor) {
					BytesToWrite = (SoundOutput.SecondaryBufferSize - BytesToLock);
					BytesToWrite += PlayCursor;
				}
				//When only one chunk is to be filled
				else {
					BytesToWrite = PlayCursor - BytesToLock;
				}
				FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite);
				
			}

			/*         For Sound           */


			Window_Dimension Dimension = GetWinDimension(Window);

			UpdateWin(&BackBuffer, DeviceContext, Dimension.Width, Dimension.Height, 0, 0/*,Dimension.Width,Dimension.Height*/);
			//ReleaseDC (Window, DeviceContext);
			xOffset++;
			yOffset++;

		}
	}
	else {
		//NOTE Diagnostics
	}
	return 0;
}