// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GAS_MeleeAttack.generated.h"

/**
 * 
 */
UCLASS()
class PLOTOPIA_API UGAS_MeleeAttack : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	
private:
	UPROPERTY(EditAnywhere,Category="GAS|Debugs")
	bool bDrawDebugs = false;
	
	UPROPERTY(EditAnywhere,Category="GAS|Socket")
	FName SocketName{"FX_Trail_01_R"};
	
	UPROPERTY(EditAnywhere,Category="GAS|Socket")
	float SocketExtensionOffest{40.f};
	
	UPROPERTY(EditAnywhere,Category="GAS|Socket")
	float SphereTraceRadius{60.f};
	
	TArray<FHitResult> PerformSphereTrace(USkeletalMeshComponent* MeshComp) const;
	void SendEventsToActors(USkeletalMeshComponent* MeshComp,const TArray<FHitResult>& Hits) const;
};
