#include<Windows.h>
#include<stdint.h>
#include<Xinput.h>

#define global_variable static;
#define local_persist static;

global_variable bool Running;

struct Offscreen_Buffer {
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	static const int BytesPerPixel = 4;
	int Pitch;
};

static Offscreen_Buffer BackBuffer;

struct WindowDimension {
	int Width;
	int Height;
};

//GetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)

typedef X_INPUT_GET_STATE (x_input_get_state);

X_INPUT_GET_STATE (XInputGetStateStub) {
	return 0;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;

#define XInputGetState XInputGetState_


//SetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE (x_input_set_state);

X_INPUT_SET_STATE (XInputSetStateStub) {
	return 0;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputSetState XInputSetState_



void Win32LoadXInput (void) {
	HMODULE XInputLibrary = LoadLibraryA ("xinput1_3.dll");
	if (XInputLibrary) {
		XInputGetState = (x_input_get_state *)GetProcAddress (XInputLibrary, "XInputGetState");

	}
}

WindowDimension GetWinDimension (HWND Window) {
	WindowDimension Result;
	RECT ClientRect;
	GetClientRect (Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

static void RenderGradient (Offscreen_Buffer *Buffer, int xOffset, int yOffset) {
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
			uint8_t Red = (x + y + xOffset+yOffset) % 255;;
			
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

static void WinResizeDIBSection (Offscreen_Buffer *Buffer, int Width, int Height) {


	//Free the Memory if already allocated
	if (Buffer->Memory)
		VirtualFree (Buffer->Memory, 0, MEM_RELEASE);


	Buffer->Height = Height;
	Buffer->Width = Width;

	Buffer->Info.bmiHeader.biSize = sizeof (Buffer->Info.bmiHeader);

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

	Buffer->Memory = VirtualAlloc (0, BitMapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	//RenderGradient (Buffer, 100, 100);

}

void UpdateWin (Offscreen_Buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight, int X, int Y/*, int Width, int Height*/) {

	StretchDIBits (DeviceContext,

		//Bitmap gets stretched according to it
		0, 0, WindowWidth, WindowHeight,

		//BufferWidth remains same
		0, 0, Buffer->Width, Buffer->Height,

		Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

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


		WindowDimension Dimension = GetWinDimension (Window);
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
	Running = true;

	int xOffset = 0;
	int yOffset = 0;

	HDC DeviceContext = GetDC (Window);
	while (Running) {

		MSG Message;

		while (PeekMessage (&Message, 0, 0, 0, PM_REMOVE)) {
			if (Message.message == WM_QUIT) {
				Running = false;
			}
			TranslateMessage (&Message);
			DispatchMessage (&Message);

		}

		for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex) {
			XINPUT_STATE ControllerState;
			if ( XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {

				//TODO: See if COntrollerState.dwPacketNumber is increments too rapidly
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

		RenderGradient (&BackBuffer, xOffset, yOffset);
		

		WindowDimension Dimension = GetWinDimension (Window);

		UpdateWin (&BackBuffer, DeviceContext, Dimension.Width, Dimension.Height, 0, 0/*,Dimension.Width,Dimension.Height*/);
		//ReleaseDC (Window, DeviceContext);
		xOffset++;
		yOffset++;

	}
	return 0;
}