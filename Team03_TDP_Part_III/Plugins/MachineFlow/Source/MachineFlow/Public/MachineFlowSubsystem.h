#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MachineFlowSubsystem.generated.h"

class UUserWidget;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMachineScreenChanged, FName, FromScreen, FName, ToScreen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMachineOpenStateChanged);

/**
 * Owns navigation between ticketing-machine screens.
 *
 * Lives on the GameInstance, so it doesn't belong to any level: it works the same in a
 * teammate's private test map and in the shared main map. In any Blueprint, right-click
 * and search "Get MachineFlowSubsystem" to use it.
 */
UCLASS()
class MACHINEFLOW_API UMachineFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---------- Open / close ----------

	/** Show the machine UI. Does not touch input, cursor or the player; that stays with your existing setup.
	 *  PlayerController may be left empty (uses player 0).
	 *  StartScreen empty = HomeScreen from Project Settings. */
	UFUNCTION(BlueprintCallable, Category = "Machine Flow")
	void OpenMachine(APlayerController* PlayerController, FName StartScreen);

	/** Hide all machine screens. */
	UFUNCTION(BlueprintCallable, Category = "Machine Flow")
	void CloseMachine();

	UFUNCTION(BlueprintPure, Category = "Machine Flow")
	bool IsMachineOpen() const { return bIsOpen; }

	// ---------- Navigation ----------

	/** Go to a screen. If the machine isn't open yet, opens it (handy for debug keys). */
	UFUNCTION(BlueprintCallable, Category = "Machine Flow")
	bool GoTo(FName ScreenId);

	/** Return to the previous screen. Returns false if there is nothing to go back to. */
	UFUNCTION(BlueprintCallable, Category = "Machine Flow")
	bool Back();

	/** Clear history and return to HomeScreen. */
	UFUNCTION(BlueprintCallable, Category = "Machine Flow")
	bool Home();

	UFUNCTION(BlueprintPure, Category = "Machine Flow")
	FName GetCurrentScreen() const { return CurrentScreen; }

	/** Register a screen at runtime instead of in Project Settings.
	 *  Lets someone test their screen in their own map without touching shared files. */
	UFUNCTION(BlueprintCallable, Category = "Machine Flow")
	void RegisterScreen(FName ScreenId, TSubclassOf<UUserWidget> WidgetClass);

	// ---------- Shared session data (passed between screens/systems) ----------

	UFUNCTION(BlueprintCallable, Category = "Machine Flow|Session")
	void SetSessionValue(FName Key, const FString& Value);

	/** Returns false if the key was never set. */
	UFUNCTION(BlueprintPure, Category = "Machine Flow|Session")
	bool GetSessionValue(FName Key, FString& Value) const;

	UFUNCTION(BlueprintCallable, Category = "Machine Flow|Session")
	void SetSessionNumber(FName Key, float Value);

	/** Returns false if the key was never set. */
	UFUNCTION(BlueprintPure, Category = "Machine Flow|Session")
	bool GetSessionNumber(FName Key, float& Value) const;

	UFUNCTION(BlueprintCallable, Category = "Machine Flow|Session")
	void ClearSession();

	// ---------- Events ----------

	UPROPERTY(BlueprintAssignable, Category = "Machine Flow")
	FOnMachineScreenChanged OnScreenChanged;

	UPROPERTY(BlueprintAssignable, Category = "Machine Flow")
	FOnMachineOpenStateChanged OnMachineOpened;

	UPROPERTY(BlueprintAssignable, Category = "Machine Flow")
	FOnMachineOpenStateChanged OnMachineClosed;

private:
	bool ShowScreen(FName ScreenId);
	TSubclassOf<UUserWidget> ResolveScreenClass(FName ScreenId) const;
	UUserWidget* FindCachedWidget(FName ScreenId) const;
	void RemoveAllScreens();
	void ResetState();
	void HandlePreLoadMap(const FString& MapName);

	TWeakObjectPtr<APlayerController> OwningPC;

	/** Screens created so far. Kept alive while the machine is open so state survives Back(). */
	UPROPERTY()
	TMap<FName, TObjectPtr<UUserWidget>> WidgetCache;

	UPROPERTY()
	TMap<FName, TSubclassOf<UUserWidget>> RuntimeScreens;

	TArray<FName> History;
	FName CurrentScreen = NAME_None;
	bool bIsOpen = false;

	TMap<FName, FString> Session;

	FDelegateHandle PreLoadMapHandle;
};
