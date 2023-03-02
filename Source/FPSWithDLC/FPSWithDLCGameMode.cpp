// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSWithDLCGameMode.h"
#include "FPSWithDLCHUD.h"
#include "FPSWithDLCCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFPSWithDLCGameMode::AFPSWithDLCGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AFPSWithDLCHUD::StaticClass();
}
