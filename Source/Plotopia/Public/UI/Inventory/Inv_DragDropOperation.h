// Inv_DragDropOperation.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Items/Data/Inv_ItemData.h"
#include "Inv_DragDropOperation.generated.h"

/**
 * Custom DragDropOperation for inventory items
 * Carries complete drag payload via FInv_DragPayload struct
 */
UCLASS()
class PLOTOPIA_API UInv_DragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** Drag payload data (slot index, item instance, icon) */
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	FInv_DragPayload DragPayload;
};