// This source file was based on Microsoft SDK Documents.

#include "ZFrameDXUT.h"
#include <map>
#include <cassert>
using namespace PaintsNow;

static std::map<int, ZFrameDXUT*> mapWindows;

static bool CALLBACK IsDeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
static bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);
static HRESULT CALLBACK OnCreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
static HRESULT CALLBACK OnResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
static void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
static void CALLBACK OnFrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);
static LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext);
static void CALLBACK KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);
static void CALLBACK OnLostDevice(void* pUserContext);
static void CALLBACK OnDestroyDevice(void* pUserContext);

//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning E_FAIL.
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext) {
	// No fallback defined by this app, so reject any device that 
	// doesn't support at least ps2.0
	if (pCaps->PixelShaderVersion < D3DPS_VERSION(2, 0))
		return false;

	// Skip backbuffer formats that don't support alpha blending
	IDirect3D9* pD3D = DXUTGetD3D9Object();
	if (FAILED(pD3D->CheckDeviceFormat(pCaps->AdapterOrdinal, pCaps->DeviceType,
		AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
		D3DRTYPE_TEXTURE, BackBufferFormat)))
		return false;

	return true;
}

//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext) {
	assert(DXUT_D3D9_DEVICE == pDeviceSettings->ver);

	HRESULT hr;
	IDirect3D9* pD3D = DXUTGetD3D9Object();
	D3DCAPS9 caps;

	V(pD3D->GetDeviceCaps(pDeviceSettings->d3d9.AdapterOrdinal,
	  pDeviceSettings->d3d9.DeviceType,
	  &caps));

	// If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
	// then switch to SWVP.
	if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
		caps.VertexShaderVersion < D3DVS_VERSION(1, 1)) {
		pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	// Debugging vertex shaders requires either REF or software vertex processing 
	// and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
	if (pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF) {
		pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
		pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
		pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}
#endif
#ifdef DEBUG_PS
	pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
	// For the first device created if its a REF device, optionally display a warning dialog box
	static bool s_bFirstTime = true;
	if (s_bFirstTime) {
		s_bFirstTime = false;
		if (pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF)
			DXUTDisplaySwitchingToREFWarning(pDeviceSettings->ver);
	}

	return true;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext) {
	ZFrameDXUT* frame = reinterpret_cast<ZFrameDXUT*>(pUserContext);
	// TODO: place init code here
	frame->OnInitialize(pd3dDevice);
	return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext) {
	ZFrameDXUT* frame = reinterpret_cast<ZFrameDXUT*>(pUserContext);
	// TODO: place reset code here

	return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext) {
	// Update the camera's position based on user input 
	// No operations here
}


//--------------------------------------------------------------------------------------
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, DXUT will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext) {
	ZFrameDXUT* frame = reinterpret_cast<ZFrameDXUT*>(pUserContext);
	assert(frame != nullptr);

	frame->OnRender();
}


//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext) {
	return 0;
}

IFrame::EventKeyboard ConvertKeyMessage(UINT nChar, UINT nFlags, int extraMask) {
	int key = 0;
	if (nChar >= VK_F1 && nChar <= VK_F12) {
		key += (nChar - VK_F1) + IFrame::EventKeyboard::KEY_F1;
	} else {
		switch (nChar) {
		case VK_PRIOR:
			key += IFrame::EventKeyboard::KEY_PAGE_UP;
			break;
		case VK_NEXT:
			key += IFrame::EventKeyboard::KEY_PAGE_DOWN;
			break;
		case VK_END:
			key += IFrame::EventKeyboard::KEY_END;
			break;
		case VK_HOME:
			key += IFrame::EventKeyboard::KEY_HOME;
			break;
		case VK_LEFT:
			key += IFrame::EventKeyboard::KEY_LEFT;
			break;
		case VK_UP:
			key += IFrame::EventKeyboard::KEY_UP;
			break;
		case VK_RIGHT:
			key += IFrame::EventKeyboard::KEY_RIGHT;
			break;
		case VK_DOWN:
			key += IFrame::EventKeyboard::KEY_DOWN;
			break;
		case VK_INSERT:
			key += IFrame::EventKeyboard::KEY_INSERT;
			break;
		case VK_DELETE:
			key += IFrame::EventKeyboard::KEY_DELETE;
			break;
		case VK_NUMLOCK:
			key += IFrame::EventKeyboard::KEY_NUM_LOCK;
			break;
		default:
			BYTE state[256];
			WORD code[2];

			if (GetKeyboardState(state)) {
				if (ToAscii(nChar, 0, state, code, 0) == 1)
					nChar = code[0] & 0xFF;
			}
			break;
		}
	}

	if (key == 0)
		key = nChar;

	if (::GetKeyState(VK_MENU) < 0)
		key |= IFrame::EventKeyboard::KEY_ALT;
	if (::GetKeyState(VK_CONTROL) < 0)
		key |= IFrame::EventKeyboard::KEY_CTRL;
	if (::GetKeyState(VK_SHIFT) < 0)
		key |= IFrame::EventKeyboard::KEY_SHIFT;

	if (nChar >= 'A' && nChar <= 'Z' && ((key & IFrame::EventKeyboard::KEY_SHIFT) == IFrame::EventKeyboard::KEY_SHIFT))
		key += 'a' - 'A';

	return IFrame::EventKeyboard(key | extraMask);
}

