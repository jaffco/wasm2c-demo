#include "../libDaisy/src/daisy_seed.h"
#include "wasm-app.h"

using namespace daisy;

class JaffxTimer {
private:
  bool mDone = false;
  unsigned int mStartTime = 0;
  unsigned int mEndTime = 0;
  unsigned int mTickFreq = 0; 

public:
  void start() {  
    mTickFreq = System::GetTickFreq();
    mStartTime = System::GetTick();
  }

  void end() {
    mEndTime = System::GetTick();
    mDone = true;
  }

  unsigned int ticksElapsed() {
    if (!mDone) {
      return 0;
    }
    return mEndTime - mStartTime;
  }

  float usElapsed() {
    if (!mDone) {
      return 0.f;
    }
    float ticksElapsed = float(mEndTime - mStartTime);
    return (ticksElapsed * 1e6f) / mTickFreq;
  }
};

static DaisySeed hardware;
w2c_app wasm_app;

// Error handler for WASM runtime
void os_print_last_error(const char* msg) {
  hardware.PrintLine(msg);
  while(1) { System::Delay(100); }
}

int main() {
  hardware.Init();
  hardware.StartLog(true);

  System::Delay(200);
  hardware.PrintLine("===========================================");
  hardware.PrintLine("    WASM Audio Processing on Daisy Seed   ");
  hardware.PrintLine("===========================================");
  hardware.PrintLine("");

  // Initialize WASM runtime
  hardware.PrintLine("Initializing WebAssembly...");
  wasm_rt_init();
  hardware.PrintLine("WASM runtime initialized.");

  hardware.PrintLine("Instantiating WASM module...");
  wasm2c_app_instantiate(&wasm_app);
  hardware.PrintLine("WASM initialized successfully!");

  // Test the WASM function
  hardware.PrintLine("Testing WASM process function...");
  float test_in = 0.5f;
  float test_out = w2c_app_process(&wasm_app, test_in);
  hardware.PrintLine("Test: input=" FLT_FMT3 ", output=" FLT_FMT3, FLT_VAR3(test_in), FLT_VAR3(test_out));

  // BENCHMARKING BELOW

  // Benchmark configuration
  const int WARMUP_RUNS = 10;
  const int BENCHMARK_RUNS = 48000;  // 1 second at 48kHz
    
  // Warmup phase to stabilize caches and branch prediction
  hardware.PrintLine("");
  hardware.PrintLine("[WARMUP] Running %d warmup iterations...", WARMUP_RUNS);
  volatile float warmup_result = 0.0f;  // Prevent optimization
  for (int i = 0; i < WARMUP_RUNS; i++) {
    float a = (float)(daisy::Random::GetFloat() * 2.0f - 1.0f);  // -1.0 to 1.0
    warmup_result += w2c_app_process(&wasm_app, a);
  }
  hardware.PrintLine("[OK] Warmup complete (result=" FLT_FMT3 ")", FLT_VAR3(warmup_result));
  
  // Benchmark phase with random inputs
  hardware.PrintLine("");
  hardware.PrintLine("[BENCHMARK] Running %d iterations...", BENCHMARK_RUNS);
  
  float total_us = 0.0f;
  float min_us = 1e9f;
  float max_us = 0.0f;
  float total_ticks = 0.0f;
  float min_ticks = 1e9f;
  float max_ticks = 0.0f;
  volatile float checksum = 0.0f;  // Prevent optimization
  
  for (int i = 0; i < BENCHMARK_RUNS; i++) {
    JaffxTimer timer;
    
    // Use random audio samples in range -1.0 to 1.0
    float a = (float)(daisy::Random::GetFloat() * 2.0f - 1.0f);
    
    timer.start();
    float result = w2c_app_process(&wasm_app, a);
    timer.end();
    
    float elapsed_us = timer.usElapsed();
    float elapsed_ticks = (float)timer.ticksElapsed();
    
    total_us += elapsed_us;
    total_ticks += elapsed_ticks;
    
    if (elapsed_us < min_us) min_us = elapsed_us;
    if (elapsed_us > max_us) max_us = elapsed_us;
    if (elapsed_ticks < min_ticks) min_ticks = elapsed_ticks;
    if (elapsed_ticks > max_ticks) max_ticks = elapsed_ticks;
    
    // Use result to prevent dead code elimination
    checksum += result;
  }
  
  float avg_us = total_us / BENCHMARK_RUNS;
  float avg_ticks = total_ticks / BENCHMARK_RUNS;
  
  hardware.PrintLine("");
  hardware.PrintLine("=== BENCHMARK RESULTS ===");
  hardware.PrintLine("Iterations: %d", BENCHMARK_RUNS);
  hardware.PrintLine("Average:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(avg_us), (int)avg_ticks);
  hardware.PrintLine("Minimum:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(min_us), (int)min_ticks);
  hardware.PrintLine("Maximum:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(max_us), (int)max_ticks);
  hardware.PrintLine("Checksum:   " FLT_FMT3 " (prevents optimization)", FLT_VAR3(checksum));
  
  // Verify correctness with known values
  float verify_in = 0.5f;
  float verify_out = w2c_app_process(&wasm_app, verify_in);
  hardware.PrintLine("");
  hardware.PrintLine("Verification: input=" FLT_FMT3 ", output=" FLT_FMT3, FLT_VAR3(verify_in), FLT_VAR3(verify_out));
  hardware.PrintLine("[SUCCESS] WASM2C benchmark complete!");  


  hardware.PrintLine("Test Complete!");
  hardware.SetLed(true);

  // Prepare for next test
  System::Delay(200);
  System::ResetToBootloader(System::BootloaderMode::DAISY_INFINITE_TIMEOUT);
  return 0;
}