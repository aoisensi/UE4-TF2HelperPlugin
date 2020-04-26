#pragma once

#include "CoreMinimal.h"

struct FTF2AimMatrixDirection
{
public:
	FName Name;
	int32 AliveFrame;
	FVector SampleValue;
	UAnimSequence* Asset;
};