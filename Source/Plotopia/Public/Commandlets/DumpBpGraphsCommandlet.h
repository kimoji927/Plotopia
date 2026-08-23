// DumpBpGraphsCommandlet.h
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "DumpBpGraphsCommandlet.generated.h"

/**
 * Editor commandlet: dumps blueprint graphs / widget trees / CDO defaults /
 * DataTable contents for the inventory UI, for diagnosing the "client cannot
 * display item icons" problem.
 *
 * Usage: UnrealEditor-Cmd.exe GAS.uproject -run=DumpBpGraphs
 */
UCLASS()
class PLOTOPIA_API UDumpBpGraphsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
