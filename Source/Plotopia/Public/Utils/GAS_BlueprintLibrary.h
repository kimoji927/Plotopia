// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GAS_BlueprintLibrary.generated.h"

UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Left,
	Right,
	Forward,
	Back
};

USTRUCT(BlueprintType)
struct FClosestActorWithTagResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> Actor;
	
	UPROPERTY(BlueprintReadWrite)
	float Distance{0.f};
};

UCLASS()
class PLOTOPIA_API UGAS_BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure)
	static EHitDirection GetHitDirection(const FVector& TargetForward,const FVector& ToInstigator);
	
	UFUNCTION(BlueprintPure)
	static FName GetDirectionName(const EHitDirection& HitDirection);
	
	//查找拥有标签的最近的玩家
	UFUNCTION(BlueprintCallable)
	static FClosestActorWithTagResult FindClosestActorWithTag(UObject* WorldContextObject,const FVector& Origin,const FName& Tag,float SearchRange);
	
	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayer(AActor* Target,const TSubclassOf<UGameplayEffect>& DamageEffect,UPARAM(ref) FGameplayEventData& Payload,const FGameplayTag& DataTag,float Damage,const FGameplayTag& EventTagOverride,UObject* OptionalParticleSystem = nullptr);
	
	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayers(TArray<AActor*> Targets,const TSubclassOf<UGameplayEffect>& DamageEffect,UPARAM(ref) FGameplayEventData& Payload,const FGameplayTag& DataTag,float Damage,const FGameplayTag& EventTagOverride,UObject* OptionalParticleSystem = nullptr);
	
	UFUNCTION(BlueprintCallable,Category="GAS|Abilities")
	static TArray<AActor*> HitBoxOverlapTest(AActor* AvatarActor,float HitBoxRadius,float HitBoxForwardOffest = 0.f,float HitBoxElevationOffest = 0.f,bool bDrawDebugs = false);
	
	static void DrawHitBoxOverlapDes(const UObject* WorldContextObject,const TArray<FOverlapResult>& OverlapResults,const FVector& HitBoxLocation,float HitBoxRadius);
	
	UFUNCTION(BlueprintCallable,Category="GAS|Abilities")
	static TArray<AActor*> ApplyKnockback(AActor* AvatarActor,const TArray<AActor*>& HitActors,float InnerRadius,float OuterRadius,float LaunchForceMagnitude,float RotationAngle = 45.f,bool bDrawDebugs = false);
};
