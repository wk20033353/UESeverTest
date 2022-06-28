// Copyright Epic Games, Inc. All Rights Reserved.

#include "UESeverTestGameMode.h"
#include "UESeverTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUESeverTestGameMode::AUESeverTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
