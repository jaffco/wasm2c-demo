#include "MarshallModel.h"

// Sample rate constant - adjust based on your audio setup
#define SAMPLE_RATE 48000

extern "C" {

void process(float* input, float* output, int num_samples) {
    static bool initialized = false;
    static MarshallModelWeights mWeights;
    static wavenet::RTWavenet<1, 128, Layer1, Layer2> mModel;
    // Lazy initialization on first call
    if (!initialized) {
        mModel.loadModel(mWeights.weights);
        initialized = true;
    }
    
    // Process the entire block at once using the model's block-based forward method
    mModel.model.forward(input, output, num_samples);
}

} // extern "C"