#include <windows.h>
#include <tchar.h>
#include <mmsystem.h>			// PlaySound 함수 사용을 위함(추가 종속성에 winmm.lib추가 필요)
#include "resource.h"

// 매크로 상수
#define random(n) (rand()%n)	// 0-n까지 랜덤 수 
#define TS 24					// 블록 픽셀 크기

// 함수원형
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// 메인 윈도우 Proc
LRESULT CALLBACK PauseChildProc(HWND, UINT, WPARAM, LPARAM);	// 잠시 중지 상태일 때 화면에 출력할 윈도우 Proc
// 화면관련 함수
void DrawScreen(HDC hdc);								// 화면 그리기
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit);	// 비트맵 그리기
void PrintTile(HDC hdc, int x, int y, int c);			// 타일 그리기
void UpdateBoard();										// 게임판 업데이트(게임판 외 화면 깜밖임 제거)
// 블록관련 함수
void MakeNewBrick();									// 새로운 블록 만들고, 다음 블록 생성
int GetAround(int x, int y, int b, int r);				// 블록 주위 게임판 정보 Get
BOOL MoveDown();										// 블록을 내려보냄. 바닥에 닿으면 TRUE, 닿지않으면 FALSE
void TestFull();										// 블록을 기록하거나 가득찬 벽돌을 제거
BOOL IsMovingBrick(int x, int y);						// 이동중인 블록의 좌표 조사(이동하는 블록의 깜밖임 제거)
// 설정관련 함수
void PlayEffectSound(UINT Sound);						// 사운드 출력
void AdjustMainWindow();								// 초기 Window 화면 설정

// 구조체 및 엵거형 선언
struct Point {											// 좌표용 구조체
	int x, y;
};
enum class tag_Status { GAMEOVER, RUNNING, PAUSE };		// 게임 상태용 열거형

// 전역 변수
HINSTANCE g_hInst;
HWND hWndMain, hPauseChild;
LPCTSTR lpszClass = _T("Tetris");

