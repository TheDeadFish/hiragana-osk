#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

const char regKey[] = "SOFTWARE\\DeadFishShitware\\hiragana-osk-VQKO";


#define DAKU 51
#define HANDA 55
#define KANASET 59
#define HIRAMAX 60
#define MAINMAX 50

LOGFONT g_lFontJap;
HFONT g_hfont;

static
void initLogFont(LOGFONT* lf, 
	LPCWSTR name, int PointSize)
{
	memset(lf, 0, sizeof(*lf));
	lf->lfHeight = -MulDiv(PointSize, 96, 72);
	wcscpy(lf->lfFaceName, name);
}

static 
HFONT create_font(LPCWSTR name, int pt)
{
	LOGFONT lf; initLogFont(&lf, name, pt);
	return CreateFontIndirect(&lf);
}


static
BOOL choose_font(LOGFONT* lf, HWND hwnd)
{
	CHOOSEFONT cf = {};
	cf.lStructSize = sizeof (cf);
	cf.lpLogFont = lf;
	cf.Flags = CF_SCREENFONTS | CF_EFFECTS 
		| CF_INITTOLOGFONTSTRUCT;
	return ChooseFont(&cf);
}

void init_font(void)
{
	initLogFont(&g_lFontJap, L"Arial", 24);
	
	// fetch font from registry
	HKEY hKey = NULL; 
	RegCreateKeyA(HKEY_CURRENT_USER, regKey, &hKey);
	DWORD dataSize = sizeof(g_lFontJap);
	RegQueryValueExA(hKey, "japFont", 0, NULL, 
		(LPBYTE)&g_lFontJap, &dataSize);
	RegCloseKey(hKey);
	
	g_hfont = CreateFontIndirect(&g_lFontJap);
}

void select_font(HWND hwnd)
{
	if(!choose_font(&g_lFontJap, hwnd)) 
		return;
	DeleteObject(g_hfont);
	g_hfont = CreateFontIndirect(&g_lFontJap);
	
	// save font to registry
	HKEY hKey = NULL;
	RegCreateKeyA(HKEY_CURRENT_USER, regKey, &hKey);
	RegSetValueExA(hKey, "japFont", 0, REG_BINARY, 
		(LPBYTE)&g_lFontJap, sizeof(g_lFontJap));
	RegCloseKey(hKey);
}

HFONT g_hFontLabel;
HPEN g_redPen;
HPEN g_facePen;



int g_buttonW = 48;
int g_buttonH = 40;
int g_selIndex = -1;
int g_shift = 0;

#define BUTTON_SPC 5
#define BUTTON_MRGN 5

WCHAR hiragana[] = L"あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもや ゆ よらりるれろわゐ ゑをん゛ゃゅょ゜っゝ *";
WCHAR katakana[] = L"アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤ ユ ヨラリルレロワヰ ヱヲン゛ャュョ゜ッヽー*";
WCHAR* g_charset = hiragana;
const char romanji_x[] = "-kstnhmyrw gzd b         p    ";

void getButtonChar(WCHAR* ch, int idx)
{
	*(DWORD*)ch = 0; if((idx == KANASET)||
	(idx == DAKU)||(idx == HANDA)) {
		ch[0] = g_charset[idx]; } else {
	switch(g_shift) {
	case 0: ch[0] = g_charset[idx]; break;
	case 1: switch(idx/5) {  case 1 ... 3:
		case 5: ch[0] = g_charset[idx]+1; }
	break;  default: if(idx/5 == 5)
		ch[0] = g_charset[idx]+2; } }
}

void getButtonRect(RECT* rc, int idx)
{
	if(g_charset[idx] == ' ') { memset(
		rc, -1, sizeof(*rc)); return; }
	
	// calculate x-y position
	int x, y, ofs = 0; if(idx >= 50) { x = 
	idx - 50; y = 5; ofs = BUTTON_SPC; } else {
	x = (idx / 5); y = idx % 5; }

	rc->left = BUTTON_MRGN + ((9-x) * (g_buttonW + BUTTON_SPC));
	rc->top = BUTTON_MRGN*5 + ofs + (y * (g_buttonH + BUTTON_SPC));
	rc->right = rc->left + g_buttonW;
	rc->bottom = rc->top + g_buttonH;
}

const WCHAR g_szClassName[] = L"hiragana-osk";

void drawButton(HDC hdc, RECT* rc,
	WCHAR* str, BOOL selected)
{
	// draw the background
	SelectObject(hdc, selected ? g_redPen : g_facePen);
	SelectObject(hdc, GetSysColorBrush((selected
		? COLOR_HOTLIGHT : COLOR_3DFACE)));
	RoundRect(hdc, rc->left, rc->top, 
		rc->right, rc->bottom, 10, 10);
		
	// draw the text
	SetTextColor(hdc, selected ? 0xFFFFFF : 0);
	DrawTextW(hdc, str, -1, rc, DT_VCENTER | DT_CENTER | DT_SINGLELINE);		
}

