#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MachineScreenInterface.generated.h"

/**
 * OPTIONAL. A widget can add this interface (Class Settings > Interfaces > Add)
 * without changing its parent class. If it doesn't, everything still works;
 * the widget's normal Event Construct fires each time the screen is shown.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UMachineScreenInterface : public UInterface
{
	GENERATED_BODY()
};

class MACHINEFLOW_API IMachineScreenInterface
{
	GENERATED_BODY()

public:
	/** Called right after this screen becomes visible. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Machine Flow")
	void OnScreenEnter(FName ScreenId, FName PreviousScreenId);

	/** Called right before this screen is hidden. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Machine Flow")
	void OnScreenExit(FName ScreenId, FName NextScreenId);
};