//--------------------------------------------------------------------------------------
// As a convenience, DXUT inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext) {
	ZFrameDXUT* frame = reinterpret_cast<ZFrameDXUT*>(pUserContext);
	assert(frame != nullptr);
	frame->OnKeyboard(ConvertKeyMessage(nChar, 0, bKeyDown ? 0 : IFrame::EventKeyboard::KEY_POP));
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice(void* pUserContext) {
	ZFrameDXUT* frame = reinterpret_cast<ZFrameDXUT*>(pUserContext);
	assert(frame != nullptr);
	// TODO:
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice(void* pUserContext) {
	ZFrameDXUT* frame = reinterpret_cast<ZFrameDXUT*>(pUserContext);
	assert(frame != nullptr);
	// TODO:
}


ZFrameDXUT::ZFrameDXUT(const Int2& size, IFrame::Callback* cb) : windowSize(size), mainLoopStarted(false) {
	SetCallback(cb);
	static bool inited = false;
	assert(!inited);

	if (!inited) {
		inited = true;
	}
}

void ZFrameDXUT::SetCallback(IFrame::Callback* cb) {
	callback = cb;
}

void ZFrameDXUT::SetWindowTitle(const String& title) {
	windowTitle = title;
	::SetWindowTextW(hWnd, (WCHAR*)Utf8ToSystem(windowTitle).c_str());
}

void ZFrameDXUT::OnMouse(const EventMouse& mouse) {
	if (callback != nullptr) {
		callback->OnMouse(mouse);
	}
}

void ZFrameDXUT::OnRender() {
	if (callback != nullptr) {
		callback->OnRender();
	}
}

void ZFrameDXUT::OnWindowSize(const Int2& newSize) {
	windowSize = newSize;
	if (callback != nullptr) {
		callback->OnWindowSize(newSize);
	}
}

void ZFrameDXUT::OnInitialize(void* param) {
	if (callback != nullptr) {
		callback->OnInitialize(param);
	}
}

void ZFrameDXUT::OnKeyboard(const EventKeyboard& keyboard) {
	if (callback != nullptr) {
		callback->OnKeyboard(keyboard);
	}
}

void ZFrameDXUT::SetWindowSize(const Int2& size) {
	RECT rect;
	::GetWindowRect(hWnd, &rect);
	Int2 center((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
	::SetWindowPos(hWnd, nullptr, center.x() - size.x() / 2, center.y() - size.y() / 2, size.x(), size.y(), 0);
	windowSize = size;
}

const Int2& ZFrameDXUT::GetWindowSize() const {
	return windowSize;
}

void ZFrameDXUT::EnterMainLoop() {
	DXUTSetCallbackD3D9DeviceAcceptable(IsDeviceAcceptable, this);
	DXUTSetCallbackD3D9DeviceCreated(OnCreateDevice, this);
	DXUTSetCallbackD3D9DeviceReset(OnResetDevice, this);
	DXUTSetCallbackD3D9FrameRender(OnFrameRender, this);
	DXUTSetCallbackD3D9DeviceLost(OnLostDevice, this);
	DXUTSetCallbackD3D9DeviceDestroyed(OnDestroyDevice, this);
	DXUTSetCallbackMsgProc(MsgProc, this);
	DXUTSetCallbackKeyboard(KeyboardProc, this);
	DXUTSetCallbackFrameMove(OnFrameMove, this);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings, this);

	DXUTInit(true, true);
	DXUTCreateWindow((WCHAR*)Utf8ToSystem(windowTitle).c_str());
	DXUTCreateDevice(true, (int)windowSize.x(), (int)windowSize.y());


	// Custom main loop
	HWND hWnd = DXUTGetHWND();
	MSG msg;
	msg.message = WM_nullptr;

	mainLoopStarted = true;
	while (::GetMessage(&msg, nullptr, 0, 0)) {
		if (TranslateAccelerator(hWnd, nullptr, &msg) == 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	mainLoopStarted = false;
	// DXUTGetExitCode();
}

void ZFrameDXUT::ExitMainLoop() {
	if (mainLoopStarted) {
		::PostQuitMessage(0);
	}
}

void ZFrameDXUT::FireRender() {
	::InvalidateRect(hWnd, nullptr, FALSE);
}

void ZFrameDXUT::ShowCursor(CURSOR cursor) {
	// Not implemented
}

void ZFrameDXUT::WarpCursor(const Int2& position) {
	::SetCursorPos(position.x(), position.y());
}
