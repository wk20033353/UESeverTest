// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GInterActiveComponent.generated.h"

class AThirdPersonCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THIRDPERSON_API UGInterActiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGInterActiveComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
		void BeginInteractive(AThirdPersonCharacter* player);
	UFUNCTION(BlueprintCallable)
		void EndInteractive(AThirdPersonCharacter* player);
	UFUNCTION(BlueprintCallable)
		bool CanInteractive(AThirdPersonCharacter* player);

	UFUNCTION(BlueprintCallable)
		void SetInteractGuid(int64 guid) { m_nCurInteractGuid = guid; };

	FVector GetOwnerLocation();

private:
	/** RepNotify，用于同步InteractGuid */
	UFUNCTION()
		void OnReq_CurInteractGuid();

protected:
	TWeakObjectPtr<USceneComponent> OwnerRootComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InterActive")
		float Distance = 150;

	/** 当前交互中玩家的唯一标识*/
	UPROPERTY(replicatedUsing = OnReq_CurInteractGuid)
		uint64 m_nCurInteractGuid;
};
