#include "Portaklon/Portaklon.hpp"

// Sample rate constant - adjust based on your audio setup
#define SAMPLE_RATE 48000
#define BLOCK_SIZE 128

extern "C" {    

void process(float* input, float* output, int num_samples) {
    static Portaklon portaklon;
    static bool firstRun = true;
    if (firstRun) {
        portaklon.init(SAMPLE_RATE, BLOCK_SIZE);
        firstRun = false;
    }

    // Portaklon processes mono audio
    portaklon.process(input, output, num_samples);
}

}