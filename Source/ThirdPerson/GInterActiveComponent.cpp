// Fill out your copyright notice in the Description page of Project Settings.
#include "GInterActiveComponent.h"
#include "ThirdPersonCharacter.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UGInterActiveComponent::UGInterActiveComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	//SetIsReplicated(true);
}


// Called when the game starts
void UGInterActiveComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if (GetOwner())
	{
		OwnerRootComponent = GetOwner()->GetRootComponent();
	}
}


// Called every frame
void UGInterActiveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGInterActiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGInterActiveComponent, m_nCurInteractGuid);
}

void UGInterActiveComponent::OnReq_CurInteractGuid()
{
	UE_LOG(LogTemp, Warning, TEXT("UGInteractiveComponent::OnReq_CurInteractGuid %lld"), m_nCurInteractGuid);
}


void UGInterActiveComponent::BeginInteractive(AThirdPersonCharacter* player)
{
	if (GetOwner() == nullptr || player == nullptr)
	{
		return;
	}

	player->HandleInteractive(this, true);
}

void UGInterActiveComponent::EndInteractive(AThirdPersonCharacter* player)
{
	if (GetOwner() == nullptr || player == nullptr)
	{
		return;
	}

	player->HandleInteractive(this, false);
}

bool UGInterActiveComponent::CanInteractive(AThirdPersonCharacter* player)
{
	if (player == nullptr)
	{
		return false;
	}

	if (m_nCurInteractGuid != 0)
	{
		return false;
	}

	float dis = FVector::Distance(OwnerRootComponent->GetComponentLocation(), player->GetActorLocation());
	return dis < Distance;
}


FVector UGInterActiveComponent::GetOwnerLocation()
{
	if (!OwnerRootComponent.IsValid())
	{
		return FVector::ZeroVector;
	}

	return OwnerRootComponent->GetComponentLocation();
}
