#include "MarshallModel.h"

// Sample rate constant - adjust based on your audio setup
#define SAMPLE_RATE 48000

extern "C" {

void process(float* input, float* output, int num_samples) {
    static bool initialized = false;
    static MarshallModelWeights mWeights;
    static wavenet::RTWavenet<1, 1, Layer1, Layer2> mModel;
    // Lazy initialization on first call
    if (!initialized) {
        mModel.loadModel(mWeights.weights);
        initialized = true;
    }
    
    // Process samples one at a time using the underlying model's forward method
    for (int i = 0; i < num_samples; i++) {
        output[i] = mModel.model.forward(input[i]);
    }
}

} // extern "C"