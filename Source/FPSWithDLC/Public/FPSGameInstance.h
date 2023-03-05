// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MapInfoAsset.h"
#include "FPSGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class FPSWITHDLC_API UFPSGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	class UDLCLoader* DLCLoader;

public:
	UFUNCTION(BlueprintCallable)
	TArray<FMapInfo> GetMapsInfo();

protected:
	virtual void Init() override;
};
