// ZhouZun


#include "Utils/GAS_BlueprintLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GAS_AttributeSet.h"
#include "Characters/GAS_BaseCharacter.h"
#include "Characters/GAS_EnemyCharacter.h"
#include "Engine/OverlapResult.h"
#include "GameplayTags/GASTags.h"
#include "Kismet/GameplayStatics.h"

EHitDirection UGAS_BlueprintLibrary::GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator)
{
	const float Dot = FVector::DotProduct(TargetForward,ToInstigator);
	if (Dot < -0.5f)
	{
		return EHitDirection::Back;
	}
	if (Dot < 0.5f)
	{
		const FVector Cross = FVector::CrossProduct(ToInstigator,TargetForward);
		if (Cross.Z < 0.f)
		{
			return EHitDirection::Right;
		}
		return EHitDirection::Left;
	}
	return EHitDirection::Forward;
}

FName UGAS_BlueprintLibrary::GetDirectionName(const EHitDirection& HitDirection)
{
	switch (HitDirection)
	{
		case EHitDirection::Back:return FName("Back");
		case EHitDirection::Left:return FName("Left");
		case EHitDirection::Right:return FName("Right");
		case EHitDirection::Forward:return FName("Forward");
		default:return FName("None");
	}
}

FClosestActorWithTagResult UGAS_BlueprintLibrary::FindClosestActorWithTag(UObject* WorldContextObject,const FVector& Origin, const FName& Tag,float SearchRange)
{
	TArray<AActor*> ActorsWithTag;
	//在世界中查找所有带TAG标签的Actor,并将它们填充到一个数组ActorsWithTag中
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject,Tag,ActorsWithTag);
	
	//返回float类型所能表示的最大有限正数值
	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosestActor = nullptr;
	
	//遍历拥有标签的Actor
	for (AActor* Actor : ActorsWithTag)
	{
		if (!IsValid(Actor)) continue;
		AGAS_BaseCharacter* BaseCharacter = Cast<AGAS_BaseCharacter>(Actor);
		if (!IsValid(BaseCharacter) || !BaseCharacter->IsAlive()) continue;
		
		const float Distance = FVector::Dist(Origin,Actor->GetActorLocation());
		if (AGAS_BaseCharacter* SearchingCharacter = Cast<AGAS_BaseCharacter>(WorldContextObject);IsValid(SearchingCharacter))
		{
			if (Distance > SearchingCharacter->SearchRange) continue;
		}
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestActor = Actor;
		}
	}
	
	FClosestActorWithTagResult Result;
	Result.Actor = ClosestActor;
	Result.Distance = ClosestDistance;
	return Result;
}

void UGAS_BlueprintLibrary::SendDamageEventToPlayer(AActor* Target,const TSubclassOf<UGameplayEffect>& DamageEffect,FGameplayEventData& Payload,const FGameplayTag& DataTag,float Damage,const FGameplayTag& EventTagOverride,UObject* OptionalParticleSystem)
{
	AGAS_BaseCharacter* PlayerCharacter = Cast<AGAS_BaseCharacter>(Target);
	if (!IsValid(PlayerCharacter)) return;
	if (!PlayerCharacter->IsAlive()) return;
	
	//约定：Damage 传正数，内部统一取绝对值，避免符号歧义导致死亡误判或回血
	const float ActualDamage = FMath::Abs(Damage);
	
	FGameplayTag EventTag;
	if (!EventTagOverride.MatchesTagExact(GASTags::None))
	{
		EventTag = EventTagOverride;
	}
	else
	{
		UGAS_AttributeSet* AttributeSet = Cast<UGAS_AttributeSet>(PlayerCharacter->GetAttributeSet());
		if (!IsValid(AttributeSet)) return;
	
		const bool bLethal = AttributeSet->GetHealth() - ActualDamage <= 0.0f;
		EventTag = bLethal ? GASTags::GASEvents::Player::Death : GASTags::GASEvents::Player::HitReact;
	}
	
	Payload.OptionalObject = OptionalParticleSystem;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PlayerCharacter,EventTag,Payload);
	
	UAbilitySystemComponent* TargetASC = PlayerCharacter->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)) return;
	
	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect,1.f,ContextHandle);
	
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle,DataTag,-ActualDamage);
	
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UGAS_BlueprintLibrary::SendDamageEventToPlayers(TArray<AActor*> Targets,
	const TSubclassOf<UGameplayEffect>& DamageEffect, FGameplayEventData& Payload, const FGameplayTag& DataTag,
	float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem)
{
	for (AActor* Target : Targets)
	{
		SendDamageEventToPlayer(Target,DamageEffect,Payload,DataTag,Damage,EventTagOverride,OptionalParticleSystem);
	}
}