// 게임판 크기 관련 변수
int arBW[] = { 8, 10, 12, 15, 20 };		// 게임판 가로 길이 리스트
int arBH[] = { 15, 20, 25, 30, 32 };	// 게임판 세로 길이 리스트
int BW = 10;							// 현재 게임판 가로 길이
int BH = 20;							// 현재 게임판 세로 길이
int board[22][34];						// 게임판 크기(게임판 크기 제한의 최대값으로 설정)
// 블록 종류 설정 0-9
Point Shape[][4][4] = {
	{ {0,0,1,0,2,0,-1,0}, {0,0,0,1,0,-1,0,-2}, {0,0,1,0,2,0,-1,0}, {0,0,0,1,0,-1,0,-2} },
	{ {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1} },
	{ {0,0,-1,0,0,-1,1,-1}, {0,0,0,1,-1,0,-1,-1}, {0,0,-1,0,0,-1,1,-1}, {0,0,0,1,-1,0,-1,-1} },
	{ {0,0,-1,-1,0,-1,1,0}, {0,0,-1,0,-1,1,0,-1}, {0,0,-1,-1,0,-1,1,0}, {0,0,-1,0,-1,1,0,-1} },
	{ {0,0,-1,0,1,0,-1,-1}, {0,0,0,-1,0,1,-1,1}, {0,0,-1,0,1,0,1,1}, {0,0,0,-1,0,1,1,-1} },
	{ {0,0,1,0,-1,0,1,-1}, {0,0,0,1,0,-1,-1,-1}, {0,0,1,0,-1,0,-1,1}, {0,0,0,-1,0,1,1,1} },
	{ {0,0,-1,0,1,0,0,1}, {0,0,0,-1,0,1,1,0}, {0,0,-1,0,1,0,0,-1}, {0,0,-1,0,0,-1,0,1} },
	{ {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0} },
	{ {0,0,0,0,0,-1,1,0},{0,0,0,0,-1,0,0,-1},{0,0,0,0,0,1,-1,0},{0,0,0,0,0,1,1,0} },
};
// 게임판 각 위치 상태 (공백, 블록, 벽)
enum { EMPTY, BRICK, WALL = sizeof(Shape) / sizeof(Shape[0]) + 1 };	
int nx, ny, rot;			// 현재 블록 위치, 회전정도(0, 1, 2, 3)
int brick, nbrick;			// 현재 블록 종류(0-9), 다음 블록 종류(0-9)
int score;					// 점수
int bricknum;				// 생성된 블록의 수 
int interval;				// 타이머 시간(1000-200)
BOOL bShowSpace = TRUE;		// 공백 표시 결정 변수
BOOL bQuiet = FALSE;		// 사운드 여부 결정 변수
tag_Status GameStatus;		// 게임 상태 
HBITMAP hBit[11];			// 비트맵 저장 변수

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpszCmdParam, _In_ int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	HACCEL hAccel;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	// 메인 윈도우
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);
	
	// 잠시 중지 표시 기능을 위한 윈도우
	WndClass.lpfnWndProc = PauseChildProc;
	WndClass.lpszClassName = _T("PauseChild");
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_SAVEBITS;
	RegisterClass(&WndClass);

	// nWidth, nHeight를 0, 0으로 설정해서 window 생성
	hWnd = CreateWindow(lpszClass, lpszClass, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	while (GetMessage(&Message, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &Message)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}
	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage,
	WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	int i, trot, x, y;

	switch (iMessage) {
	case WM_CREATE:
		hWndMain = hWnd;
		hPauseChild = CreateWindow(_T("PauseChild"), NULL, WS_CHILD | WS_BORDER, 0, 0, 0, 0,
			hWnd, (HMENU)0, g_hInst, NULL);	// 잠시 중지 윈도우(자식 윈도우) 생성
		AdjustMainWindow();
		GameStatus = tag_Status::GAMEOVER;
		srand(GetTickCount64());		// 랜덤 seed를 실행시간(시스템이 시작된 후 경과시간)으로 정함 
		for (i = 0; i < 11; i++)
			hBit[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1 + i));
		return 0;
	case WM_COMMAND:
		// 게임판 크기 변경
		if (LOWORD(wParam) >= IDM_GAME_SIZE1 && LOWORD(wParam) <= IDM_GAME_SIZE5) {
			if (GameStatus != tag_Status::GAMEOVER)
				return 0;

			BW = arBW[LOWORD(wParam) - IDM_GAME_SIZE1];
			BH = arBH[LOWORD(wParam) - IDM_GAME_SIZE1];
			AdjustMainWindow();
			memset(board, 0, sizeof(board));	// 게임판 초기화
			InvalidateRect(hWnd, NULL, TRUE);
		}
		switch (LOWORD(wParam)) {
		case IDM_GAME_START:		// 게임 시작
			// 게임이 진행중이지 않을 때만 IDM_GAME_START 수행
			if (GameStatus != tag_Status::GAMEOVER)	
				break;

			// 게임판 초기화(외곽은 벽으로, 나머지는 빈칸으로 설정)
			for (x = 0; x < BW + 2; x++) {
				for (y = 0; y < BH + 2; y++)
					board[x][y] = (y == 0 || y == BH + 1 || x == 0 || x == BW + 1) ? WALL : EMPTY;
			}
			
			// 게임 상태 초기화
			score = 0;
			bricknum = 0;
			GameStatus = tag_Status::RUNNING;
			nbrick = random(sizeof(Shape) / sizeof(Shape[0]));
			MakeNewBrick();
			interval = 1000;					
			SetTimer(hWnd, 1, interval, NULL);	
			break;
		case IDM_GAME_PAUSE:		// 게임 일지정지
			// 실행 중이면 일시정지
			if (GameStatus == tag_Status::RUNNING) {
				GameStatus = tag_Status::PAUSE;
				SetTimer(hWnd, 1, 1000, NULL);
				ShowWindow(hPauseChild, SW_SHOW);
			}
			// 일시정지 중이면 다시 실행
			else if (GameStatus == tag_Status::PAUSE) {
				GameStatus = tag_Status::RUNNING;
				SetTimer(hWnd, 1, interval, NULL);
				ShowWindow(hPauseChild, SW_HIDE);
			}
			// 정지상태이면 아무 일 하지 않음
			break;
		case IDM_GAME_VIEWSPACE:	// 공백 show 설정
			bShowSpace = !bShowSpace;
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_GAME_QUIET:		// 사운드 설정
			bQuiet = !bQuiet;
			break;
		case IDM_GAME_EXIT:			// 게임 종료 및 window 종료
			DestroyWindow(hWnd);	
			break;
		}
		return 0;
	case WM_INITMENU:	// 메뉴가 열릴 때 전달되는 메시지
		CheckMenuItem((HMENU)wParam, IDM_GAME_VIEWSPACE,
			MF_BYCOMMAND | (bShowSpace ? MF_CHECKED : MF_UNCHECKED)); // 메뉴의 체크 상태 변경
		return 0;
	case WM_TIMER:		// 자동으로 블록 내려감
		if (GameStatus == tag_Status::RUNNING) {
			if (MoveDown() == TRUE)
				MakeNewBrick();
		}
		else {
			// timer 간격마다 child window 깜박임
			if (IsWindowVisible(hPauseChild))
				ShowWindow(hPauseChild, SW_HIDE);
			else
				ShowWindow(hPauseChild, SW_SHOW);
		}
	
		return 0;
	case WM_KEYDOWN:	// 블록 이동 컨트롤 
		// 게임이 진행중이거나 이동중인 블록이 없으면 작동x
		if (GameStatus != tag_Status::RUNNING || brick == -1)	
			return 0;
		switch (wParam) {
		case VK_LEFT:	// 왼쪽 이동
			if (GetAround(nx - 1, ny, brick, rot) == EMPTY) {
				if ((lParam & 0x40000000) == 0)
					PlayEffectSound(IDR_WAVE1);
				nx--;
				UpdateBoard();
			}
			break;
		case VK_RIGHT:	// 오른쪽 이동
			if (GetAround(nx + 1, ny, brick, rot) == EMPTY) {
				if ((lParam & 0x40000000) == 0)
					PlayEffectSound(IDR_WAVE4);
				nx++;
				UpdateBoard();
			}
			break;
		case VK_UP:		// 블럭 회전
			trot = (rot == 3 ? 0 : rot + 1);
			if (GetAround(nx, ny, brick, trot) == EMPTY) {
				PlayEffectSound(IDR_WAVE1);
				rot = trot; 
				UpdateBoard();
			}
			break;
		case VK_DOWN:	// 블록 한칸씩 내리기
			if (MoveDown() == TRUE)
				MakeNewBrick();
			break;
		case VK_SPACE:	// 한번에 블록 내리기
			PlayEffectSound(IDR_WAVE3);
			while (MoveDown() == FALSE) { ; }
			MakeNewBrick();
			break;
		}
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawScreen(hdc);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		for (i = 0; i < 11; i++)
			DeleteObject(hBit[i]);
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}
LRESULT CALLBACK PauseChildProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;
	RECT crt;
	TEXTMETRIC tm;

	switch (iMessage) {
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &crt);
		SetTextAlign(hdc, TA_CENTER);
		GetTextMetrics(hdc, &tm);
		TextOut(hdc, crt.right / 2, crt.bottom / 2 - tm.tmHeight / 2, _T("PAUSE"), 5);
		EndPaint(hWnd, &ps);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}


