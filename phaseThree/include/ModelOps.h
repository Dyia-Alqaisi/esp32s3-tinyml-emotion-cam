#pragma once

#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>

namespace ModelOps
{
    // Default broad MobileNetV2 resolver.
    // Run tools/generate_model_ops.py to replace this with the exact resolver.
    constexpr int OPERATOR_COUNT = 14;

    using Resolver =
        tflite::MicroMutableOpResolver<OPERATOR_COUNT>;

    inline Resolver createResolver()
    {
        Resolver resolver;

        resolver.AddAdd();
        resolver.AddAveragePool2D();
        resolver.AddConv2D();
        resolver.AddDepthwiseConv2D();
        resolver.AddFullyConnected();
        resolver.AddMean();
        resolver.AddMul();
        resolver.AddPad();
        resolver.AddPadV2();
        resolver.AddQuantize();
        resolver.AddRelu();
        resolver.AddRelu6();
        resolver.AddReshape();
        resolver.AddSoftmax();

        return resolver;
    }
}
