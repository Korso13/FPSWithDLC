// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSGameInstance.h"

#include "DLCLoader.h"

TArray<FMapInfo> UFPSGameInstance::GetMapsInfo()
{
	return DLCLoader->GetMapsInfo();
}

void UFPSGameInstance::Init()
{
	Super::Init();

	DLCLoader = NewObject<UDLCLoader>(this, "DLC Loader");
}
