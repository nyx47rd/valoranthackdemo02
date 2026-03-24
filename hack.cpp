// =============================================================================
// VALORANT_ADVANCED_FRAMEWORK.H - Professional Game Manipulation Framework
// =============================================================================

#pragma once

#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <DirectXMath.h>
#include <dwmapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <span>
#include <expected>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <future>
#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numbers>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dwmapi.lib")

// =============================================================================
// 1. CORE MATHEMATICS & 3D ENGINE
// =============================================================================

namespace Math {
Â  Â  using namespace DirectX;
Â  Â Â 
Â  Â  struct Vector2 {
Â  Â  Â  Â  float x, y;
Â  Â  Â  Â  Vector2(float _x = 0, float _y = 0) : x(_x), y(_y) {}
Â  Â  Â  Â  float Length() const { return std::sqrt(x*x + y*y); }
Â  Â  Â  Â  Vector2 Normalize() const { float l = Length(); return l > 0 ? Vector2(x/l, y/l) : Vector2(); }
Â  Â  };

Â  Â  struct Vector3 {
Â  Â  Â  Â  float x, y, z;
Â  Â  Â  Â  Vector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
Â  Â  Â  Â Â 
Â  Â  Â  Â  Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
Â  Â  Â  Â  Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
Â  Â  Â  Â  Vector3 operator*(float s) const { return {x*s, y*s, z*s}; }
Â  Â  Â  Â Â 
Â  Â  Â  Â  float Length() const { return std::sqrt(x*x + y*y + z*z); }
Â  Â  Â  Â  Vector3 Normalize() const { float l = Length(); return l > 0 ? *this * (1.0f/l) : Vector3(); }
Â  Â  Â  Â  float Dot(const Vector3& o) const { return x*o.x + y*o.y + z*o.z; }
Â  Â  Â  Â  Vector3 Cross(const Vector3& o) const {
Â  Â  Â  Â  Â  Â  return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x};
Â  Â  Â  Â  }
Â  Â  };
Â  Â Â 
Â  Â  struct Matrix4x4 {
Â  Â  Â  Â  float m[4][4];
Â  Â  Â  Â  Matrix4x4() { memset(m, 0, sizeof(m)); m[0][0]=m[1][1]=m[2][2]=m[3][3]=1.0f; }
Â  Â  Â  Â Â 
Â  Â  Â  Â  Vector3 TransformPosition(const Vector3& pos) const {
Â  Â  Â  Â  Â  Â  return {
Â  Â  Â  Â  Â  Â  Â  Â  pos.x*m[0][0] + pos.y*m[0][1] + pos.z*m[0][2] + m[0][3],
Â  Â  Â  Â  Â  Â  Â  Â  pos.x*m[1][0] + pos.y*m[1][1] + pos.z*m[1][2] + m[1][3],
Â  Â  Â  Â  Â  Â  Â  Â  pos.x*m[2][0] + pos.y*m[2][1] + pos.z*m[2][2] + m[2][3]
Â  Â  Â  Â  Â  Â  };
Â  Â  Â  Â  }
Â  Â  };

Â  Â  // SIMD-optimized vector operations
Â  Â  inline Vector3 SIMD_Add(const Vector3& a, const Vector3& b) {
Â  Â  Â  Â  // In production, use __m128 intrinsics here
Â  Â  Â  Â  return a + b;
Â  Â  }

Â  Â  inline bool WorldToScreen(const Vector3& worldPos, const Matrix4x4& viewMatrix,Â 
Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â const Matrix4x4& projMatrix, float screenWidth, float screenHeight,
Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â Vector3& screenPos) {
Â  Â  Â  Â  Vector3 viewPos = viewMatrix.TransformPosition(worldPos);
Â  Â  Â  Â Â 
Â  Â  Â  Â  float w = viewPos.x * projMatrix.m[3][0] + viewPos.y * projMatrix.m[3][1] +Â 
Â  Â  Â  Â  Â  Â  Â  Â  Â  viewPos.z * projMatrix.m[3][2] + projMatrix.m[3][3];
Â  Â  Â  Â Â 
Â  Â  Â  Â  if (w < 0.001f) return false;
Â  Â  Â  Â Â 
Â  Â  Â  Â  screenPos.x = (viewPos.x * projMatrix.m[0][0] + viewPos.y * projMatrix.m[0][1] +Â 
Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â viewPos.z * projMatrix.m[0][2] + projMatrix.m[0][3]) / w;
Â  Â  Â  Â  screenPos.y = (viewPos.x * projMatrix.m[1][0] + viewPos.y * projMatrix.m[1][1] +Â 
Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â viewPos.z * projMatrix.m[1][2] + projMatrix.m[1][3]) / w;
Â  Â  Â  Â Â 
Â  Â  Â  Â  screenPos.x = (screenPos.x + 1.0f) * 0.5f * screenWidth;
Â  Â  Â  Â  screenPos.y = (1.0f - screenPos.y) * 0.5f * screenHeight;
Â  Â  Â  Â Â 
Â  Â  Â  Â  return screenPos.x >= 0 && screenPos.x <= screenWidth &&Â 
Â  Â  Â  Â  Â  Â  Â  Â screenPos.y >= 0 && screenPos.y <= screenHeight;
Â  Â  }
}

// =============================================================================
// 2. ADVANCED MEMORY ENGINE
// =============================================================================

