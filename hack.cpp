#include <windows.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>
#include <string>
#include <cfloat>
#include <cmath>

// =============================================================================
// 1. KERNEL PROTOKOLÜ (Düzeltilmiş Struct: Buffer-Safe)
// =============================================================================
#define IOCTL_READ_VM CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// METHOD_BUFFERED kullanıldığı için Response artık bir pointer değil, 
// doğrudan verinin kopyalanacağı adresin kendisi gibi davranır.
struct KERNEL_READ_REQUEST {
    ULONG ProcessId;
    ULONGLONG Address;
    SIZE_T Size;
    BYTE Data[1024]; // Statik buffer ile kernel-user arası veri güvenliği
};

struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };

// =============================================================================
// 2. GÜVENLİ BELLEK YÖNETİMİ (RAII ve Chain Desteği)
// =============================================================================
class MemoryManager {
private:
    HANDLE hDriver;
    DWORD pid;
    uintptr_t baseAddress;

    void Cleanup() {
        if (hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(hDriver);
            hDriver = INVALID_HANDLE_VALUE;
        }
    }

public:
    MemoryManager() : hDriver(INVALID_HANDLE_VALUE), pid(0), baseAddress(0) {}
    ~MemoryManager() { Cleanup(); }

    bool Init(const char* procName) {
        hDriver = CreateFileA("\\\\.\\MyKernelDriver", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hDriver == INVALID_HANDLE_VALUE) return false;

        pid = GetPid(procName);
        if (!pid) { Cleanup(); return false; }

        baseAddress = GetModule(procName);
        if (!baseAddress) { Cleanup(); return false; }

        return true;
    }

    // Gerçek Veri Pipeline'ı: Başarısızlık durumunda bool döner
    bool ReadRaw(uintptr_t addr, void* buffer, size_t size) {
        if (!addr || !buffer || size > 1024) return false;

        KERNEL_READ_REQUEST req = { pid, addr, size };
        DWORD bytes;

        if (DeviceIoControl(hDriver, IOCTL_READ_VM, &req, sizeof(req), &req, sizeof(req), &bytes, NULL)) {
            // Kernel'in yazdığı veriyi user-buffer'a kopyala
            memcpy(buffer, req.Data, size);
            return true;
        }
        return false;
    }

    // Güvenli Şablon: Başarı durumunu referansla döner
    template <typename T>
    bool Read(uintptr_t addr, T& value) {
        return ReadRaw(addr, &value, sizeof(T));
    }

    // Multi-Level Pointer Chain Çözücü
    uintptr_t ReadChain(uintptr_t base, const std::vector<uintptr_t>& offsets) {
        uintptr_t current = base;
        for (size_t i = 0; i < offsets.size(); i++) {
            if (!Read<uintptr_t>(current + offsets[i], current)) return 0;
        }
        return current;
    }

    uintptr_t GetBase() { return baseAddress; }
};

// =============================================================================
// 3. OPTİMİZE EDİLMİŞ MATEMATİK MOTORU
// =============================================================================
class MathEngine {
public:
    // Düzeltilmiş W2S ve Matrix Layout (Row-Major Varsayımı)
    bool WorldToScreen(const Vector3& pos, Vector2& screen, float* matrix, int w, int h) {
        float clipW = matrix[3] * pos.x + matrix[7] * pos.y + matrix[11] * pos.z + matrix[15];
        
        if (clipW < 0.01f) return false;

        float ndcX = (matrix[0] * pos.x + matrix[4] * pos.y + matrix[8] * pos.z + matrix[12]) / clipW;
        float ndcY = (matrix[1] * pos.x + matrix[5] * pos.y + matrix[9] * pos.z + matrix[13]) / clipW;

        screen.x = (w / 2.0f) * (1.0f + ndcX);
        screen.y = (h / 2.0f) * (1.0f - ndcY);

        return (screen.x >= 0 && screen.x <= w && screen.y >= 0 && screen.y <= h);
    }

    // Ağır powf/sqrtf fonksiyonlarından arındırılmış mesafe hesabı
    inline float GetDistSq(Vector2 p1, Vector2 p2) {
        float dx = p1.x - p2.x;
        float dy = p1.y - p2.y;
        return (dx * dx) + (dy * dy);
    }
};

// =============================================================================
// 4. ANA SİSTEM (Data Pipeline & Fail-Safe)
// =============================================================================
int main() {
    MemoryManager mem;
    MathEngine math;

    if (!mem.Init("VALORANT-Win64-Shipping.exe")) return -1;

    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    Vector2 screenCenter = { (float)sw / 2.0f, (float)sh / 2.0f };

    while (!(GetAsyncKeyState(VK_F10) & 0x8000)) {
        float matrix[16];
        // 5 Seviyeli Pointer Chain Örneği
        uintptr_t matrixPtr = mem.ReadChain(mem.GetBase(), { 0x5000000, 0x120, 0x8, 0x48, 0x20 });
        
        if (!matrixPtr || !mem.ReadRaw(matrixPtr, matrix, sizeof(matrix))) {
            Sleep(10); continue;
        }

        uintptr_t entityList = mem.ReadChain(mem.GetBase(), { 0x6000000, 0x28 });
        if (!entityList) continue;

        float closestDistSq = FLT_MAX;
        Vector2 bestTarget = { 0, 0 };

        for (int i = 0; i < 64; i++) {
            uintptr_t player;
            if (!mem.Read<uintptr_t>(entityList + (i * 0x8), player) || !player) continue;

            // Filtreleme Zinciri
            int health, team, isDormant;
            if (!mem.Read(player + 0x320, health) || health <= 0) continue;
            if (!mem.Read(player + 0x3E0, team) || team == 1) continue; // Dinamik teamID çekilmeli
            if (mem.Read(player + 0x100, isDormant) && isDormant) continue;

            Vector3 headPos;
            if (!mem.Read(player + 0x1B0, headPos)) continue;

            Vector2 screenPos;
            if (math.WorldToScreen(headPos, screenPos, matrix, sw, sh)) {
                float dSq = math.GetDistSq(screenPos, screenCenter);
                
                // Dinamik FOV (Çözünürlük bağımsız: ekran genişliğinin %10'u)
                float dynamicFovSq = (sw * 0.1f) * (sw * 0.1f);

                if (dSq < closestDistSq && dSq < dynamicFovSq) {
                    closestDistSq = dSq;
                    bestTarget = screenPos;
                }
            }
        }

        // Yumuşatılmış (Lerp-Like) Aim Hareketi
        if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) && closestDistSq != FLT_MAX) {
            float smooth = 6.5f; // Zaman bazlı (delta time) eklenmesi önerilir
            int moveX = static_cast<int>((bestTarget.x - screenCenter.x) / smooth);
            int moveY = static_cast<int>((bestTarget.y - screenCenter.y) / smooth);
            // mouse_move(moveX, moveY);
        }
        
        Sleep(1); // Yüksek yenileme hızı ve düşük CPU yükü dengesi
    }
    return 0;
}
