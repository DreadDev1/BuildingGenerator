//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * 
 */
class DUNGEONARCHITECTRUNTIME_API PMRandom {
public:
    PMRandom();
    void Init(int32 seed);
    float NextGaussianFloat();
    float NextGaussianFloat(float mean, float stdDev);
    float GetNextUniformFloat();

private:
    FRandomStream random;
};

