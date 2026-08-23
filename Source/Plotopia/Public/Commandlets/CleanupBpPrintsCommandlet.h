// CleanupBpPrintsCommandlet.h
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CleanupBpPrintsCommandlet.generated.h"

/**
 * Editor commandlet: removes all debug PrintString/PrintText/PrintWarning/PrintError
 * nodes from the project's widget blueprints (inventory UI), rewires the execution
 * flow around them, prunes orphaned helper nodes, then recompiles and saves.
 *
 * Usage: UnrealEditor-Cmd.exe GAS.uproject -run=CleanupBpPrints
 */
UCLASS()
class PLOTOPIA_API UCleanupBpPrintsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
