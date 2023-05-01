#include <windows.h>
#include <tchar.h>
#include <time.h>
#include "resource.h"

#define MAX_PIN 6
#define MAX_CHANCE 8
#define WM_USER 0x0400
#define UM_GAME_OVER WM_USER+4

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HelpChildProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PauseChildProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EndChildProc(HWND, UINT, WPARAM, LPARAM);

POINT CenterPoint(RECT&);									// RECT 중심 얻기
void InitGameBoard(RECT);									// 게임화면 rect 초기화
void InitGameState();										// 게임 설정 초기화
void InitAnswerPin();										// answerPin 초기화
void SetFont(int, BOOL init = FALSE);						// font 변수 세팅
void MakeStartRect(POINT, RECT[]);							// 초기화면 버튼 만들기

void DrawGame(HDC);											// 게임화면 그리기
void DrawMain(HDC, RECT, RECT[]);							// 초기화면 그리기
void DrawTitle(HDC, TCHAR[], int, int, int);				// 초기화면 Title 그리기
void DrawStartRect(HDC, RECT[]);							// 초기화면 버튼 그리기
void DrawFocus(HDC, POINT, POINT);							// Focus 그리기
void SetHTriPoint(POINT[], POINT);							// Sett Horizontal TriPoint information
void SetVTriPoint(POINT[], POINT);							// Sett Vertical TriPoint information
void DrawPin(HDC, COLORREF, int, int, int);					// Pin 그리기
void DrawBlankPin(HDC, int, int, int);						// Blank Pin 그리기
void DrawRoundRect(HDC, RECT, int, int, COLORREF);			// Round Rect 그리기
void DrawTextRect(HDC, RECT*, TCHAR*, int, UINT);			// Text 그리기
void DrawTextRect(HDC, RECT*, LPCTSTR, int, UINT);			// DrawTextRect overloading

void SortCheckBoard();										// sort checkBoard
BOOL IsIn(int[], int, int);									// 배열안에 숫자가 있는지 판단
BOOL CheckHitBlow();										// Hit and blow check

HINSTANCE g_hInst;
HWND hWndMain, hHelpChild, hPauseChild, hEndChild;
LPCTSTR lpszClass = _T("Hit and Blow");

// pin 정보
COLORREF pinList[MAX_PIN] = { RGB(255, 89, 94), RGB(255, 146, 76), RGB(255, 202, 58),
							RGB(138, 201, 38), RGB(25, 130, 196), RGB(106, 76, 147) };
COLORREF checkPin[] = { RGB(241, 250, 238), RGB(230, 57, 70) };
enum PINSTATE { INIT, GAME, SELECT, CHECK };		// pin 종류
int pinSize[] = { 70, 30, 50, 15 };					// init, select, pinList, check pin

