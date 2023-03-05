// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MapInfoAsset.generated.h"

/**
 * 
 */

USTRUCT(Blueprintable, BlueprintType)
struct FMapInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoftObjectPath PrimaryAssetPath;

	UPROPERTY()
	class UPrimaryAssetLabel* PrimaryAssetLabel;

	FString GetLevelName() const;
	FText GetLevelDescription() const;
	FString GetLevelReference();
};

UCLASS()
class FPSWITHDLC_API UMapInfoBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="MapInfo|Methods")
	static FString GetLevelReference(UPARAM(ref) FMapInfo& MapInfoRef)
	{
		return MapInfoRef.GetLevelReference();
	}
};

UCLASS(Blueprintable, BlueprintType)
class FPSWITHDLC_API UMapInfoAsset : public UObject
{
	GENERATED_BODY()

public:

	UMapInfoAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
	{
		
	}
	
	UPROPERTY(EditAnywhere)
	FMapInfo MapInfo;
};
