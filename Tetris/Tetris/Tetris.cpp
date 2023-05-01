#include <windows.h>
#include <tchar.h>
#include <mmsystem.h>			// PlaySound �Լ� ����� ����(�߰� ���Ӽ��� winmm.lib�߰� �ʿ�)
#include "resource.h"

// ��ũ�� ���
#define random(n) (rand()%n)	// 0-n���� ���� �� 
#define TS 24					// ��� �ȼ� ũ��

// �Լ�����
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// ���� ������ Proc
LRESULT CALLBACK PauseChildProc(HWND, UINT, WPARAM, LPARAM);	// ��� ���� ������ �� ȭ�鿡 ����� ������ Proc
// ȭ����� �Լ�
void DrawScreen(HDC hdc);								// ȭ�� �׸���
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit);	// ��Ʈ�� �׸���
void PrintTile(HDC hdc, int x, int y, int c);			// Ÿ�� �׸���
void UpdateBoard();										// ������ ������Ʈ(������ �� ȭ�� ������ ����)
// ��ϰ��� �Լ�
void MakeNewBrick();									// ���ο� ��� �����, ���� ��� ����
int GetAround(int x, int y, int b, int r);				// ��� ���� ������ ���� Get
BOOL MoveDown();										// ����� ��������. �ٴڿ� ������ TRUE, ���������� FALSE
void TestFull();										// ����� ����ϰų� ������ ������ ����
BOOL IsMovingBrick(int x, int y);						// �̵����� ����� ��ǥ ����(�̵��ϴ� ����� ������ ����)
// �������� �Լ�
void PlayEffectSound(UINT Sound);						// ���� ���
void AdjustMainWindow();								// �ʱ� Window ȭ�� ����

// ����ü �� ������ ����
struct Point {											// ��ǥ�� ����ü
	int x, y;
};
enum class tag_Status { GAMEOVER, RUNNING, PAUSE };		// ���� ���¿� ������

// ���� ����
HINSTANCE g_hInst;
HWND hWndMain, hPauseChild;
LPCTSTR lpszClass = _T("Tetris");