// 게임판 정보
int gameBoard[MAX_CHANCE][4] = { {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
								 {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1} };
int checkBoard[MAX_CHANCE][4] = { {0, 0, 0, 0 }, { 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
								  {0, 0, 0, 0 }, { 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} };
int answerBoard[4] = { -1, -1, -1, -1 };
RECT initBoard[3];				// main game boards
RECT gameBoardRect[8][4];		// each board (chance, hit/blow, pin)
RECT answerBoardRect;			// answer board
RECT answerKeyRect;
POINT gameBoardPin[8][2][4];	// each board (hit/blow, pin) 
POINT answerPin[4];				// answer pin
POINT selectPin[MAX_PIN];		// select board pin

// 게임 control을 위한 state
LOGFONT font;	// font 정보 저장할 변수
enum GAMESTATE { START, PLAYING, PAUSE, END, HELP } bGameState;		// 프로그램 state
enum PLAYER { ONE, TWO } bPlayer;					// 게임 구분(1 player, 2 player)
enum WIN { WIN, PLAYER1, PLAYER2, LOSS } bWin;
int leftChance;										// 남은 기회의 수
int focus[2];										// 현재 포커스 [0] -> gmaeBoardPin(0-3), [1] -> selectPin(0-5)

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpszCmdParam, _In_ int nCmdShow) {
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	HACCEL hAccel;
	g_hInst = hInstance;
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	// Help Child Window
	WndClass.lpfnWndProc = HelpChildProc;
	WndClass.lpszClassName = _T("HelpChild");
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_SAVEBITS;
	RegisterClass(&WndClass);

	// Pasuse Child Window
	WndClass.lpfnWndProc = PauseChildProc;
	WndClass.lpszClassName = _T("PauseChild");
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_SAVEBITS;
	RegisterClass(&WndClass);

	// End Child
	WndClass.lpfnWndProc = EndChildProc;
	WndClass.lpszClassName = _T("EndChild");
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_SAVEBITS;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		10, 10, 1200, 900,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	hWndMain = hWnd; // hWnd 정보도 전역변수에 저장!

	while (GetMessage(&Message, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &Message)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}
	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;
	HWND hDesktopWnd;		// 바탕화면 핸들 얻기
	RECT deskR;				// 바탕화면 RECT
	POINT p;				

	static HBRUSH hBrush;							// 배경 brush
	static RECT clientR, startRectList[4];			// startRectList: player1, player2, exit, help
	switch (iMessage) {
	case WM_CREATE:
		hWndMain = hWnd;
		hDesktopWnd = GetDesktopWindow();
		// child window 생성
		hHelpChild = CreateWindow(_T("HelpChild"), NULL, WS_CHILD | WS_BORDER, 0, 0, 0, 0,
			hWnd, (HMENU)0, g_hInst, NULL);
		hPauseChild = CreateWindow(_T("PauseChild"), NULL, WS_CHILD | WS_BORDER, 0, 0, 0, 0,
			hWnd, (HMENU)0, g_hInst, NULL);
		hEndChild = CreateWindow(_T("EndChild"), NULL, WS_CHILD | WS_BORDER, 0, 0, 0, 0,
			hWnd, (HMENU)0, g_hInst, NULL);

		// Window setting
		GetClientRect(hWnd, &clientR);
		GetClientRect(hDesktopWnd, &deskR);
		p = CenterPoint(deskR);
		MoveWindow(hWnd, p.x - 600, p.y - 470, 1200, 900, TRUE);	// 현재 컴퓨터 위치에 중심으로 윈도우 옮김
		p = CenterPoint(clientR);
		SetWindowPos(hHelpChild, NULL, p.x - 300, p.y - 350, 600, 700, SWP_NOZORDER);
		SetWindowPos(hPauseChild, NULL, p.x - 200, p.y - 200, 400, 400, SWP_NOZORDER); 
		SetWindowPos(hEndChild, NULL, p.x - 400, p.y - 300, 800, 600, SWP_NOZORDER);

		SetFont(50, TRUE);	// font setting

		// Game 정보 초기화
		bGameState = START;
		hBrush = CreateSolidBrush(RGB(34, 34, 34));
		InitGameBoard(clientR);
		InvalidateRect(hWnd, NULL, TRUE);
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// 기본 paint setting
		FillRect(hdc, &clientR, hBrush);
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);

		// bGameState에 따라 Paint
		switch (bGameState) {
		case HELP: // 초기화면
		case START:
			DrawMain(hdc, clientR, startRectList);
			break;
		case PLAYING: // 게임 실행화면
		case END:
			DrawGame(hdc);
			break;
		}
		EndPaint(hWnd, &ps);
		return 0;
	case WM_LBUTTONDOWN:
		// 초기화면에서 버튼 클릭 CHECK
		if (bGameState == START) {
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);
			for (int i = 0; i < 4; i++) {
				if (PtInRect(&startRectList[i], p))
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_GAME_PLAY1 + i, 0), 0);
			}
		}
		else if(bGameState == PLAYING) {
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);
			if (PtInRect(&answerKeyRect, p))
				SendMessage(hWnd, WM_KEYDOWN, VK_RETURN, 0);
		}
		return 0;
	case WM_KEYDOWN:
		if (bGameState == PLAYING) {
			switch (wParam) {
			case 0x50: // p
				if (bGameState == PLAYING) {
					bGameState = PAUSE;
					ShowWindow(hPauseChild, SW_SHOW);
				}
				else if (bGameState == PAUSE && IsWindowVisible(hPauseChild)) {
					bGameState = PLAYING;
					ShowWindow(hPauseChild, SW_HIDE);
				}
				break;
			case 0x41:	// a
			case VK_LEFT:
				if (focus[1] == 0)
					focus[1] = 5;
				else
					focus[1]--;
				gameBoard[MAX_CHANCE - leftChance][focus[0]] = focus[1];
				InvalidateRect(hWnd, &initBoard[2], TRUE);
				InvalidateRect(hWnd, &gameBoardRect[MAX_CHANCE - leftChance][3], TRUE);
				break;
			case 0x44:	// d
			case VK_RIGHT:
				focus[1] = (focus[1] + 1) % MAX_PIN;
				gameBoard[MAX_CHANCE - leftChance][focus[0]] = focus[1];
				InvalidateRect(hWnd, &initBoard[2], TRUE);
				InvalidateRect(hWnd, &gameBoardRect[MAX_CHANCE - leftChance][3], TRUE);
				break;
			case 0x57:	// w
			case VK_UP:
				if (focus[0] == 0)
					focus[0] = 3;
				else
					focus[0]--;
				gameBoard[MAX_CHANCE - leftChance][focus[0]] = focus[1];
				InvalidateRect(hWnd, &initBoard[2], TRUE);
				InvalidateRect(hWnd, &gameBoardRect[MAX_CHANCE - leftChance][3], TRUE);
				break;
			case 0x53:	// s
			case VK_DOWN:
				focus[0] = (focus[0] + 1) % 4;
				gameBoard[MAX_CHANCE - leftChance][focus[0]] = focus[1];
				InvalidateRect(hWnd, &initBoard[2], TRUE);
				InvalidateRect(hWnd, &gameBoardRect[MAX_CHANCE - leftChance][3], TRUE);
				break;
			case VK_RETURN:
				if (bGameState == PLAYING) {
					if (IsIn(gameBoard[MAX_CHANCE - leftChance], -1, 4))
						MessageBox(hWnd, _T("You dose not select all pin in this chance!!"), _T("Error!"), MB_OK);
					else {
						if (CheckHitBlow())
							SendMessage(hWnd, UM_GAME_OVER, 0, 0);
						else if (leftChance == 0) {
							bWin = LOSS;
							SendMessage(hWnd, UM_GAME_OVER, 0, 0);
						}
						InvalidateRect(hWnd, &initBoard[1], TRUE);
						InvalidateRect(hWnd, &initBoard[2], TRUE);
					}
					break;
				}
				
			}
		}
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_GAME_PLAY1:		// 1 Player
			if (bGameState == START) {
				bGameState = PLAYING;
				bPlayer = ONE;
				InvalidateRect(hWnd, NULL, TRUE);
				InitGameState();
			}
			break;
		case ID_GAME_PLAY2:		// 2 Player
			if (bGameState == START) {
				bGameState = PLAYING;
				bPlayer = TWO;
				InvalidateRect(hWnd, NULL, TRUE);
				InitGameState();
			}
			break;
		case ID_GAME_EXIT:		// Exit
			if (bGameState != HELP && bGameState != PAUSE)
				DestroyWindow(hWnd);
			break;
		case ID_GAME_HELP:
			if (bGameState == START) {
				bGameState = HELP;
				ShowWindow(hHelpChild, SW_SHOW);
			}
			else if (bGameState == HELP) {
				bGameState = START;
				ShowWindow(hHelpChild, SW_HIDE);
			}
		}
		return 0;
	case UM_GAME_OVER: 
		bGameState = END;
		ShowWindow(hEndChild, SW_SHOW);
		return 0;
	case WM_DESTROY:
		DeleteObject(hBrush);
		DestroyWindow(hHelpChild);
		DestroyWindow(hPauseChild);
		DestroyWindow(hEndChild);
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}
LRESULT CALLBACK HelpChildProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;
	HPEN hPen, hOldPen;
	POINT p;
	RECT buffer, clientR;

	static HBRUSH hBrush;
	static RECT textRect, exitRect;
	static TCHAR title[] = _T("Help"), exit[] = _T("Exit(Ctrl+H)"), helpTitle[] = _T("How to Play?");
	static LPCTSTR helpStr = _T("Predict four arbitrarily set pins!\n")
		_T("○ 1 Player: You will be given 8 chances.\n\n")
		_T("○ 2 Player: Each player wiil 4 chances.\n\n")
		_T("○ Hit: Color+Postion Correct!\n\n")
		_T("○ Blow: Only Color Correct!\n\n")
		_T("When you get 4 Hits, you win the game!\n\n")
		_T("If you don't get four hits during the opportunity, you lose the game or there's no winner!\n\n")
		_T("     ★★★ Now start the game! Good Luck! ★★★");

	switch (iMessage) {
	case WM_CREATE:
		hBrush = CreateSolidBrush(RGB(50, 50, 50));
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// 기본 setting
		GetClientRect(hWnd, &clientR);
		FillRect(hdc, &clientR, hBrush);
		p = CenterPoint(clientR);
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);
		SetRect(&exitRect, p.x - 80, p.y + 280, p.x + 80, p.y + 330);
		SetRect(&textRect, clientR.left + 20, clientR.top + 100, clientR.right - 20, clientR.bottom - 70);
		hPen = CreatePen(PS_DASHDOT, 5, RGB(255, 255, 255));
		hOldPen = (HPEN)SelectObject(hdc, hPen);

		// title 그리기
		p.y -= 340;
		DrawTitle(hdc, title, p.x, p.y, 70);

		// message text out
		DrawRoundRect(hdc, textRect, 20, 20, RGB(70, 70, 70));
		SetRect(&textRect, textRect.left + 10, textRect.top + 20, textRect.right - 10, textRect.bottom - 20);
		DrawTextRect(hdc, &textRect, helpTitle, 40, DT_CENTER);
		textRect.top += 50;
		MoveToEx(hdc, textRect.left, textRect.top - 8, NULL);
		LineTo(hdc, textRect.right, textRect.top - 8);
		DrawTextRect(hdc, &textRect, helpStr, 30, DT_LEFT | DT_WORDBREAK);

		// exit button 그리기
		DrawRoundRect(hdc, exitRect, 20, 20, RGB(100, 143, 139));
		buffer = exitRect;
		buffer.top += 10;
		DrawTextRect(hdc, &buffer, exit, 30, DT_CENTER | DT_VCENTER);

		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_LBUTTONDOWN:
		if (bGameState == HELP) {
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);
			if (PtInRect(&exitRect, p)) {
				bGameState = START;
				ShowWindow(hWnd, SW_HIDE);
			}
		}
		return 0;
	case WM_DESTROY:
		DeleteObject(hBrush);
		PostQuitMessage(0);
		return 0;
	}

	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}
