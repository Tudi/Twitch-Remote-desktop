#ifndef _TOOLS_H_
#define _TOOLS_H_

void GetDesktopResolution(int& horizontal, int& vertical);
void BarbaricTCharToChar( TCHAR *In, char *Out, int MaxLen );
BOOL CALLBACK EnumWindowsProcSetActive( HWND hwnd, LPARAM lParam );
void GetMouseNormalizedCordsForPixel( int &x, int &y );
HWND IsWindowOurFocusWindow( HWND ActiveWindow, int Loop = 0 );
void MoveMouseAsDemo();

#endif