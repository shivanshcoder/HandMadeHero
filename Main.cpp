#include<Windows.h>

int CALLBACK WinMain ( HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow ) {
	MessageBoxA ( 0, "Hello World", "Hello", MB_OK | MB_ICONEXCLAMATION );
	return 0;
}