LRESULT CALLBACK PauseChildProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;
	HPEN hPen, hOldPen;
	POINT p;
	RECT clientR, buffer;

	static HBRUSH hBrush;
	static RECT buttonRect[3];
	static TCHAR title[] = _T("Pause"), button[][8] = { _T("Resume"), _T("Restart"), _T("Exit") };

	switch (iMessage) {
	case WM_CREATE:
		hBrush = CreateSolidBrush(RGB(50, 50, 50));
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// 기본 setting
		GetClientRect(hWnd, &clientR);
		FillRect(hdc, &clientR, hBrush);
		p = CenterPoint(clientR);
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);
		hPen = CreatePen(PS_DASHDOT, 5, RGB(255, 255, 255));
		hOldPen = (HPEN)SelectObject(hdc, hPen);

		// Title 그리기
		p.y -= 200;
		DrawTitle(hdc, title, p.x, p.y, 70);
		MoveToEx(hdc, p.x - 180, p.y+70, NULL);
		LineTo(hdc, p.x + 180, p.y+70);

		// button 그리기
		p.y += 100;
		for (int i = 0; i < 3; i++) {
			SetRect(&buttonRect[i], p.x - 120, p.y, p.x + 120, p.y + 70);
			DrawRoundRect(hdc, buttonRect[i], 50, 50, RGB(100, 143, 139));
			buffer = buttonRect[i];
			buffer.top += 10;
			DrawTextRect(hdc, &buffer, button[i], 50, DT_CENTER | DT_VCENTER);
			p.y += 100;
		}

		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_LBUTTONDOWN:
		if (bGameState == PAUSE) {
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);
			if (PtInRect(&buttonRect[0], p)) {		// resume
				bGameState = PLAYING;
				ShowWindow(hPauseChild, SW_HIDE);
			}
			else if (PtInRect(&buttonRect[1], p)) {	// restart
				bGameState = START;
				ShowWindow(hPauseChild, SW_HIDE);
				InvalidateRect(hWndMain, NULL, TRUE);
			}
			else if (PtInRect(&buttonRect[2], p)) {	// exit
				bGameState = PLAYING;
				SendMessage(hWndMain, WM_COMMAND, MAKEWPARAM(ID_GAME_EXIT, 0), (LPARAM)hPauseChild);
			}
		}
		
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}
LRESULT CALLBACK EndChildProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;
	HPEN hPen, hOldPen;
	POINT p;
	RECT clientR, buffer;

	static HBRUSH hBrush;
	static RECT buttonRect[2];
	static TCHAR title[] = _T("Game Over"), button[][8] = { _T("Restart"), _T("Exit") },
		winStr[][25] = { _T("You win the game!"), _T("Player1 win the game!"), _T("Player2 win the game!"), _T("You loss the game...")};

	switch (iMessage) {
	case WM_CREATE:
		hBrush = CreateSolidBrush(RGB(50, 50, 50));
		return 0;
	case WM_PAINT:
		Sleep(3000);
		hdc = BeginPaint(hWnd, &ps);
		// 기본 setting
		GetClientRect(hWnd, &clientR);
		FillRect(hdc, &clientR, hBrush);
		p = CenterPoint(clientR);
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);
		hPen = CreatePen(PS_DASHDOT, 5, RGB(255, 255, 255));
		hOldPen = (HPEN)SelectObject(hdc, hPen);

		// Title 그리기
		p.y -= 280;
		DrawTitle(hdc, title, p.x, p.y, 70);
		MoveToEx(hdc, p.x - 350, p.y + 70, NULL);
		LineTo(hdc, p.x + 350, p.y + 70);

		// 게임 승리, 패배 정보 출력
		p.y += 200;
		DrawTitle(hdc, winStr[bWin], p.x, p.y, 95);

		// button 그리기
		p.y += 250;
		p.x -= 200;
		for (int i = 0; i < 2; i++) {
			SetRect(&buttonRect[i], p.x - 140, p.y, p.x + 140, p.y + 70);
			DrawRoundRect(hdc, buttonRect[i], 50, 50, RGB(100, 143, 139));
			buffer = buttonRect[i];
			buffer.top += 10;
			DrawTextRect(hdc, &buffer, button[i], 50, DT_CENTER | DT_VCENTER);
			p.x += 400;
		}
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_LBUTTONDOWN:
		if (bGameState == END) {
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);
			if (PtInRect(&buttonRect[0], p)) {	// restart
				bGameState = START;
				ShowWindow(hWnd, SW_HIDE);
				InvalidateRect(hWndMain, NULL, TRUE);
			}
			else if (PtInRect(&buttonRect[1], p)) {	// exit
				bGameState = PLAYING;
				SendMessage(hWndMain, WM_COMMAND, MAKEWPARAM(ID_GAME_EXIT, 0), (LPARAM)hWnd);
			}
		}
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