TArray<AActor*> UGAS_BlueprintLibrary::HitBoxOverlapTest(AActor* AvatarActor,float HitBoxRadius,float HitBoxForwardOffest,float HitBoxElevationOffest,bool bDrawDebugs)
{
	if (!IsValid(AvatarActor)) return TArray<AActor*>();
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(AvatarActor);
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(ActorsToIgnore);
	QueryParams.AddIgnoredActor(AvatarActor);
	
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn,ECR_Block);
	
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(HitBoxRadius);
	
	const FVector Forward = AvatarActor->GetActorForwardVector() * HitBoxForwardOffest;
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + Forward + FVector(0.f,0.f,HitBoxElevationOffest);
	
	UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor,EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return TArray<AActor*>();
	World->OverlapMultiByChannel(OverlapResults, HitBoxLocation, FQuat::Identity, ECC_Visibility,Sphere,QueryParams,ResponseParams);
	
	//碰撞检测到的Actor
	TArray<AActor*> ActorsHit;
	for (const FOverlapResult& Result : OverlapResults)
	{
		AGAS_BaseCharacter* BaseCharacter = Cast<AGAS_BaseCharacter>(Result.GetActor());
		if (!IsValid(BaseCharacter)) continue;
		if (!BaseCharacter->IsAlive()) continue;
		ActorsHit.AddUnique(BaseCharacter);
	}
	
	if (bDrawDebugs)
	{
		DrawHitBoxOverlapDes(AvatarActor,OverlapResults,HitBoxLocation,HitBoxRadius);
	}
	
	return ActorsHit;
}

void UGAS_BlueprintLibrary::DrawHitBoxOverlapDes(const UObject* WorldContextObject,const TArray<FOverlapResult>& OverlapResults,const FVector& HitBoxLocation,float HitBoxRadius)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject,EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return;
	DrawDebugSphere(World,HitBoxLocation,HitBoxRadius,16,FColor::Red,false,3.0f);
		
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (IsValid(Result.GetActor()))
		{
			FVector DebugLocation = Result.GetActor()->GetActorLocation();
			DebugLocation.Z += 100.f;
			DrawDebugSphere(World,DebugLocation,30.f,12,FColor::Green,false,3.0f);
		}
	}
}

TArray<AActor*> UGAS_BlueprintLibrary::ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors, float InnerRadius,float OuterRadius, float LaunchForceMagnitude, float RotationAngle, bool bDrawDebugs)
{
	for (AActor* HitActor : HitActors)
	{
		ACharacter* HitCharacter = Cast<ACharacter>(HitActor);
		if (!IsValid(HitCharacter) || !IsValid(AvatarActor)) return TArray<AActor*>();
		
		const FVector HitCharacterLocation = HitCharacter->GetActorLocation();
		const FVector AvatarLocation = AvatarActor->GetActorLocation();
		
		const FVector ToHitActor = HitCharacterLocation - AvatarLocation;
		const float Distance = FVector::Dist(AvatarLocation,HitCharacterLocation);
		
		float LaunchForce = 0.f;
		if (Distance > OuterRadius) continue;
		if (Distance <= InnerRadius)
		{
			LaunchForce = LaunchForceMagnitude;
		}
		else
		{
			const FVector2D FalloffRange(InnerRadius,OuterRadius);//被击飞的范围
			const FVector2D LaunchForceRange(LaunchForceMagnitude,0.f);//被击飞的力度
			LaunchForce = FMath::GetMappedRangeValueClamped(FalloffRange,LaunchForceRange,Distance);
		}
		if (bDrawDebugs)
		{
			GEngine->AddOnScreenDebugMessage(-1,3.f,FColor::Red,FString::Printf(TEXT("LaunchForce: %f"),LaunchForce));
		}
		
		FVector KnockbackForce = ToHitActor.GetSafeNormal();
		KnockbackForce.Z = 0.f;
		
		const FVector Right = KnockbackForce.RotateAngleAxis(90.f,FVector::UpVector);
		KnockbackForce = KnockbackForce.RotateAngleAxis(-RotationAngle,Right) * LaunchForce;
		
		if (bDrawDebugs)
		{
			UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor,EGetWorldErrorMode::LogAndReturnNull);
			DrawDebugDirectionalArrow(World,HitCharacterLocation,HitCharacterLocation+KnockbackForce,100.f,FColor::Green,false,3.f);
		}
		
		if (AGAS_EnemyCharacter* EnemyCharacter = Cast<AGAS_EnemyCharacter>(HitCharacter);IsValid(EnemyCharacter))
		{
			EnemyCharacter->StopMovementUntilLanded();
		}
		
		HitCharacter->LaunchCharacter(KnockbackForce,true,true);
	}
	return HitActors;
}
