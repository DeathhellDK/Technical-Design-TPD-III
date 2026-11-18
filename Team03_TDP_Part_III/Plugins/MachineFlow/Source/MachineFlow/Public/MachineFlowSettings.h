#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Blueprint/UserWidget.h"
#include "MachineFlowSettings.generated.h"

/**
 * Project Settings > Game > Machine Flow
 *
 * Maps a screen ID (e.g. "TopUp_SelectAmount") to the widget that should be shown.
 * Saved to Config/DefaultGame.ini, a plain text file, so source control can merge it
 * (unlike a .umap or Data Table asset).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Machine Flow"))
class MACHINEFLOW_API UMachineFlowSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Screen shown when the machine opens and when Home() is called. */
	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	FName HomeScreen = TEXT("MainMenu");

	/** Screen ID -> widget class. Any UUserWidget works; no special parent class needed. */
	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TMap<FName, TSoftClassPtr<UUserWidget>> Screens;

	/** Viewport Z-order for machine screens (higher draws on top of the game HUD). */
	UPROPERTY(Config, EditAnywhere, Category = "Display")
	int32 ZOrder = 10;

	/** Wipe the shared session values (amount, card, etc.) whenever the machine closes. */
	UPROPERTY(Config, EditAnywhere, Category = "Session")
	bool bClearSessionOnClose = true;
};