POINT CenterPoint(RECT & r) {
	POINT p;
	p.x = (r.left + r.right) / 2;
	p.y = (r.top + r.bottom) / 2;
	return p;
}
void InitGameBoard(RECT clientR) {
	RECT buffer1, buffer2;
	POINT p, q;
	int offset;
	// Set initBoard
	SetRect(&initBoard[0], clientR.left + 50, clientR.top + 70, clientR.right - 50, clientR.bottom - 30);
	SetRect(&initBoard[1], initBoard[0].left + 10, initBoard[0].top + 20, initBoard[0].right - 10, initBoard[0].bottom - 150);
	SetRect(&initBoard[2], initBoard[1].left, initBoard[1].bottom + 20, initBoard[1].right, initBoard[0].bottom - 20);

	// Set GameBoardRect, answerBoardRect Use initBoard[1]
	SetRect(&buffer1, initBoard[1].left + 10, initBoard[1].top + 10, initBoard[1].left + 100, initBoard[1].bottom - 10);
	offset = buffer1.right - buffer1.left + 29;
	for (int i = 0; i < MAX_CHANCE; i++) {
		gameBoardRect[i][0] = buffer1;
		SetRect(&gameBoardRect[i][1], buffer1.left + 10, buffer1.top + 10, buffer1.right - 10, buffer1.top + 50);
		SetRect(&gameBoardRect[i][2], gameBoardRect[i][1].left, gameBoardRect[i][1].bottom + 10, gameBoardRect[i][1].right, gameBoardRect[i][1].bottom + 80);
		buffer2 = gameBoardRect[i][2];
		buffer2.right = buffer2.left + (buffer2.right - buffer2.left) / 2;
		buffer2.bottom = buffer2.top + (buffer2.bottom - buffer2.top) / 2;
		p = CenterPoint(buffer2);
		for (int j = 0; j < 3; j += 2) {
			q = p;
			gameBoardPin[i][0][j] = p;
			q.x += (buffer2.right - buffer2.left);
			gameBoardPin[i][0][j + 1] = q;
			p.y += (buffer2.bottom - buffer2.top);
		}
		SetRect(&gameBoardRect[i][3], gameBoardRect[i][2].left, gameBoardRect[i][2].bottom + 10, gameBoardRect[i][2].right, buffer1.bottom - 10);
		q = CenterPoint(gameBoardRect[i][3]);
		q.y -= 140;
		for (int j = 0; j < 4; j++) {
			gameBoardPin[i][1][j] = q;
			q.y += 90;
		}
		OffsetRect(&buffer1, offset, 0);
	}
	SetRect(&answerBoardRect, buffer1.left, buffer1.top + 40, buffer1.right, buffer1.bottom - 40);

	// Set selectPin Use initBoard[2]
	p = CenterPoint(initBoard[2]);
	p.x -= 370;
	for (int i = 0; i < MAX_PIN; i++) {
		selectPin[i] = p;
		p.x += 135;
	}
	SetRect(&answerKeyRect, p.x - 40, p.y - 20, p.x + 40, p.y + 20);

	// Set answerBoard Use answerBoardRect[0]
	p = CenterPoint(answerBoardRect);
	p.y -= 150;
	for (int i = 0; i < 4; i++) {
		answerPin[i] = p;
		p.y += 100;
	}
}
void InitGameState() {
	// 게임보드 초기화
	for (int i = 0; i < MAX_CHANCE; i++) {
		for (int j = 0; j < 4; j++) {
			gameBoard[i][j] = -1;
			checkBoard[i][j] = 0;
		}
			
	}
	// random pin 생성
	InitAnswerPin();

	// chance 및 포커스 초기화
	leftChance = MAX_CHANCE;
	focus[0] = focus[1] = 0;
}
void InitAnswerPin() {
	int randNum, check[MAX_PIN] = { 0, 0, 0, 0, 0, 0 };
	BOOL bTwice = FALSE;

	srand(time(NULL));
	for (int i = 0; i < 4; i++) {
		randNum = rand() % 6;
		answerBoard[i] = randNum;
		check[randNum]++;

		// 중복 check, 오직 하나의 case에 대해서만 중복 허용
		if (check[randNum] == 2) {
			if (bTwice == TRUE) {
				do {
					answerBoard[i] = rand() % 6;
				} while (randNum == answerBoard[i]);
			}
			else
				bTwice = TRUE;
		}
	}
}
void SetFont(int height, BOOL init) {
	font.lfHeight = height;
	if (init) {
		font.lfWidth = 0;
		font.lfEscapement = 0;
		font.lfOrientation = 0;
		font.lfWeight = FW_NORMAL;
		font.lfItalic = 0;
		font.lfUnderline = 0;
		font.lfStrikeOut = 0;
		font.lfCharSet = ANSI_CHARSET;
		font.lfOutPrecision = 0;
		font.lfClipPrecision = 0;
		font.lfQuality = 0;
		font.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS, _T("MS Reference Sans Serif");
	}
}
void MakeStartRect(POINT mPoint, RECT rect[]) {
	RECT buffer;
	int offset;
	SetRect(&buffer, mPoint.x - 440, mPoint.y + 600, mPoint.x - 200, mPoint.y + 680);
	offset = buffer.right - buffer.left + 80;
	for (int i = 0; i < 3; i++) {
		rect[i] = buffer;
		OffsetRect(&buffer, offset, 0);
	}
}
void DrawGame(HDC hdc) {
	TCHAR chanceNum[][2] = { _T("1"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"), _T("8") };
	TCHAR enter[] = _T("Enter"), playerText[][4] = {_T("P1"), _T("P2")};
	TCHAR helpStr[] = _T("Hit(Red): Color+Position   Blow(White): Color  Move: ▲(w) ▼(s) ◀(a) ▶(d)");
	RECT strR, buffer;

	// Draw Background
	buffer = initBoard[0];
	buffer.top -= 60;
	DrawTextRect(hdc, &buffer, helpStr, 37, DT_CENTER);
	DrawRoundRect(hdc, initBoard[0], 40, 40, RGB(50, 50, 50));
	DrawRoundRect(hdc, initBoard[1], 30, 30, RGB(80, 80, 80));
	DrawRoundRect(hdc, initBoard[2], 30, 30, RGB(80, 80, 80));
	for (int i = 0; i < MAX_CHANCE; i++) {
		DrawRoundRect(hdc, gameBoardRect[i][0], 20, 20, RGB(60, 60, 60));
		DrawTextRect(hdc, &gameBoardRect[i][1], chanceNum[i], 40, DT_CENTER | DT_VCENTER);
		DrawRoundRect(hdc, gameBoardRect[i][2], 20, 20, RGB(45, 45, 45));
		DrawRoundRect(hdc, gameBoardRect[i][3], 20, 20, RGB(45, 45, 45));
		for (int j = 0; j < 4; j++) {
			if (checkBoard[i][j] == 0)
				DrawBlankPin(hdc, gameBoardPin[i][0][j].x, gameBoardPin[i][0][j].y, pinSize[CHECK]);
			else
				DrawPin(hdc, checkPin[checkBoard[i][j] - 1], gameBoardPin[i][0][j].x, gameBoardPin[i][0][j].y, pinSize[CHECK]);
			if (gameBoard[i][j] == -1)
				DrawBlankPin(hdc, gameBoardPin[i][1][j].x, gameBoardPin[i][1][j].y, pinSize[GAME]);
			else
				DrawPin(hdc, pinList[gameBoard[i][j]], gameBoardPin[i][1][j].x, gameBoardPin[i][1][j].y, pinSize[GAME]);
		}
	}
	DrawRoundRect(hdc, answerBoardRect, 20, 20, RGB(45, 45, 45));
	
	// Draw select board
	SetRect(&strR, selectPin[0].x - 140, selectPin[0].y - 30, selectPin[0].x - 90, selectPin[0].y + 30);
	if (bPlayer == TWO && leftChance % 2 == 1)
		DrawTextRect(hdc, &strR, playerText[1], 50, DT_CENTER | DT_VCENTER);
	else 
		DrawTextRect(hdc, &strR, playerText[0], 50, DT_CENTER | DT_VCENTER);

	for (int i = 0; i < MAX_PIN; i++)
		DrawPin(hdc, pinList[i], selectPin[i].x, selectPin[i].y, pinSize[SELECT]);
	DrawRoundRect(hdc, answerKeyRect, 20, 20, RGB(120, 120, 120));
	buffer = answerKeyRect;
	buffer.top += 5;
	DrawTextRect(hdc, &buffer, enter, 30, DT_CENTER | DT_VCENTER);

	// Draw answer board
	for (int i = 0; i < 4; i++)
		DrawPin(hdc, pinList[answerBoard[i]], answerPin[i].x, answerPin[i].y, pinSize[GAME]);
	if (bGameState != END)
		DrawRoundRect(hdc, answerBoardRect, 20, 20, RGB(120, 120, 120));

	// Draw focus
	if (leftChance > 0)
		DrawFocus(hdc, gameBoardPin[MAX_CHANCE - leftChance][1][focus[0]], selectPin[focus[1]]);
}
void DrawMain(HDC hdc, RECT clientR, RECT rect[]) {
	POINT mPoint;
	int i;

	mPoint = CenterPoint(clientR);
	mPoint.y -= 350;

	// PtlnRect check할 Rect 초기화
	MakeStartRect(mPoint, rect);
	// Help 버튼을 위한 Rect 만들기
	SetRect(&rect[3], clientR.right - 100, clientR.top + 10, clientR.right - 10, clientR.top + 50);
	// Hit and Blow 글씨 쓰기
	DrawTitle(hdc, (TCHAR*)lpszClass, mPoint.x, mPoint.y, 150);
	mPoint.y += 300;
	mPoint.x -= 2 * pinSize[INIT];
	// Main 화면 그리기
	for (i = 0; i < MAX_PIN / 2; i++)
		DrawPin(hdc, pinList[i], mPoint.x + i * 2 * pinSize[INIT], mPoint.y, pinSize[INIT]);
	for (; i < MAX_PIN; i++)
		DrawPin(hdc, pinList[i], mPoint.x + (i - 3) * 2 * pinSize[INIT], mPoint.y + 2 * pinSize[INIT], pinSize[INIT]);
	// PtlnRect check할 Rect그리기
	DrawStartRect(hdc, rect);
}
void DrawTitle(HDC hdc, TCHAR str[], int x, int y, int size) {
	HFONT hFont, hOldFont;
	SetFont(size);
	hFont = CreateFontIndirect(&font);
	hOldFont = (HFONT)SelectObject(hdc, hFont);
	SetTextAlign(hdc, TA_CENTER);
	TextOut(hdc, x, y, str, _tcslen(str));
	SetTextAlign(hdc, TA_TOP | TA_LEFT);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}
void DrawStartRect(HDC hdc, RECT rect[]) {
	RECT buffer;
	TCHAR str[][10] = { _T("1 Player"), _T("2 Player"), _T("Exit"), _T("HELP") };

	for (int i = 0; i < 3; i++) {
		DrawRoundRect(hdc, rect[i], 40, 40, RGB(34, 163, 159));
		buffer = rect[i];
		buffer.top += 10;
		DrawTextRect(hdc, &buffer, str[i], 60, DT_CENTER | DT_VCENTER);
	}
	DrawTextRect(hdc, &rect[3], str[3], 30, DT_CENTER | DT_VCENTER);
}
void DrawFocus(HDC hdc, POINT game, POINT select) {
	POINT triPoint[2][6];
	HPEN hPen1, hPen2, hOldPen;
	HBRUSH hBrush, hOldBrush;
	int pointNum[2] = { 3, 3 };

	hPen1 = CreatePen(PS_DASH, 8, RGB(0, 141, 98));
	hPen2 = CreatePen(PS_DASH, 5, RGB(0, 141, 98));
	hBrush = CreateSolidBrush(RGB(0, 141, 98));
	hOldPen = (HPEN)SelectObject(hdc, hPen1);
	hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

	// Set Triangle
	SetVTriPoint(triPoint[0], game);
	SetHTriPoint(triPoint[1], select);

	Ellipse(hdc, game.x - pinSize[GAME], game.y - pinSize[GAME], game.x + pinSize[GAME], game.y + pinSize[GAME]);
	Ellipse(hdc, select.x - pinSize[SELECT], select.y - pinSize[SELECT], select.x + pinSize[SELECT], select.y + pinSize[SELECT]);
	
	SelectObject(hdc, hPen2);
	SelectObject(hdc, hBrush);
	for (int i = 0; i < 2; i++)
		PolyPolygon(hdc, triPoint[i], pointNum, 2);

	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBrush);
	DeleteObject(hPen1);
	DeleteObject(hPen2);
	DeleteObject(hBrush);
}
void SetHTriPoint(POINT tri[], POINT src) {
	tri[0].x = src.x - 60;
	tri[0].y = src.y - 30;
	tri[1].x = src.x - 60;
	tri[1].y = src.y + 30;
	tri[2].x = src.x - 80;
	tri[2].y = src.y;

	tri[3].x = src.x + 60;
	tri[3].y = src.y - 30;
	tri[4].x = src.x + 60;
	tri[4].y = src.y + 30;
	tri[5].x = src.x + 80;
	tri[5].y = src.y;
}
void SetVTriPoint(POINT tri[], POINT src) {
	tri[0].x = src.x - 20;
	tri[0].y = src.y - 40;
	tri[1].x = src.x + 20;
	tri[1].y = src.y - 40;
	tri[2].x = src.x;
	tri[2].y = src.y - 50;

	tri[3].x = src.x + 20;
	tri[3].y = src.y + 40;
	tri[4].x = src.x - 20;
	tri[4].y = src.y + 40;
	tri[5].x = src.x;
	tri[5].y = src.y + 50;
}
void DrawPin(HDC hdc, COLORREF pin, int x, int y, int size) {
	HBRUSH hBrush, hOldBrush;
	HPEN hPen, hOldPen;
	hPen = (HPEN)GetStockObject(NULL_PEN);
	hOldPen = (HPEN)SelectObject(hdc, hPen);
	hBrush = CreateSolidBrush(pin);

	// 실제 원 그리기
	hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
	Ellipse(hdc, x - size, y - size, x + size, y + size);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hBrush);
}
void DrawBlankPin(HDC hdc, int x, int y, int size) {
	DrawPin(hdc, RGB(30, 30, 30), x, y, size);
}
void DrawRoundRect(HDC hdc, RECT rect, int width, int height, COLORREF color) {
	HPEN hOldPen;
	HBRUSH hBrush, hOldBrush;

	hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
	hBrush = CreateSolidBrush(color);
	hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
	RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, width, height);

	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);
}
void DrawTextRect(HDC hdc, RECT * rect, TCHAR * str, int height, UINT format) {
	HFONT hFont, hOldFont;
	SetFont(height);
	hFont = CreateFontIndirect(&font);
	hOldFont = (HFONT)SelectObject(hdc, hFont);
	DrawText(hdc, str, _tcslen(str), rect, format);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}
