#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include <d3d11.h>
#include <dxgi.h>

bool g_ImGuiInitialized = false;
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND g_hGameWnd = nullptr;

inline void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

static void CreateRenderTarget(IDXGISwapChain* SwapChain) {
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void InitImGui(IDXGISwapChain* SwapChain) {
    if (g_ImGuiInitialized || !SwapChain) return;

    if (FAILED(SwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
        return;
    }

    g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);

    DXGI_SWAP_CHAIN_DESC sd;
    if (FAILED(SwapChain->GetDesc(&sd))) {
        return;
    }
    g_hGameWnd = sd.OutputWindow;

    CreateRenderTarget(SwapChain);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(g_hGameWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    g_ImGuiInitialized = true;
}

extern "C" int PresentHandler(IDXGISwapChain* SwapChain, UINT SyncInterval, UINT Flags) { // Note that this hooks a METHOD, so swapchain is the first argument
	// 1. Dispatch raw OnPresent to mods
	for (uint8_t priority = 0; priority <= 4; priority += 1) {
		for (DLL* dll : modDLLs) {
			if (dll && dll->mod && dll->mod->OnPresentPriority == (GenericMod::Priority)priority) {
				dll->mod->OnPresent(SwapChain, SyncInterval, Flags);
			}
		}
	}

	bool hasActiveImGuiMods = false;
	for (DLL* dll : modDLLs) {
		if (dll && dll->mod && dll->enabled) {
			hasActiveImGuiMods = true;
			break;
		}
	}

	// 2. Initialize Dear ImGui on first Present call if mods are active
	if (hasActiveImGuiMods && !g_ImGuiInitialized && SwapChain) {
		InitImGui(SwapChain);
	}

	// 3. Render Dear ImGui frame and dispatch OnDrawImGui() to mods
	if (hasActiveImGuiMods && g_ImGuiInitialized && g_pd3dDeviceContext && g_mainRenderTargetView) {
		ID3D11RenderTargetView* oldRTV = nullptr;
		ID3D11DepthStencilView* oldDSV = nullptr;
		g_pd3dDeviceContext->OMGetRenderTargets(1, &oldRTV, &oldDSV);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		for (uint8_t priority = 0; priority <= 4; priority += 1) {
			for (DLL* dll : modDLLs) {
				if (dll && dll->mod && dll->enabled && dll->mod->OnDrawImGuiPriority == (GenericMod::Priority)priority) {
					dll->mod->OnDrawImGui();
				}
			}
		}

		ImGui::Render();
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Restore previous render targets so game engine state is not clobbered
		g_pd3dDeviceContext->OMSetRenderTargets(1, &oldRTV, oldDSV);
		if (oldRTV) oldRTV->Release();
		if (oldDSV) oldDSV->Release();
	}

	return 0;
}

GETTER_VAR(void*, ASM_PresentHandler_jmpback);
extern "C" void ASM_PresentHandler();

void SetupPresentHandler() {
	WriteFarJMP(Offset(base, 0x134743), (void*)&ASM_PresentHandler);
	ASM_PresentHandler_jmpback = Offset(base, 0x134751);
}