void DrawScreen(HDC hdc) {
	int x, y, i;
	TCHAR str[128];

	// 테트리스 게임판 테두리 그림 bitmap11
	for (x = 0; x < BW + 1; x++) {
		PrintTile(hdc, x, 0, WALL);
		PrintTile(hdc, x, BH + 1, WALL);
	}
	for (y = 0; y < BH + 2; y++) {
		PrintTile(hdc, 0, y, WALL);
		PrintTile(hdc, BW + 1, y, WALL);
	}

	// 게임판과 이동중인 블록을 한번에 그림 (게임판의 인수 값에 따라 bitmap을 가져와 그림)
	for (x = 1; x < BW + 1; x++) {
		for (y = 1; y < BH + 1; y++) {
			if (IsMovingBrick(x, y))
				PrintTile(hdc, x, y, brick + 1);
			else
				PrintTile(hdc, x, y, board[x][y]);
		}

	}

	// 다음 벽돌 그림
	for (x = BW + 3; x <= BW + 11; x++) {
		for (y = BH - 5; y <= BH + 1; y++) {
			if (x == BW + 3 || x == BW + 11 || y == BH - 5 || y == BH + 1)	// 다음 벽돌 그릴 위치 테두리
				PrintTile(hdc, x, y, WALL);		
			else
				PrintTile(hdc, x, y, 0);
		}
	}
	if (GameStatus != tag_Status::GAMEOVER) {
		for (i = 0; i < 4; i++)
			PrintTile(hdc, BW + 7 + Shape[nbrick][0][i].x, BH - 2 + Shape[nbrick][0][i].y, nbrick + 1);
	}

	// 정보 출력
	lstrcpy(str, _T("Teris Ver 1.2"));
	TextOut(hdc, (BW + 4) * TS, 30, str, _tcslen(str));
	wsprintf(str, _T("점수 : %d   "), score);
	TextOut(hdc, (BW + 4) * TS, 60, str, _tcslen(str));
	wsprintf(str, _T("벽돌 : %d 개  "), bricknum);
	TextOut(hdc, (BW + 4) * TS, 80, str, _tcslen(str));
}
void PrintTile(HDC hdc, int x, int y, int c) {
	DrawBitmap(hdc, x * TS, y * TS, hBit[c]);

	// 공백 표시버튼이 활성화되어 있는 경우 작은 사각형 그림(점을 찍으면 너무 작아서)
	if (c == EMPTY && bShowSpace)
		Rectangle(hdc, x * TS + TS / 2 - 1, y * TS + TS / 2 - 1, x * TS + TS / 2 + 1, y * TS + TS / 2 + 1);
}
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit) {
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx, by;
	BITMAP bit;
	
	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);

	GetObject(hBit, sizeof(BITMAP), &bit);
	bx = bit.bmWidth;
	by = bit.bmHeight;

	BitBlt(hdc, x, y, bx, by, MemDC, 0, 0, SRCCOPY);
	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}
