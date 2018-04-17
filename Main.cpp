#include<Windows.h>

#define global_variable static;
#define local_persist static;

global_variable bool Running;
global_variable BITMAPINFO BitMapInfo;
global_variable void *BitMapMemory;
global_variable HBITMAP BitMapHandle;
global_variable HDC BitMapDeviceContext;


static void WinResizeDIBSection (int Width, int Height) {


	if (BitMapHandle) {
		DeleteObject (BitMapHandle);
	}
	if(!BitMapDeviceContext) {	
		BitMapDeviceContext = CreateCompatibleDC (0);
	}
	BitMapInfo.bmiHeader.biSize = sizeof (BitMapInfo.bmiHeader);
	BitMapInfo.bmiHeader.biWidth = Width;
	BitMapInfo.bmiHeader.biHeight = -Height;
	BitMapInfo.bmiHeader.biPlanes = 1;
	BitMapInfo.bmiHeader.biBitCount = 32;
	BitMapInfo.bmiHeader.biCompression = BI_RGB;

	BitMapHandle = CreateDIBSection (BitMapDeviceContext, &BitMapInfo, DIB_RGB_COLORS, &BitMapMemory, 0, 0);
	
}

void UpdateWin (int X, int Y, int Width, int Height) {

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
		
		//UpdateWin ();

		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - X;
		int Height = Paint.rcPaint.bottom - Y;

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
	while (Running) {
		MSG Message;
		bool MessageResult = GetMessage (&Message, 0, 0, 0);
		if (MessageResult > 0) {
			TranslateMessage (&Message);
			DispatchMessage (&Message);
			
		}
		else {
			break;
		}
	}
	return 0;
}