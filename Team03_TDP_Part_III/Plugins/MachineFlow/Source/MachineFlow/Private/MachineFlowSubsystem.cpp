#include "MachineFlowSubsystem.h"

#include "MachineFlowSettings.h"
#include "MachineScreenInterface.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogMachineFlow, Log, All);

namespace
{
	void NotifyEnter(UUserWidget* Widget, FName Id, FName Prev)
	{
		if (Widget && Widget->GetClass()->ImplementsInterface(UMachineScreenInterface::StaticClass()))
		{
			IMachineScreenInterface::Execute_OnScreenEnter(Widget, Id, Prev);
		}
	}

	void NotifyExit(UUserWidget* Widget, FName Id, FName Next)
	{
		if (Widget && Widget->GetClass()->ImplementsInterface(UMachineScreenInterface::StaticClass()))
		{
			IMachineScreenInterface::Execute_OnScreenExit(Widget, Id, Next);
		}
	}
}

void UMachineFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Widgets belong to a world; drop them before a level change so we never hold dead ones.
	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UMachineFlowSubsystem::HandlePreLoadMap);
}

void UMachineFlowSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
	ResetState();
	Super::Deinitialize();
}

void UMachineFlowSubsystem::HandlePreLoadMap(const FString& /*MapName*/)
{
	ResetState();
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

void UMachineFlowSubsystem::OpenMachine(APlayerController* PlayerController, FName StartScreen)
{
	if (!PlayerController)
	{
		PlayerController = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	}
	if (!PlayerController)
	{
		UE_LOG(LogMachineFlow, Warning, TEXT("OpenMachine: no PlayerController available."));
		return;
	}

	if (OwningPC.Get() != PlayerController)
	{
		ResetState();
	}

	if (bIsOpen)
	{
		// Already open: just jump to the requested screen.
		if (!StartScreen.IsNone())
		{
			GoTo(StartScreen);
		}
		return;
	}

	OwningPC = PlayerController;
	bIsOpen = true;
	History.Reset();

	const FName Start = StartScreen.IsNone() ? GetDefault<UMachineFlowSettings>()->HomeScreen : StartScreen;
	if (!ShowScreen(Start))
	{
		UE_LOG(LogMachineFlow, Warning, TEXT("OpenMachine: could not show start screen '%s'."), *Start.ToString());
		CloseMachine();
		return;
	}

	OnMachineOpened.Broadcast();
}

void UMachineFlowSubsystem::CloseMachine()
{
	if (!bIsOpen)
	{
		return;
	}

	if (UUserWidget* Current = FindCachedWidget(CurrentScreen))
	{
		NotifyExit(Current, CurrentScreen, NAME_None);
	}

	RemoveAllScreens();
	WidgetCache.Reset();   // next open starts with fresh widgets
	History.Reset();
	CurrentScreen = NAME_None;
	bIsOpen = false;

	if (GetDefault<UMachineFlowSettings>()->bClearSessionOnClose)
	{
		Session.Reset();
	}

	OnMachineClosed.Broadcast();
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

bool UMachineFlowSubsystem::GoTo(FName ScreenId)
{
	if (ScreenId.IsNone())
	{
		return false;
	}

	if (!bIsOpen)
	{
		OpenMachine(nullptr, ScreenId);
		return bIsOpen && CurrentScreen == ScreenId;
	}

	if (ScreenId == CurrentScreen)
	{
		return true;
	}

	const FName Previous = CurrentScreen;
	if (!ShowScreen(ScreenId))
	{
		return false;
	}

	if (!Previous.IsNone())
	{
		History.Push(Previous);
	}
	return true;
}

bool UMachineFlowSubsystem::Back()
{
	if (!bIsOpen || History.Num() == 0)
	{
		return false;
	}

	const FName Target = History.Pop();
	if (!ShowScreen(Target))
	{
		History.Push(Target);
		return false;
	}
	return true;
}

bool UMachineFlowSubsystem::Home()
{
	return ResetTo(GetDefault<UMachineFlowSettings>()->HomeScreen, false);
}

bool UMachineFlowSubsystem::ResetTo(FName ScreenId, bool bClearSession)
{
	if (ScreenId.IsNone())
	{
		return false;
	}

	if (bClearSession)
	{
		Session.Reset();
	}

	if (!bIsOpen)
	{
		OpenMachine(nullptr, ScreenId);
		return bIsOpen;
	}

	History.Reset();
	return ScreenId == CurrentScreen || ShowScreen(ScreenId);
}

void UMachineFlowSubsystem::RegisterScreen(FName ScreenId, TSubclassOf<UUserWidget> WidgetClass)
{
	if (ScreenId.IsNone() || !WidgetClass)
	{
		UE_LOG(LogMachineFlow, Warning, TEXT("RegisterScreen: invalid ID or class."));
		return;
	}
	RuntimeScreens.Add(ScreenId, WidgetClass);
}

// ---------------------------------------------------------------------------
// Session data
// ---------------------------------------------------------------------------

void UMachineFlowSubsystem::SetSessionValue(FName Key, const FString& Value)
{
	Session.Add(Key, Value);
}

bool UMachineFlowSubsystem::GetSessionValue(FName Key, FString& Value) const
{
	if (const FString* Found = Session.Find(Key))
	{
		Value = *Found;
		return true;
	}
	Value.Reset();
	return false;
}

void UMachineFlowSubsystem::SetSessionNumber(FName Key, float Value)
{
	Session.Add(Key, FString::SanitizeFloat(Value));
}

bool UMachineFlowSubsystem::GetSessionNumber(FName Key, float& Value) const
{
	if (const FString* Found = Session.Find(Key))
	{
		Value = FCString::Atof(**Found);
		return true;
	}
	Value = 0.f;
	return false;
}

void UMachineFlowSubsystem::ClearSession()
{
	Session.Reset();
}

// ---------------------------------------------------------------------------
// Internals
// ---------------------------------------------------------------------------

TSubclassOf<UUserWidget> UMachineFlowSubsystem::ResolveScreenClass(FName ScreenId) const
{
	if (const TSubclassOf<UUserWidget>* Runtime = RuntimeScreens.Find(ScreenId))
	{
		return *Runtime;
	}

	const UMachineFlowSettings* Settings = GetDefault<UMachineFlowSettings>();
	if (const TSoftClassPtr<UUserWidget>* Soft = Settings->Screens.Find(ScreenId))
	{
		return Soft->LoadSynchronous();
	}
	return nullptr;
}

UUserWidget* UMachineFlowSubsystem::FindCachedWidget(FName ScreenId) const
{
	if (ScreenId.IsNone())
	{
		return nullptr;
	}
	const TObjectPtr<UUserWidget>* Found = WidgetCache.Find(ScreenId);
	return (Found && IsValid(*Found)) ? Found->Get() : nullptr;
}

bool UMachineFlowSubsystem::ShowScreen(FName ScreenId)
{
	APlayerController* PC = OwningPC.Get();
	if (!PC)
	{
		return false;
	}

	UUserWidget* Next = FindCachedWidget(ScreenId);
	if (!Next)
	{
		const TSubclassOf<UUserWidget> WidgetClass = ResolveScreenClass(ScreenId);
		if (!WidgetClass)
		{
			UE_LOG(LogMachineFlow, Warning,
				TEXT("Screen '%s' is not registered. Add it in Project Settings > Game > Machine Flow, or call RegisterScreen."),
				*ScreenId.ToString());
			return false;
		}

		Next = CreateWidget<UUserWidget>(PC, WidgetClass);
		if (!Next)
		{
			return false;
		}
		WidgetCache.Add(ScreenId, Next);
	}

	const FName Previous = CurrentScreen;
	if (UUserWidget* Old = FindCachedWidget(Previous))
	{
		NotifyExit(Old, Previous, ScreenId);
		Old->RemoveFromParent();
	}

	Next->AddToViewport(GetDefault<UMachineFlowSettings>()->ZOrder);
	CurrentScreen = ScreenId;

	NotifyEnter(Next, ScreenId, Previous);
	OnScreenChanged.Broadcast(Previous, ScreenId);
	return true;
}

void UMachineFlowSubsystem::RemoveAllScreens()
{
	for (const TPair<FName, TObjectPtr<UUserWidget>>& Pair : WidgetCache)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->RemoveFromParent();
		}
	}
}

void UMachineFlowSubsystem::ResetState()
{
	RemoveAllScreens();
	WidgetCache.Reset();
	History.Reset();
	CurrentScreen = NAME_None;
	bIsOpen = false;
	OwningPC.Reset();
}