void UpdateBoard() {
	RECT rt; 
	
	SetRect(&rt, TS, TS, (BW + 1) * TS, (BH + 1) * TS);
	InvalidateRect(hWndMain, &rt, TRUE);
}
void MakeNewBrick() {
	bricknum++;
	brick = nbrick;
	nbrick = random(sizeof(Shape) / sizeof(Shape[0]));
	
	// 현재 블록 정보 초기화
	nx = BW / 2;
	ny = 3;
	rot = 0;
	InvalidateRect(hWndMain, NULL, FALSE);
	
	// 게임 Over인 경우
	if (GetAround(nx, ny, brick, rot) != EMPTY) {
		KillTimer(hWndMain, 1);
		GameStatus = tag_Status::GAMEOVER;
		MessageBox(hWndMain, _T("게임이 끝났습니다. 다시 시작하려면 게임/시작"), 
			_T("항목(S)을 선택해 주십시오"), MB_OK);
	}
}
int GetAround(int x, int y, int b, int r) {
	int i, k = EMPTY;
	for (i = 0; i < 4; i++)
		k = max(k, board[x + Shape[b][r][i].x][y + Shape[b][r][i].y]);
	return k;
}
BOOL MoveDown() {
	// 바닥에 닿았으면
	if (GetAround(nx, ny + 1, brick, rot) != EMPTY) {
		TestFull();
		return TRUE;
	}
	// 닿지 않은 경우
	ny++;
	UpdateBoard();
	UpdateWindow(hWndMain);		// WM_PAINT 강제 발생(블록이 내려가는 것을 보여주기 위함) 
	return FALSE;
}
void TestFull() {
	int i, x, y, ty;
	int count = 0;
	static int arScoreInc[] = { 0,1,3,8,20 };

	// 게임판 정보 update(이동이 완료된 블록)
	for (i = 0; i < 4; i++) 
		board[nx + Shape[brick][rot][i].x][ny + Shape[brick][rot][i].y] = brick + 1;

	//이동중인 벽돌이 잠시 없는 상태(처리를 하지 않으면 화면을 다시 그릴 때 착륙한 벽돌이 허공에 떠있는 것처럼 나타남
	brick = -1;

	for (y = 1; y < BH + 1; y++) {
		// 한 가로줄이 블록으로 가득 찼는지 확인
		for (x = 1; x < BW + 1; x++) {
			if (board[x][y] == EMPTY) 
				break;
		}

		// 한 가로줄이 블록으로 가득 찬 경우 
		if (x == BW + 1) {
			PlayEffectSound(IDR_WAVE2);
			count++;					// 현재 몇 줄이 지워졌는지 기록
			// 현재 가로줄 지우고, 현재 가로줄을 기준으로 한줄씩 내림 
			for (ty = y; ty > 1; ty--) {
				for (x = 1; x < BW + 1; x++) {
					board[x][ty] = board[x][ty - 1];
				}
			}
			UpdateBoard();
			UpdateWindow(hWndMain);		// WM_PAINT 강제 발생(벽돌이 사라지는 모습을 보여주기 위함)
			Sleep(150);
		}
	}
	score += arScoreInc[count];			// 점수 계산(0줄, 1줄, 2줄, 3줄, 4줄, 5줄을 한번에 지운 정도 판단)

	// 떨어진 블럭 갯수가 10의 배수가 될 때마다 interval을 감소시킨다. 0.2초까지.
	if (bricknum % 10 == 0 && interval > 200) {
		interval -= 50;					// 블록이 자동으로 떨어지는 주기가 0.005초씩 빨라짐
		SetTimer(hWndMain, 1, interval, NULL);
	}
}
BOOL IsMovingBrick(int x, int y) {
	int i;
	if (GameStatus == tag_Status::GAMEOVER || brick == -1)
		return FALSE;
	
	// x, y가 현재 이동죽인 블록의 위치에 있는지 확인
	for (i = 0; i< 4; i++) {
		if (x == nx + Shape[brick][rot][i].x && y == ny + Shape[brick][rot][i].y)
			return TRUE;
	}
	return FALSE;
}
void PlayEffectSound(UINT Sound) {
	if (!bQuiet)
		PlaySound(MAKEINTRESOURCE(Sound), g_hInst, SND_RESOURCE | SND_ASYNC);
}
void AdjustMainWindow() {
	RECT crt;

	SetRect(&crt, 0, 0, (BW + 12) * TS, (BH + 2) * TS);
	// 윈도우 창크기 조정 
	AdjustWindowRect(&crt, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
	//제목표시줄+창메뉴+창에최소화버튼 옵션
	SetWindowPos(hWndMain, NULL, 0, 0, crt.right - crt.left, crt.bottom - crt.top,
		SWP_NOMOVE | SWP_NOZORDER);	// window 크기를 crt크기로 지정, 현재 위치(x, y, z 유지)
	SetWindowPos(hPauseChild, NULL, (BW + 12) * TS / 2 - 100, (BH + 2) * TS / 2 - 50, 200, 100, SWP_NOZORDER);

}