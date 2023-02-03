#pragma once
#include "MyShaders/Public/MySimpleComputeShader.h"
#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "MyFirstComputeShader_TEST.generated.h"


/**
 *
 */
UCLASS()
class STARFIGHT_API UMyFirstComputeShader_TEST : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
		static int CALL_TEST_COMPUTE_SHADER(int a, int b);
};
