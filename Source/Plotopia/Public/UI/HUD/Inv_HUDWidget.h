// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inv_HUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class PLOTOPIA_API UInv_HUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent,Category="Inventory")
	void ShowPickUpMessage(const FString& Message);
	
	UFUNCTION(BlueprintImplementableEvent,Category="Inventory")
	void HidePickUpMessage();
};
