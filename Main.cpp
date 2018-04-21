#include<Windows.h>
#include<stdint.h>
#define global_variable static;
#define local_persist static;

global_variable bool Running;

global_variable BITMAPINFO BitMapInfo;
global_variable void *BitMapMemory;
global_variable int BitMapWidth;
global_variable int BitMapHeight;
//global_variable HBITMAP BitMapHandle;
//global_variable HDC BitMapDeviceContext;


static void RenderGradient (int xOffset, int yOffset) {
	int Pitch = BitMapWidth * 4;

	uint8_t *Row = (uint8_t *)BitMapMemory;

	for (int y = 0; y < BitMapHeight; ++y) {

		uint32_t *Pixel = (uint32_t *)Row;

		for (int x = 0; x < BitMapWidth; ++x) {
			/*
				Memory
				BB GG RR xx
			
				When loaded up in memory

				Register
				xx RR GG BB
			*/
			uint8_t Blue = x + xOffset;
			uint8_t Green = y + yOffset;
			uint8_t Red = 0;

			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += Pitch;
	}
}

static void WinResizeDIBSection (int Width, int Height) {

	if (BitMapMemory)
		VirtualFree (BitMapMemory, 0, MEM_RELEASE);

	BitMapHeight = Height;
	BitMapWidth = Width;
	
	BitMapInfo.bmiHeader.biSize = sizeof (BitMapInfo.bmiHeader);
	BitMapInfo.bmiHeader.biWidth = BitMapWidth;
	BitMapInfo.bmiHeader.biHeight = -BitMapHeight;
	BitMapInfo.bmiHeader.biPlanes = 1;
	BitMapInfo.bmiHeader.biBitCount = 32;
	BitMapInfo.bmiHeader.biCompression = BI_RGB;


	int BytesPerPixel = 4;
	int BitMapMemorySize = (BitMapWidth* BitMapHeight)*BytesPerPixel;

	BitMapMemory = VirtualAlloc (0, BitMapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	RenderGradient (100,100);
	
}

void UpdateWin (HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height) {
	int WindowWidth = ClientRect->right - ClientRect->left;
	int WindowHeight = ClientRect->bottom - ClientRect->top;

	StretchDIBits (DeviceContext,
		0, 0, BitMapWidth, BitMapHeight,
		0, 0, WindowWidth, WindowHeight,
		BitMapMemory, &BitMapInfo, DIB_RGB_COLORS, SRCCOPY);
}	

LRESULT CALLBACK MainProc (HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) {
	LRESULT Result = 0;

	switch (Message) {

	case WM_SIZE: {
		RECT ClientRect;
		GetClientRect (Window, &ClientRect);
		int Width = ClientRect.right - ClientRect.left;
		int Height = ClientRect.bottom - ClientRect.top;
		WinResizeDIBSection (Width, Height);
	}break;


	case WM_PAINT: {
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint (Window, &Paint);

		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - X;
		int Height = Paint.rcPaint.bottom - Y;
	
		RECT ClientRect;
		GetClientRect (Window, &ClientRect);

		UpdateWin (DeviceContext, &ClientRect, X, Y, Width, Height);
		
		EndPaint (Window, &Paint);

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


int CALLBACK WinMain ( HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow ) {
	WNDCLASS WindowClass = {};

	WindowClass.lpfnWndProc = MainProc;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = TEXT("HandMadeHero");

	RegisterClass (&WindowClass);

	HWND WindowHandle = CreateWindowEx (0, WindowClass.lpszClassName, TEXT ("HandMadeHero"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
	Running = true;
	int xOffset = 0;
	int yOffset = 0;
	while (Running) {
		MSG Message;

		while (PeekMessage (&Message, 0, 0, 0, PM_REMOVE) ){
			if (Message.message == WM_QUIT) {
				Running = false;
			}
			TranslateMessage (&Message);
			DispatchMessage (&Message);
			



		}
		RenderGradient (xOffset, yOffset);
		HDC DeviceContext = GetDC (WindowHandle);
		RECT ClientRect;

	
		GetClientRect (WindowHandle, &ClientRect);

		int WindowWidth = ClientRect.right - ClientRect.left;
		int WindowHeight = ClientRect.bottom - ClientRect.top;

		UpdateWin (DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
		ReleaseDC (WindowHandle, DeviceContext);
		xOffset++;
		
	}
	return 0;
}