void redraw(HWND hwnd, HDC hdc)
{
	SelectObject(hdc, g_hfont);
	SetBkMode( hdc, TRANSPARENT );
	
	for(int i = 0; i < HIRAMAX; i++) 
	{
		RECT rc; getButtonRect(&rc, i);
		WCHAR str[2]; getButtonChar(str, i);
		drawButton(hdc, &rc, str, g_selIndex == i);
	}
	
	SelectObject(hdc, g_hFontLabel);	
	SetTextColor(hdc, 0xFFFFFF);
	const char* labels = romanji_x + g_shift*10;
	for(int i = 0; i < 10; i++) 
	{
		RECT rc; getButtonRect(&rc, i*5);
		rc.bottom = rc.top-3; rc.top = 0;
		WCHAR str[2] = {labels[i], 0};
		DrawTextW(hdc, str, -1, &rc, 
			DT_VCENTER | DT_CENTER | DT_SINGLELINE);		
	}
	
}

void OnMouseUp(HWND hwnd)
{
	// update character set
	if(g_selIndex == KANASET) {
		g_charset = (g_charset == hiragana)
			? katakana : hiragana;
		InvalidateRect(hwnd, NULL, TRUE);		
		return;	
	}

	// update shift state
	if((g_selIndex == DAKU) 
	||(g_selIndex == HANDA)) {
		int newShift = (g_selIndex==DAKU)?1:2;
		if(g_shift == newShift) g_shift = 0;
		else g_shift = newShift;
		InvalidateRect(hwnd, NULL, TRUE);		
		return;
	}
	
	// generate 
	WCHAR str[2]; getButtonChar(str, g_selIndex);
	if(str[0]) {
	INPUT ip; ip.type = INPUT_KEYBOARD; ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0; ip.ki.dwFlags = KEYEVENTF_UNICODE;
    ip.ki.wVk = 0; ip.ki.wScan = str[0]; 
    SendInput(1, &ip, sizeof(INPUT));
    ip.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT)); }
	if(g_shift) { g_shift = 0;
		InvalidateRect(hwnd, NULL, FALSE); }
}

void OnmoveMouse(HWND hwnd, LPARAM lParam)
{
	POINT pnt = { GET_X_LPARAM(lParam),
		GET_Y_LPARAM(lParam) };
		
	// check for character selection
	int newCharSel = -1;
	for(int i = 0; i < HIRAMAX; i++) {
		RECT rc; getButtonRect(&rc, i);
		if(PtInRect(&rc, pnt)) {
			newCharSel = i; goto FOUND; }
	}
	
	// update the window
FOUND:
	if(g_selIndex != newCharSel) {
		g_selIndex = newCharSel;
		InvalidateRect(hwnd, NULL, FALSE);
	}
}

void setClientSize(HWND hwnd, int x, int y)
{
	RECT rc1; GetClientRect(hwnd, &rc1);
	RECT rc2; GetWindowRect(hwnd, &rc2);
	x += (rc2.right - rc2.left) - rc1.right;
	y += (rc2.bottom - rc2.top) - rc1.bottom;
	MoveWindow(hwnd, rc2.left, rc2.top, x, y, TRUE);
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hPreWindow;
	
    switch(msg)
    {
		
	case WM_KEYDOWN:
		if(wParam == VK_F12) {
			select_font(hwnd);
			InvalidateRect(hwnd, NULL, TRUE);		
		} break;
	
	
	
	case WM_CREATE: {
		g_hFontLabel = create_font(L"Arial", 12);
		init_font();
		
		g_facePen = CreatePen(PS_SOLID, 3,
			GetSysColor(COLOR_3DFACE));
		g_redPen = CreatePen(PS_SOLID, 3, RGB(255,0,0));

		RECT rc; getButtonRect(&rc, MAINMAX);
		setClientSize(hwnd, rc.right + BUTTON_MRGN, 
			rc.bottom + BUTTON_MRGN);

		break; }
	
	
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			redraw(hwnd, hdc);
			EndPaint(hwnd, &ps);
			break;
			
			}
			
			
			
		case WM_EXITSIZEMOVE:
			if(hPreWindow) {
				SetForegroundWindow(hPreWindow);
				hPreWindow = NULL; }
		
		case WM_MOUSEMOVE:
			if(hPreWindow) {
				SetForegroundWindow(hPreWindow);
				hPreWindow = NULL; }
			OnmoveMouse(hwnd, lParam);
			break;
		case WM_LBUTTONUP:
			OnMouseUp(hwnd);
			break;
			
			
		case WM_ACTIVATEAPP:
			if(!wParam) hPreWindow = NULL;
			return DefWindowProc(hwnd, msg, wParam, lParam);
		
		case WM_NCLBUTTONDOWN: {
			HWND tmp = GetForegroundWindow();
			if(tmp != hwnd) hPreWindow = tmp;
			SetForegroundWindow(hwnd);
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
			
			
	
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    //Step 1: Registering the Window Class
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_HIGHLIGHT+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        MessageBoxA(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE |
		 WS_EX_NOACTIVATE | WS_EX_APPWINDOW | WS_EX_TOPMOST,
        g_szClassName, L"Hiragana OSK",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBoxA(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}