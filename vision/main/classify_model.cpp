// Classifier v2: trained int8 model via ESP-DL (C++ library, hence .cpp).
//
// STUB until a trained model exists. vision.c only calls this when classify_model_init()
// succeeds, so the blob classifier (v1) keeps answering until then.
//
// To fill in, once a quantized model (.espdl file) exists:
//   1. Add the esp-dl component to main/idf_component.yml.
//   2. Flash the .espdl file into the "model" partition (partitions.csv).
//   3. classify_model_init(): load it from that partition with ESP-DL's model loader.
//   4. classify_model(): copy rgb into the model's input tensor (quantized the way ESP-PPQ
//      exported it), run the model, take the highest-scoring output as cls and its
//      probability (0-99) as confidence.
#include "classify.h"

bool classify_model_init(void)
{
    return false;
}

bool classify_model(const uint8_t *rgb, classify_result_t *out)
{
    (void)rgb;
    (void)out;
    return false;
}
