#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class Activity;  // forward declaration

// RAII helper to lock rendering mutex for the duration of a scope.
class RenderLock {
  bool isLocked = false;
  TaskHandle_t ownerTask = nullptr;

 public:
  explicit RenderLock();
  explicit RenderLock(Activity&);  // unused for now, but keep for compatibility
  RenderLock(const RenderLock&) = delete;
  RenderLock& operator=(const RenderLock&) = delete;
  RenderLock(RenderLock&&) = delete;
  RenderLock& operator=(RenderLock&&) = delete;
  ~RenderLock();
  void unlock();
  static bool peek();
};