// ������ ũ�� ���� ����
int arBW[] = { 8, 10, 12, 15, 20 };		// ������ ���� ���� ����Ʈ
int arBH[] = { 15, 20, 25, 30, 32 };	// ������ ���� ���� ����Ʈ
int BW = 10;							// ���� ������ ���� ����
int BH = 20;							// ���� ������ ���� ����
int board[22][34];						// ������ ũ��(������ ũ�� ������ �ִ밪���� ����)
// ��� ���� ���� 0-9
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
// ������ �� ��ġ ���� (����, ���, ��)
enum { EMPTY, BRICK, WALL = sizeof(Shape) / sizeof(Shape[0]) + 1 };	
int nx, ny, rot;			// ���� ��� ��ġ, ȸ������(0, 1, 2, 3)
int brick, nbrick;			// ���� ��� ����(0-9), ���� ��� ����(0-9)
int score;					// ����
int bricknum;				// ������ ����� �� 
int interval;				// Ÿ�̸� �ð�(1000-200)
BOOL bShowSpace = TRUE;		// ���� ǥ�� ���� ����
BOOL bQuiet = FALSE;		// ���� ���� ���� ����
tag_Status GameStatus;		// ���� ���� 
HBITMAP hBit[11];			// ��Ʈ�� ���� ����

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpszCmdParam, _In_ int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	HACCEL hAccel;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	// ���� ������
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
	
	// ��� ���� ǥ�� ����� ���� ������
	WndClass.lpfnWndProc = PauseChildProc;
	WndClass.lpszClassName = _T("PauseChild");
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_SAVEBITS;
	RegisterClass(&WndClass);

	// nWidth, nHeight�� 0, 0���� �����ؼ� window ����
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
			hWnd, (HMENU)0, g_hInst, NULL);	// ��� ���� ������(�ڽ� ������) ����
		AdjustMainWindow();
		GameStatus = tag_Status::GAMEOVER;
		srand(GetTickCount64());		// ���� seed�� ����ð�(�ý����� ���۵� �� ����ð�)���� ���� 
		for (i = 0; i < 11; i++)
			hBit[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1 + i));
		return 0;
	case WM_COMMAND:
		// ������ ũ�� ����
		if (LOWORD(wParam) >= IDM_GAME_SIZE1 && LOWORD(wParam) <= IDM_GAME_SIZE5) {
			if (GameStatus != tag_Status::GAMEOVER)
				return 0;

			BW = arBW[LOWORD(wParam) - IDM_GAME_SIZE1];
			BH = arBH[LOWORD(wParam) - IDM_GAME_SIZE1];
			AdjustMainWindow();
			memset(board, 0, sizeof(board));	// ������ �ʱ�ȭ
			InvalidateRect(hWnd, NULL, TRUE);
		}
		switch (LOWORD(wParam)) {
		case IDM_GAME_START:		// ���� ����
			// ������ ���������� ���� ���� IDM_GAME_START ����
			if (GameStatus != tag_Status::GAMEOVER)	
				break;

			// ������ �ʱ�ȭ(�ܰ��� ������, �������� ��ĭ���� ����)
			for (x = 0; x < BW + 2; x++) {
				for (y = 0; y < BH + 2; y++)
					board[x][y] = (y == 0 || y == BH + 1 || x == 0 || x == BW + 1) ? WALL : EMPTY;
			}
			
			// ���� ���� �ʱ�ȭ
			score = 0;
			bricknum = 0;
			GameStatus = tag_Status::RUNNING;
			nbrick = random(sizeof(Shape) / sizeof(Shape[0]));
			MakeNewBrick();
			interval = 1000;					
			SetTimer(hWnd, 1, interval, NULL);	
			break;
		case IDM_GAME_PAUSE:		// ���� ��������
			// ���� ���̸� �Ͻ�����
			if (GameStatus == tag_Status::RUNNING) {
				GameStatus = tag_Status::PAUSE;
				SetTimer(hWnd, 1, 1000, NULL);
				ShowWindow(hPauseChild, SW_SHOW);
			}
			// �Ͻ����� ���̸� �ٽ� ����
			else if (GameStatus == tag_Status::PAUSE) {
				GameStatus = tag_Status::RUNNING;
				SetTimer(hWnd, 1, interval, NULL);
				ShowWindow(hPauseChild, SW_HIDE);
			}
			// ���������̸� �ƹ� �� ���� ����
			break;
		case IDM_GAME_VIEWSPACE:	// ���� show ����
			bShowSpace = !bShowSpace;
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_GAME_QUIET:		// ���� ����
			bQuiet = !bQuiet;
			break;
		case IDM_GAME_EXIT:			// ���� ���� �� window ����
			DestroyWindow(hWnd);	
			break;
		}
		return 0;
	case WM_INITMENU:	// �޴��� ���� �� ���޵Ǵ� �޽���
		CheckMenuItem((HMENU)wParam, IDM_GAME_VIEWSPACE,
			MF_BYCOMMAND | (bShowSpace ? MF_CHECKED : MF_UNCHECKED)); // �޴��� üũ ���� ����
		return 0;
	case WM_TIMER:		// �ڵ����� ��� ������
		if (GameStatus == tag_Status::RUNNING) {
			if (MoveDown() == TRUE)
				MakeNewBrick();
		}
		else {
			// timer ���ݸ��� child window ������
			if (IsWindowVisible(hPauseChild))
				ShowWindow(hPauseChild, SW_HIDE);
			else
				ShowWindow(hPauseChild, SW_SHOW);
		}
	
		return 0;
	case WM_KEYDOWN:	// ��� �̵� ��Ʈ�� 
		// ������ �������̰ų� �̵����� ����� ������ �۵�x
		if (GameStatus != tag_Status::RUNNING || brick == -1)	
			return 0;
		switch (wParam) {
		case VK_LEFT:	// ���� �̵�
			if (GetAround(nx - 1, ny, brick, rot) == EMPTY) {
				if ((lParam & 0x40000000) == 0)
					PlayEffectSound(IDR_WAVE1);
				nx--;
				UpdateBoard();
			}
			break;
		case VK_RIGHT:	// ������ �̵�
			if (GetAround(nx + 1, ny, brick, rot) == EMPTY) {
				if ((lParam & 0x40000000) == 0)
					PlayEffectSound(IDR_WAVE4);
				nx++;
				UpdateBoard();
			}
			break;
		case VK_UP:		// �� ȸ��
			trot = (rot == 3 ? 0 : rot + 1);
			if (GetAround(nx, ny, brick, trot) == EMPTY) {
				PlayEffectSound(IDR_WAVE1);
				rot = trot; 
				UpdateBoard();
			}
			break;
		case VK_DOWN:	// ��� ��ĭ�� ������
			if (MoveDown() == TRUE)
				MakeNewBrick();
			break;
		case VK_SPACE:	// �ѹ��� ��� ������
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

	// ��Ʈ���� ������ �׵θ� �׸� bitmap11
	for (x = 0; x < BW + 1; x++) {
		PrintTile(hdc, x, 0, WALL);
		PrintTile(hdc, x, BH + 1, WALL);
	}
	for (y = 0; y < BH + 2; y++) {
		PrintTile(hdc, 0, y, WALL);
		PrintTile(hdc, BW + 1, y, WALL);
	}

	// �����ǰ� �̵����� ����� �ѹ��� �׸� (�������� �μ� ���� ���� bitmap�� ������ �׸�)
	for (x = 1; x < BW + 1; x++) {
		for (y = 1; y < BH + 1; y++) {
			if (IsMovingBrick(x, y))
				PrintTile(hdc, x, y, brick + 1);
			else
				PrintTile(hdc, x, y, board[x][y]);
		}

	}

	// ���� ���� �׸�
	for (x = BW + 3; x <= BW + 11; x++) {
		for (y = BH - 5; y <= BH + 1; y++) {
			if (x == BW + 3 || x == BW + 11 || y == BH - 5 || y == BH + 1)	// ���� ���� �׸� ��ġ �׵θ�
				PrintTile(hdc, x, y, WALL);		
			else
				PrintTile(hdc, x, y, 0);
		}
	}
	if (GameStatus != tag_Status::GAMEOVER) {
		for (i = 0; i < 4; i++)
			PrintTile(hdc, BW + 7 + Shape[nbrick][0][i].x, BH - 2 + Shape[nbrick][0][i].y, nbrick + 1);
	}

	// ���� ���
	lstrcpy(str, _T("Teris Ver 1.2"));
	TextOut(hdc, (BW + 4) * TS, 30, str, _tcslen(str));
	wsprintf(str, _T("���� : %d   "), score);
	TextOut(hdc, (BW + 4) * TS, 60, str, _tcslen(str));
	wsprintf(str, _T("���� : %d ��  "), bricknum);
	TextOut(hdc, (BW + 4) * TS, 80, str, _tcslen(str));
}
void PrintTile(HDC hdc, int x, int y, int c) {
	DrawBitmap(hdc, x * TS, y * TS, hBit[c]);

	// ���� ǥ�ù�ư�� Ȱ��ȭ�Ǿ� �ִ� ��� ���� �簢�� �׸�(���� ������ �ʹ� �۾Ƽ�)
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
	
	// ���� ��� ���� �ʱ�ȭ
	nx = BW / 2;
	ny = 3;
	rot = 0;
	InvalidateRect(hWndMain, NULL, FALSE);
	
	// ���� Over�� ���
	if (GetAround(nx, ny, brick, rot) != EMPTY) {
		KillTimer(hWndMain, 1);
		GameStatus = tag_Status::GAMEOVER;
		MessageBox(hWndMain, _T("������ �������ϴ�. �ٽ� �����Ϸ��� ����/����"), 
			_T("�׸�(S)�� ������ �ֽʽÿ�"), MB_OK);
	}
}
int GetAround(int x, int y, int b, int r) {
	int i, k = EMPTY;
	for (i = 0; i < 4; i++)
		k = max(k, board[x + Shape[b][r][i].x][y + Shape[b][r][i].y]);
	return k;
}
BOOL MoveDown() {
	// �ٴڿ� �������
	if (GetAround(nx, ny + 1, brick, rot) != EMPTY) {
		TestFull();
		return TRUE;
	}
	// ���� ���� ���
	ny++;
	UpdateBoard();
	UpdateWindow(hWndMain);		// WM_PAINT ���� �߻�(����� �������� ���� �����ֱ� ����) 
	return FALSE;
}
void TestFull() {
	int i, x, y, ty;
	int count = 0;
	static int arScoreInc[] = { 0,1,3,8,20 };

	// ������ ���� update(�̵��� �Ϸ�� ���)
	for (i = 0; i < 4; i++) 
		board[nx + Shape[brick][rot][i].x][ny + Shape[brick][rot][i].y] = brick + 1;

	//�̵����� ������ ��� ���� ����(ó���� ���� ������ ȭ���� �ٽ� �׸� �� ������ ������ ����� ���ִ� ��ó�� ��Ÿ��
	brick = -1;

	for (y = 1; y < BH + 1; y++) {
		// �� �������� ������� ���� á���� Ȯ��
		for (x = 1; x < BW + 1; x++) {
			if (board[x][y] == EMPTY) 
				break;
		}

		// �� �������� ������� ���� �� ��� 
		if (x == BW + 1) {
			PlayEffectSound(IDR_WAVE2);
			count++;					// ���� �� ���� ���������� ���
			// ���� ������ �����, ���� �������� �������� ���پ� ���� 
			for (ty = y; ty > 1; ty--) {
				for (x = 1; x < BW + 1; x++) {
					board[x][ty] = board[x][ty - 1];
				}
			}
			UpdateBoard();
			UpdateWindow(hWndMain);		// WM_PAINT ���� �߻�(������ ������� ����� �����ֱ� ����)
			Sleep(150);
		}
	}
	score += arScoreInc[count];			// ���� ���(0��, 1��, 2��, 3��, 4��, 5���� �ѹ��� ���� ���� �Ǵ�)

	// ������ �� ������ 10�� ����� �� ������ interval�� ���ҽ�Ų��. 0.2�ʱ���.
	if (bricknum % 10 == 0 && interval > 200) {
		interval -= 50;					// ����� �ڵ����� �������� �ֱⰡ 0.005�ʾ� ������
		SetTimer(hWndMain, 1, interval, NULL);
	}
}
BOOL IsMovingBrick(int x, int y) {
	int i;
	if (GameStatus == tag_Status::GAMEOVER || brick == -1)
		return FALSE;
	
	// x, y�� ���� �̵����� ����� ��ġ�� �ִ��� Ȯ��
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
	// ������ âũ�� ���� 
	AdjustWindowRect(&crt, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
	//����ǥ����+â�޴�+â���ּ�ȭ��ư �ɼ�
	SetWindowPos(hWndMain, NULL, 0, 0, crt.right - crt.left, crt.bottom - crt.top,
		SWP_NOMOVE | SWP_NOZORDER);	// window ũ�⸦ crtũ��� ����, ���� ��ġ(x, y, z ����)
	SetWindowPos(hPauseChild, NULL, (BW + 12) * TS / 2 - 100, (BH + 2) * TS / 2 - 50, 200, 100, SWP_NOZORDER);

}