#include "../libDaisy/src/daisy_seed.h"
#include "SDRAM.hpp"

// wasm2c Runtime Headers
#include "wasm-module.h"

// Faust base classes needed for generated code
class Meta {
public:
    virtual void declare(const char* key, const char* value) = 0;
    virtual ~Meta() {}
};

class UI {
public:
    virtual void openHorizontalBox(const char* label) = 0;
    virtual void openVerticalBox(const char* label) = 0;
    virtual void closeBox() = 0;
    virtual void declare(float* zone, const char* key, const char* val) = 0;
    virtual void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step) = 0;
    virtual void addNumEntry(const char* label, float* zone, float init, float min, float max, float step) = 0;
    virtual ~UI() {}
};

class dsp {
public:
    virtual ~dsp() {}
    virtual int getNumInputs() = 0;
    virtual int getNumOutputs() = 0;
    virtual void init(int sample_rate) = 0;
    virtual void compute(int count, float** inputs, float** outputs) = 0;
    virtual void buildUserInterface(UI* ui_interface) = 0;
};

#include "../wasm-module/springreverb.cpp"

using namespace daisy;
static DaisySeed hardware;

// Global SDRAM allocator instance
static Jaffx::SDRAM sdram;

// wasm2c runtime engine
w2c_module wasm_module;

// Native DSP instance
mydsp* mDSP;

// Macro for halting on errors
#define ERROR_HALT while (true) {}

// Macro for block size
#define BLOCK_SIZE 128

// Macro for enabling audio
// #define RUN_AUDIO

// Macro for using native impl for audio
// #define NATIVE_AUDIO

// C wrapper functions for wasm2c platform to use SDRAM
extern "C" {
    void* sdram_alloc(size_t size) {
        return sdram.malloc(size);
    }
    
    void sdram_dealloc(void* ptr) {
        if (ptr) sdram.free(ptr);
    }
    
    void* sdram_realloc(void* ptr, size_t size) {
        return sdram.realloc(ptr, size);
    }
    
    void* sdram_calloc(size_t nmemb, size_t size) {
        return sdram.calloc(nmemb, size);
    }
}

