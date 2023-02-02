#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "MyShaders/Public/MySimpleComputeShader.h"

#include "MyFirstComputeShader_TEST.generated.h"


/**
 *
 */
UCLASS()
class STARFIGHT_API UMyFirstComputeShader_TEST : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION()
		static int CALL_TEST_COMPUTE_SHADER(int a, int b);
};