class ThreadSafeMemoryEngine {
private:
Â  Â  HANDLE process_handle_;
Â  Â  DWORD process_id_;
Â  Â  std::atomic<bool> connected_;
Â  Â  mutable std::shared_mutex cache_mutex_;
Â  Â Â 
Â  Â  struct CacheEntry {
Â  Â  Â  Â  std::vector<uint8_t> data;
Â  Â  Â  Â  std::chrono::steady_clock::time_point timestamp;
Â  Â  Â  Â  std::atomic<size_t> access_count{0};
Â  Â  };
Â  Â Â 
Â  Â  std::unordered_map<uintptr_t, CacheEntry> memory_cache_;
Â  Â  std::queue<uintptr_t> cache_queue_;
Â  Â  static constexpr size_t MAX_CACHE_SIZE = 10000;
Â  Â  static constexpr auto CACHE_TTL = std::chrono::seconds(30);
Â  Â Â 
Â  Â  // Thread pool
Â  Â  std::vector<std::jthread> worker_threads_;
Â  Â  std::queue<std::function<void()>> task_queue_;
Â  Â  std::mutex queue_mutex_;
Â  Â  std::condition_variable queue_condition_;
Â  Â  std::atomic<bool> shutdown_{false};
Â  Â Â 
Â  Â  // Rate limiting
Â  Â  std::atomic<size_t> request_count_{0};
Â  Â  std::chrono::steady_clock::time_point rate_limit_reset_;
Â  Â  static constexpr size_t RATE_LIMIT = 1000; // requests per second

public:
Â  Â  ThreadSafeMemoryEngine() : process_handle_(INVALID_HANDLE_VALUE), process_id_(0), connected_(false) {
Â  Â  Â  Â  rate_limit_reset_ = std::chrono::steady_clock::now();
Â  Â  Â  Â  // Start worker threads
Â  Â  Â  Â  unsigned int num_threads = std::thread::hardware_concurrency();
Â  Â  Â  Â  for (unsigned int i = 0; i < num_threads; ++i) {
Â  Â  Â  Â  Â  Â  worker_threads_.emplace_back([this] { WorkerThread(); });
Â  Â  Â  Â  }
Â  Â  }
Â  Â Â 
Â  Â  ~ThreadSafeMemoryEngine() noexcept {
Â  Â  Â  Â  shutdown_ = true;
Â  Â  Â  Â  queue_condition_.notify_all();
Â  Â  Â  Â  // jthreads automatically join
Â  Â  }
Â  Â Â 
Â  Â  // RAII Handle Wrapper
Â  Â  class ScopedHandle {
Â  Â  Â  Â  HANDLE handle_;
Â  Â  public:
Â  Â  Â  Â  explicit ScopedHandle(HANDLE h = INVALID_HANDLE_VALUE) noexcept : handle_(h) {}
Â  Â  Â  Â  ~ScopedHandle() noexcept { if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }
Â  Â  Â  Â  ScopedHandle(const ScopedHandle&) = delete;
Â  Â  Â  Â  ScopedHandle& operator=(const ScopedHandle&) = delete;
Â  Â  Â  Â  ScopedHandle(ScopedHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = INVALID_HANDLE_VALUE; }
Â  Â  Â  Â  HANDLE get() const noexcept { return handle_; }
Â  Â  Â  Â  bool is_valid() const noexcept { return handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr; }
Â  Â  };

Â  Â  std::expected<bool, std::string> Attach(const std::string& processName) {
Â  Â  Â  Â  if (connected_) return true;
Â  Â  Â  Â Â 
Â  Â  Â  Â  DWORD pid = FindProcessIdByName(processName);
Â  Â  Â  Â  if (pid == 0) return std::unexpected("Process not found");
Â  Â  Â  Â Â 
Â  Â  Â  Â  // Security checks
Â  Â  Â  Â  if (!ValidateProcess(pid)) {
Â  Â  Â  Â  Â  Â  return std::unexpected("Process validation failed");
Â  Â  Â  Â  }
Â  Â  Â  Â Â 
Â  Â  Â  Â  process_handle_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
Â  Â  Â  Â  if (!process_handle_) {
Â  Â  Â  Â  Â  Â  return std::unexpected("Failed to open process: " + std::to_string(GetLastError()));
Â  Â  Â  Â  }
Â  Â  Â  Â Â 
Â  Â  Â  Â  process_id_ = pid;
Â  Â  Â  Â  connected_ = true;
Â  Â  Â  Â  return true;
Â  Â  }
Â  Â Â 
Â  Â  void Detach() noexcept {
Â  Â  Â  Â  std::unique_lock lock(cache_mutex_);
Â  Â  Â  Â  if (process_handle_ != INVALID_HANDLE_VALUE) {
Â  Â  Â  Â  Â  Â  CloseHandle(process_handle_);
Â  Â  Â  Â  Â  Â  process_handle_ = INVALID_HANDLE_VALUE;
Â  Â  Â  Â  }
Â  Â  Â  Â  memory_cache_.clear();
Â  Â  Â  Â  while (!cache_queue_.empty()) cache_queue_.pop();
Â  Â  Â  Â  connected_ = false;
Â  Â  }

Â  Â  template<typename T>
Â  Â  std::expected<T, std::string> Read(uintptr_t address) {
Â  Â  Â  Â  if (!connected_) return std::unexpected("Not connected");
Â  Â  Â  Â Â 
Â  Â  Â  Â  // Rate limiting
Â  Â  Â  Â  if (!CheckRateLimit()) return std::unexpected("Rate limit exceeded");
Â  Â  Â  Â Â 
Â  Â  Â  Â  // Check cache first
Â  Â  Â  Â  {
Â  Â  Â  Â  Â  Â  std::shared_lock lock  