class Timer {
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

// Error handler for WASM runtime
void os_print_last_error(const char* msg) {
  hardware.PrintLine(msg);
  ERROR_HALT
}

// Wrapper to handle memory management between host and WASM linear memory
// WASM functions receive u32 offsets into linear memory, not host pointers
void wasm2c_module_process(w2c_module* instance, const float* input, float* output, size_t size) {
  // Get WASM linear memory
  wasm_rt_memory_t* mem = w2c_module_memory(instance);

  // Use fixed offsets in WASM memory for buffers
  // WASM memory is 64KB, stack is 8KB. Use safe offsets in the data region.
  const u32 INPUT_OFFSET = 16384;  // 16KB offset for input buffer
  const u32 OUTPUT_OFFSET = 32768; // 32KB offset for output buffer

  // Copy input to WASM memory
  float* wasm_input = (float*)(mem->data + INPUT_OFFSET);
  memcpy(wasm_input, input, size * sizeof(float));

  // Call the WASM process function with memory offsets
  w2c_module_process(instance, INPUT_OFFSET, OUTPUT_OFFSET, (u32)size);

  // Copy output from WASM memory
  float* wasm_output = (float*)(mem->data + OUTPUT_OFFSET);
  memcpy(output, wasm_output, size * sizeof(float));
}

bool InitWasm2c() {
  // Initialize WASM runtime
  hardware.PrintLine("Initializing wasm2c Runtime...");
  wasm_rt_init();
  hardware.PrintLine("wasm2c runtime initialized.");

  hardware.PrintLine("Instantiating wasm2c module...");
  wasm2c_module_instantiate(&wasm_module);
  hardware.PrintLine("wasm2c initialized successfully!");
  return true;
}

// Audio callback using buffer-based wasm2c processing
static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {

  #ifndef NATIVE_AUDIO
  // Process the entire buffer at once using the wasm2c wrapper
  wasm2c_module_process(&wasm_module, in[0], out[0], size);
  #endif

  #ifdef NATIVE_AUDIO
  // Native audio processing
  float* inBuff = const_cast<float*>(in[0]);
  float* outBuff = out[0];
  mDSP->compute(size, &inBuff, &outBuff);
  #endif

  // Copy left channel to right channel for stereo output
  memcpy(out[1], out[0], size * sizeof(float));
}

int main() {
  hardware.Init();
  hardware.StartLog(true); // wait for serial connection

  System::Delay(200);
  hardware.PrintLine("===========================================");
  hardware.PrintLine("         wasm2c Demo - Daisy Wrapper       ");
  hardware.PrintLine("===========================================");
  hardware.PrintLine("");

  // Initialize SDRAM allocator
  hardware.PrintLine("Initializing SDRAM allocator...");
  sdram.init();
  hardware.PrintLine("SDRAM initialized (64MB at 0xC0000000)");
  hardware.PrintLine("");   

  // use a placement new to construct the DSP object in SDRAM
  void* dsp_memory = sdram.malloc(sizeof(mydsp));
  if (!dsp_memory) {
    hardware.PrintLine("FATAL: Unable to allocate memory for DSP object in SDRAM");
    ERROR_HALT
  }
  mDSP = new (dsp_memory) mydsp();
  mDSP->init(48000); // Initialize DSP at 48kHz sample rate

  // Initialize wasm2c runtime
  if (!InitWasm2c()) {
    hardware.PrintLine("FATAL: wasm2c initialization failed");
    ERROR_HALT
  }  

  hardware.PrintLine("=== Testing Process Function ===");

  // Generate a few test samples with different inputs
  hardware.PrintLine("Calling process() with various inputs...");
  for (int i = 0; i < 5; i++) {
    float input = (float)i * 0.1f;  // 0.0, 0.1, 0.2, 0.3, 0.4
    
    // prepare and process single-sample buffers
    float input_buffer[1] = {input};
    float output_buffer[1] = {0.0f};
    wasm2c_module_process(&wasm_module, input_buffer, output_buffer, 1);
    float output = output_buffer[0];

    hardware.PrintLine("  process(" FLT_FMT3 ") = " FLT_FMT3, FLT_VAR3(input), FLT_VAR3(output));
  }  

  hardware.PrintLine("");
  hardware.PrintLine("=== Running Performance Benchmarks ===");
  System::Delay(100);  

  // Benchmark configuration
  const int WARMUP_RUNS = 10;
  const int BENCHMARK_RUNS = 100;
    
  // Warmup phase
  hardware.PrintLine("");
  hardware.PrintLine("[WARMUP] Running %d warmup iterations...", WARMUP_RUNS);
  volatile float warmup_result_wasm = 0.0f;
  volatile float warmup_result_native = 0.0f;
  for (int i = 0; i < WARMUP_RUNS; i++) {
    // prepare and process single-sample buffers
    float input = daisy::Random::GetFloat(-1.f, 1.f);
    float input_buffer[1] = {input};
    float output_buffer_wasm[1] = {0.0f};
    float output_buffer_native[1] = {0.0f};
    
    // WASM warmup
    wasm2c_module_process(&wasm_module, input_buffer, output_buffer_wasm, 1);
    warmup_result_wasm += output_buffer_wasm[0];
    
    // Native warmup
    float* in_ptr = input_buffer;
    float* out_ptr = output_buffer_native;
    mDSP->compute(1, &in_ptr, &out_ptr);
    warmup_result_native += output_buffer_native[0];
  }
  hardware.PrintLine("[OK] Warmup complete (WASM result=" FLT_FMT3 ", Native result=" FLT_FMT3 ")", FLT_VAR3(warmup_result_wasm), FLT_VAR3(warmup_result_native));
  
  // Benchmark phase
  hardware.PrintLine("");
  hardware.PrintLine("[BENCHMARK] Running %d iterations...", BENCHMARK_RUNS);
  
  // WASM timing variables
  float total_us_wasm = 0.0f;
  float min_us_wasm = 1e9f;
  float max_us_wasm = 0.0f;
  float total_ticks_wasm = 0.0f;
  float min_ticks_wasm = 1e9f;
  float max_ticks_wasm = 0.0f;
  volatile float checksum_wasm = 0.0f;
  
  // Native timing variables
  float total_us_native = 0.0f;
  float min_us_native = 1e9f;
  float max_us_native = 0.0f;
  float total_ticks_native = 0.0f;
  float min_ticks_native = 1e9f;
  float max_ticks_native = 0.0f;
  volatile float checksum_native = 0.0f;
  
  for (int i = 0; i < BENCHMARK_RUNS; i++) {

    // prepare buffers
    float input_buffer[BLOCK_SIZE];
    float output_buffer_wasm[BLOCK_SIZE];
    float output_buffer_native[BLOCK_SIZE];
    for (int j = 0; j < BLOCK_SIZE; j++) {
      input_buffer[j] = daisy::Random::GetFloat(-1.f, 1.f);
      output_buffer_wasm[j] = 0.f;
      output_buffer_native[j] = 0.f;
    }

    // Benchmark WASM
    Timer timer_wasm;
    timer_wasm.start();
    wasm2c_module_process(&wasm_module, input_buffer, output_buffer_wasm, BLOCK_SIZE);
    timer_wasm.end();
    
    float elapsed_us_wasm = timer_wasm.usElapsed();
    float elapsed_ticks_wasm = (float)timer_wasm.ticksElapsed();
    
    total_us_wasm += elapsed_us_wasm;
    total_ticks_wasm += elapsed_ticks_wasm;
    
    if (elapsed_us_wasm < min_us_wasm) min_us_wasm = elapsed_us_wasm;
    if (elapsed_us_wasm > max_us_wasm) max_us_wasm = elapsed_us_wasm;
    if (elapsed_ticks_wasm < min_ticks_wasm) min_ticks_wasm = elapsed_ticks_wasm;
    if (elapsed_ticks_wasm > max_ticks_wasm) max_ticks_wasm = elapsed_ticks_wasm;
    
    // Benchmark Native
    Timer timer_native;
    float* in_ptr = input_buffer;
    float* out_ptr = output_buffer_native;
    timer_native.start();
    mDSP->compute(BLOCK_SIZE, &in_ptr, &out_ptr);
    timer_native.end();
    
    float elapsed_us_native = timer_native.usElapsed();
    float elapsed_ticks_native = (float)timer_native.ticksElapsed();
    
    total_us_native += elapsed_us_native;
    total_ticks_native += elapsed_ticks_native;
    
    if (elapsed_us_native < min_us_native) min_us_native = elapsed_us_native;
    if (elapsed_us_native > max_us_native) max_us_native = elapsed_us_native;
    if (elapsed_ticks_native < min_ticks_native) min_ticks_native = elapsed_ticks_native;
    if (elapsed_ticks_native > max_ticks_native) max_ticks_native = elapsed_ticks_native;
    
    // use checksums to prevent optimization
    for (int j = 0; j < BLOCK_SIZE; j++) {
      checksum_wasm += output_buffer_wasm[j];
      checksum_native += output_buffer_native[j];
    }  
  }
  
  float avg_us_wasm = total_us_wasm / BENCHMARK_RUNS;
  float avg_ticks_wasm = total_ticks_wasm / BENCHMARK_RUNS;
  float avg_us_native = total_us_native / BENCHMARK_RUNS;
  float avg_ticks_native = total_ticks_native / BENCHMARK_RUNS;
    
  hardware.PrintLine("");
  hardware.PrintLine("=== BENCHMARK RESULTS ===");
  hardware.PrintLine("Iterations: %d", BENCHMARK_RUNS);
  hardware.PrintLine("");

  hardware.PrintLine("WASM Implementation:");
  hardware.PrintLine("  Average:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(avg_us_wasm), (int)avg_ticks_wasm);
  hardware.PrintLine("  Minimum:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(min_us_wasm), (int)min_ticks_wasm);
  hardware.PrintLine("  Maximum:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(max_us_wasm), (int)max_ticks_wasm);
  hardware.PrintLine("  Checksum:   " FLT_FMT3, FLT_VAR3(checksum_wasm));
  hardware.PrintLine("");

  hardware.PrintLine("Native Implementation:");
  hardware.PrintLine("  Average:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(avg_us_native), (int)avg_ticks_native);
  hardware.PrintLine("  Minimum:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(min_us_native), (int)min_ticks_native);
  hardware.PrintLine("  Maximum:    " FLT_FMT3 " us (%d ticks)", FLT_VAR3(max_us_native), (int)max_ticks_native);
  hardware.PrintLine("  Checksum:   " FLT_FMT3, FLT_VAR3(checksum_native));
  hardware.PrintLine("");
  
  hardware.PrintLine("Comparison:");
  float speedup = avg_us_native / avg_us_wasm;
  hardware.PrintLine("  WASM is " FLT_FMT3 "x slower than Native", FLT_VAR3(speedup));
  hardware.PrintLine("  Checksum difference: " FLT_FMT3 " (should be ~0)", FLT_VAR3(checksum_wasm - checksum_native));
    
  // Calculate real-time performance for both
  float samples_per_us_wasm = (float)BLOCK_SIZE / avg_us_wasm;
  float samples_per_sec_wasm = samples_per_us_wasm * 1000000.0f;
  float realtime_factor_48k_wasm = samples_per_sec_wasm / 48000.0f;
  
  float samples_per_us_native = (float)BLOCK_SIZE / avg_us_native;
  float samples_per_sec_native = samples_per_us_native * 1000000.0f;
  float realtime_factor_48k_native = samples_per_sec_native / 48000.0f;
    
  hardware.PrintLine("");
  hardware.PrintLine("=== REAL-TIME ANALYSIS ===");
  hardware.PrintLine("Sample rate: 48000 Hz");
  hardware.PrintLine("WASM Throughput:   " FLT_FMT3 " samples/sec (" FLT_FMT3 "x real-time)", FLT_VAR3(samples_per_sec_wasm), FLT_VAR3(realtime_factor_48k_wasm));
  hardware.PrintLine("Native Throughput: " FLT_FMT3 " samples/sec (" FLT_FMT3 "x real-time)", FLT_VAR3(samples_per_sec_native), FLT_VAR3(realtime_factor_48k_native));
    
  hardware.PrintLine("");
  if (realtime_factor_48k_wasm >= 1.0f) {
    hardware.PrintLine("WASM: CAN run in REAL-TIME! OK");
  } else {
    hardware.PrintLine("WASM: Too slow for real-time X");
  }
  if (realtime_factor_48k_native >= 1.0f) {
    hardware.PrintLine("Native: CAN run in REAL-TIME! OK");
  } else {
    hardware.PrintLine("Native: Too slow for real-time X");
  }
    
  hardware.PrintLine("");
  hardware.PrintLine("[SUCCESS] WASM vs Native benchmark complete!");

  // =============================================
  // IMPULSE RESPONSE TEST (10 seconds)
  // =============================================
  hardware.PrintLine("");
  hardware.PrintLine("=== IMPULSE RESPONSE TEST ===");
  hardware.PrintLine("Processing 10 seconds of audio at 48kHz");
  hardware.PrintLine("Firing an impulse (1.0) every second");
  hardware.PrintLine("");

  const int SAMPLE_RATE = 48000;
  const int TOTAL_SECONDS = 10;
  const int TOTAL_SAMPLES = SAMPLE_RATE * TOTAL_SECONDS;
  const int TOTAL_BLOCKS = TOTAL_SAMPLES / BLOCK_SIZE;

  // Allocate output storage in SDRAM
  float* impulse_outputs = (float*)sdram.malloc(TOTAL_SAMPLES * sizeof(float));
  if (!impulse_outputs) {
    hardware.PrintLine("[ERROR] Failed to allocate impulse output buffer!");
    ERROR_HALT
  }
  hardware.PrintLine("Allocated %d samples in SDRAM", TOTAL_SAMPLES);

  // Process all blocks
  for (int block = 0; block < TOTAL_BLOCKS; block++) {
    float input_buffer[BLOCK_SIZE];
    float output_buffer[BLOCK_SIZE];

    // Fill with silence
    for (int j = 0; j < BLOCK_SIZE; j++) {
      input_buffer[j] = 0.0f;
      output_buffer[j] = 0.0f;
    }

    // Fire impulse at the start of each second
    int sample_offset = block * BLOCK_SIZE;
    if (sample_offset % SAMPLE_RATE == 0) {
      input_buffer[0] = 1.0f;
      hardware.PrintLine("  Impulse at second %d (sample %d)", sample_offset / SAMPLE_RATE, sample_offset);
    }

    // Process through WASM module
    wasm2c_module_process(&wasm_module, input_buffer, output_buffer, BLOCK_SIZE);

    // Store outputs
    for (int j = 0; j < BLOCK_SIZE; j++) {
      impulse_outputs[sample_offset + j] = output_buffer[j];
    }
  }

  hardware.PrintLine("");
  hardware.PrintLine("Processing complete. Logging %d samples...", TOTAL_SAMPLES);
  hardware.PrintLine("");

  // Print all samples with demarcator lines between seconds
  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    if (i % SAMPLE_RATE == 0) {
      hardware.PrintLine("========== SECOND %d ==========", i / SAMPLE_RATE);
    }
    // Scale float to integer for precise serial transmission (5 decimal places)
    int scaled = (int)(impulse_outputs[i] * 100000.0f);
    hardware.PrintLine("S:%d", scaled);
  }

  hardware.PrintLine("=== IMPULSE RESPONSE TEST COMPLETE ===");

  // Free the SDRAM allocation
  sdram.free(impulse_outputs);

  hardware.PrintLine("Test Complete!");
  hardware.SetLed(true);
  System::Delay(200);

  #ifndef RUN_AUDIO
  // Prepare for next test
  System::ResetToBootloader(System::BootloaderMode::DAISY_INFINITE_TIMEOUT);
  #endif

  // Start Audio
  hardware.SetAudioBlockSize(BLOCK_SIZE); // number of samples handled per callback (buffer size)
	hardware.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ); // sample rate
  hardware.StartAudio(AudioCallback);
  return 0;
}