void DrawTextRect(HDC hdc, RECT * rect, LPCTSTR str, int height, UINT format) {
	HFONT hFont, hOldFont;
	SetFont(height);
	hFont = CreateFontIndirect(&font);
	hOldFont = (HFONT)SelectObject(hdc, hFont);
	DrawText(hdc, str, _tcslen(str), rect, format);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}
void SortCheckBoard() {
	int i, checkNum[3] = { 0, 0, 0 }, temp[4] = { 0, 0, 0, 0 };
	
	for (i = 0; i < 4; i++)
		checkNum[2 - checkBoard[MAX_CHANCE - leftChance][i]]++;
	for (i = 1; i < 3; i++)
		checkNum[i] += checkNum[i - 1];

	for (i = 3; i > -1; i--) {
		temp[checkNum[2 - checkBoard[MAX_CHANCE - leftChance][i]] - 1] = checkBoard[MAX_CHANCE - leftChance][i];
		checkNum[2 - checkBoard[MAX_CHANCE - leftChance][i]]--;
	}
	for (i = 0; i < 4; i++)
		checkBoard[MAX_CHANCE - leftChance][i] = temp[i];
}
BOOL IsIn(int arr[], int num, int size) {
	for (int i = 0; i < size; i++) {
		if (arr[i] == num)
			return TRUE;
	}
	return FALSE;
}
BOOL CheckHitBlow() {
	int i, j, hitBlow[4] = { -1, -1, -1, -1 };

	// find hit
	for (i = 0; i < 4; i++) {
		if (gameBoard[MAX_CHANCE - leftChance][i] == answerBoard[i])
			hitBlow[i] = i;
	}

	// find blow
	for (i = 0; i < 4; i++) {
		if (hitBlow[i] == -1) {	// hit가 아닌 pin만 체크
			j = (i + 1) % 4;
			while (j != i) {
				if (!IsIn(hitBlow, j, 4) && gameBoard[MAX_CHANCE - leftChance][i] == answerBoard[j]) {
					hitBlow[i] = j;
					j = i;
				}
				else
					j = (j + 1) % 4;
			}
		}
	}

	// checkBoard에 hit, blow 정보 내림차순으로 기록
	for (i = 0; i < 4; i++) {
		if (hitBlow[i] == -1)
			checkBoard[MAX_CHANCE - leftChance][i] = 0;		// no
		else if (hitBlow[i] == i)
			checkBoard[MAX_CHANCE - leftChance][i] = 2;		// hit
		else
			checkBoard[MAX_CHANCE - leftChance][i] = 1;		// blow
	}
	SortCheckBoard();

	// 맞쳤을 경우 승자 판단
	if (checkBoard[MAX_CHANCE - leftChance][3] == 2) { // 정답 맞춤
		if (bPlayer == ONE)
			bWin = WIN;
		else {
			if ((MAX_CHANCE - leftChance) % 2 == 0)
				bWin = PLAYER1;
			else
				bWin = PLAYER2;
		}
		return TRUE;
	}
	else {												// 정답 맞추지 못함
		leftChance--;
		focus[0] = focus[1] = 0;
		return FALSE;
	}
}