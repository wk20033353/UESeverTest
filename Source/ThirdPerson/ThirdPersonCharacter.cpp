// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "ThirdPersonBullet.h"
#include "Utiltiy/GAssetManager.h"
#include "AbleCore/Classes/ablAbility.h"
#include "AbleCore/Classes/ablAbilityBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GInterActiveComponent.h"

float const Rad2Deg = 57.29578f;
static int64 g_GuidVal = 0;
//////////////////////////////////////////////////////////////////////////
// AThirdPersonCharacter

AThirdPersonCharacter::AThirdPersonCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create able Component
	m_SkillComp = CreateDefaultSubobject<UAblAbilityComponent>("AbleComponent");
	m_SkillComp->SetIsReplicated(true);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	//初始化玩家生命值
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	//初始化投射物类
	ProjectileClass = AThirdPersonBullet::StaticClass();
	//初始化射速
	FireRate = 0.25f;
	bIsFiringWeapon = false;

	//服务器特定的功能
	if (GetLocalRole() == ROLE_Authority)
	{
		Guid = ++g_GuidVal;

		UE_LOG(LogTemp, Warning, TEXT("AThirdPersonCharacter InitGuid %lld"), Guid);
	}
}

//////////////////////////////////////////////////////////////////////////
// 复制的属性
void AThirdPersonCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AThirdPersonCharacter, Guid, COND_InitialOnly);
	//复制当前生命值。
	DOREPLIFETIME(AThirdPersonCharacter, CurrentHealth);
}


//////////////////////////////////////////////////////////////////////////
// Input

void AThirdPersonCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AThirdPersonCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AThirdPersonCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AThirdPersonCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AThirdPersonCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AThirdPersonCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AThirdPersonCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AThirdPersonCharacter::OnResetVR);

	// 处理发射投射物
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AThirdPersonCharacter::StartFire);
}


void AThirdPersonCharacter::OnResetVR()
{
	// If ThirdPerson is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in ThirdPerson.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AThirdPersonCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AThirdPersonCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AThirdPersonCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AThirdPersonCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

//
void AThirdPersonCharacter::OnRep_Guid()
{
	UE_LOG(LogTemp, Error, TEXT("AThirdPersonCharacter::OnRep_Guid %lld!!!"), Guid);
}


void AThirdPersonCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void AThirdPersonCharacter::OnHealthUpdate()
{
	//客户端特定的功能
	if (IsLocallyControlled())
	{
		FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

		if (CurrentHealth <= 0)
		{
			FString deathMessage = FString::Printf(TEXT("You have been killed."));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
		}
	}

	//服务器特定的功能
	if (GetLocalRole() == ROLE_Authority)
	{
		FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
	}

	//在所有机器上都执行的函数。 
	/*
		因任何因伤害或死亡而产生的特殊功能都应放在这里。
	*/
}

void AThirdPersonCharacter::SetCurrentHealth(float healthValue)
{
	//服务器特定的功能
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
		OnHealthUpdate();
	}
}

float AThirdPersonCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(damageApplied);
	return damageApplied;
}

void AThirdPersonCharacter::StartFire()
{
	if (!bIsFiringWeapon)
	{
		bIsFiringWeapon = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().SetTimer(FiringTimer, this, &AThirdPersonCharacter::StopFire, FireRate, false);
		HandleFire();
	}
}

void AThirdPersonCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void AThirdPersonCharacter::HandleFire_Implementation()
{
	FVector spawnLocation = GetActorLocation() + (GetControlRotation().Vector() * 100.0f) + (GetActorUpVector() * 50.0f);
	FRotator spawnRotation = GetControlRotation();

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	AThirdPersonBullet* spawnedProjectile = GetWorld()->SpawnActor<AThirdPersonBullet>(spawnLocation, spawnRotation, spawnParameters);
}



void AThirdPersonCharacter::PlaySkill(const FString& filePath, AActor* Sender, bool bPlayImmediately)
{
	if (filePath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("AGPlayerCharacter::PlaySkill Path is empty!!!"));
		return;
	}

	UGAssetManager::Get()->AsyncLoadBluePrint(filePath, [this, filePath, Sender, bPlayImmediately](UObject* pObj) {
		if (pObj == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("AGPlayerCharacter::PlaySkill No Object: %s"), *filePath);
			return;
		}

		if (bPlayImmediately)
		{
			this->StopSkill(EAblAbilityTaskResult::Interrupted);
		}

		UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(pObj);

		//使用Bp->GetDefaultObject获取UAblAbility，解决在DS下，客户端退出DS的BUG
		UAblAbility* pAbility = Bp->GetDefaultObject<UAblAbility>();

		if (Sender == nullptr)
		{
			UAblAbilityContext* pContext = UAblAbilityBlueprintLibrary::CreateAbilityContext(pAbility, m_SkillComp, GetOwner(), GetOwner());
			m_SkillComp->ActivateAbility(pContext);
		}
		else
		{
			UAblAbilityContext* pContext = UAblAbilityBlueprintLibrary::CreateAbilityContext(pAbility, m_SkillComp, GetOwner(), Sender);
			m_SkillComp->ActivateAbility(pContext);
		}
		});
}

void AThirdPersonCharacter::StopSkill(EAblAbilityTaskResult reason)
{
	UAblAbility* pCurAbility = m_SkillComp->GetActiveAbility();
	if (pCurAbility != nullptr)
	{
		m_SkillComp->CancelAbility(pCurAbility, reason);
	}
}


void AThirdPersonCharacter::ClacIK(float deltaSecs)
{
	USkeletalMeshComponent* RootMeshComponent = GetMesh();
	if (!RootMeshComponent)
	{
		return;
	}

	FName footLName = "foot_l";
	if (!RootMeshComponent->DoesSocketExist(footLName))
	{
		return;
	}

	FName footRName = "foot_r";
	if (!RootMeshComponent->DoesSocketExist(footRName))
	{
		return;
	}

	FVector curLocation = RootMeshComponent->GetComponentLocation();
	FVector footLLocation = RootMeshComponent->GetSocketLocation(footLName);
	FVector footRLocation = RootMeshComponent->GetSocketLocation(footRName);

	float targetLeftZOffset = 0;
	float targetRightZOffset = 0;
	float targetZOffset = 0;
	{
		FVector start(footLLocation.X, footLLocation.Y, curLocation.Z + 50.0f);
		FVector end(footLLocation.X, footLLocation.Y, curLocation.Z - 75.0f);

		FHitResult Hit;
		if (SweapCollisionTrace(GetOwner(), start, end, Hit))
		{
			m_LeftFootHit = true;
			FVector Normal = Hit.Normal;

			float Roll = FMath::Atan2(Normal.Y, Normal.Z) * Rad2Deg;
			Roll = FMath::Clamp(Roll, m_FootRollMin, m_FootRollMax);
			float Pitch = FMath::Atan2(Normal.X, Normal.Z) * Rad2Deg;
			Pitch = -1.0f * FMath::Clamp(Pitch, m_FootPitchMin, m_FootPitchMax);
			m_LeftFootRotOffset = FRotator(Pitch, 0.0f, Roll);

			float ZOffset = Hit.Location.Z - curLocation.Z;
			targetLeftZOffset = FMath::Clamp(ZOffset, m_FootZMin, m_FootZMax);
		}
		else
		{
			m_LeftFootHit = false;
			m_LeftFootRotOffset = FRotator(0.0f, 0.0f, 0.0f);
			targetLeftZOffset = 0.0f;
		}
	}

	{
		FVector start(footRLocation.X, footRLocation.Y, curLocation.Z + 50.0f);
		FVector end(footRLocation.X, footRLocation.Y, curLocation.Z - 75.0f);

		FHitResult Hit;
		if (SweapCollisionTrace(GetOwner(), start, end, Hit))
		{
			m_LeftFootHit = true;
			FVector Normal = Hit.Normal;

			float Roll = FMath::Atan2(Normal.Y, Normal.Z) * Rad2Deg;
			Roll = FMath::Clamp(Roll, m_FootRollMin, m_FootRollMax);
			float Pitch = FMath::Atan2(Normal.X, Normal.Z) * Rad2Deg;
			Pitch = -1.0f * FMath::Clamp(Pitch, m_FootPitchMin, m_FootPitchMax);
			m_RightFootRotOffset = FRotator(Pitch, 0.0f, Roll);

			float ZOffset = Hit.Location.Z - curLocation.Z;
			targetRightZOffset = FMath::Clamp(ZOffset, m_FootZMin, m_FootZMax);
		}
		else
		{
			m_RightFootHit = false;
			m_RightFootRotOffset = FRotator(0.0f, 0.0f, 0.0f);
			targetRightZOffset = 0.0f;
		}
	}

	targetZOffset = FMath::Min(targetRightZOffset, targetLeftZOffset);
	if (targetLeftZOffset < targetRightZOffset)
	{
		targetRightZOffset -= targetLeftZOffset;
		targetLeftZOffset = 0.0f;
	}
	else
	{
		targetLeftZOffset -= targetRightZOffset;
		targetRightZOffset = 0.0f;
	}

	m_LeftFootZOffset = UKismetMathLibrary::FInterpTo(m_LeftFootZOffset, targetLeftZOffset, deltaSecs, 3.0f);
	m_RightFootZOffset = UKismetMathLibrary::FInterpTo(m_RightFootZOffset, targetRightZOffset, deltaSecs, 3.0f);
	m_PelvisZOffset = UKismetMathLibrary::FInterpTo(m_PelvisZOffset, targetZOffset, deltaSecs, 3.0f);
}

bool AThirdPersonCharacter::SweapCollisionTrace(AActor* pActor, const FVector& start, const FVector& end, FHitResult& Hit)
{
	FCollisionShape shape = FCollisionShape();
	FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam;

	FCollisionQueryParams queryParams = FCollisionQueryParams(SCENE_QUERY_STAT(EngineWrapper), true);
	queryParams.AddIgnoredActor(pActor);


	bool bHit = GWorld->LineTraceSingleByChannel(Hit, start, end, ECollisionChannel::ECC_Visibility, queryParams, ResponseParam);

	//GDrawDebugLineTraceSingle(GWorld, start, end, EDrawDebugTrace::ForOneFrame, bHit, Hit, FLinearColor::Red, FLinearColor::Green, 5.0f);

	return bHit;
}

void AThirdPersonCharacter::HandleInteractive_Implementation(UGInterActiveComponent* target, bool bStart)
{
	if (target == nullptr)
	{
		return;
	}

	if (bStart)
	{
		if (!target->CanInteractive(this))
		{
			return;
		}

		//服务器特定的功能
		if (GetLocalRole() == ROLE_Authority)
		{
			UE_LOG(LogTemp, Error, TEXT("UGInterActiveComponent::BeginInteractive Do Server"));
			target->SetInteractGuid(this->GetGuid());
		}
	}
	else
	{
		//服务器特定的功能
		if (GetLocalRole() == ROLE_Authority)
		{
			UE_LOG(LogTemp, Error, TEXT("UGInterActiveComponent::EndInteractive Do Server"));
			target->SetInteractGuid(0);
		}
	}
}

int32 AThirdPersonCharacter::TryCrash()
{
	TArray<int32> Data;
	Data.Add(1);
	Data.Add(2);

	int32 ret = Data[1] + Data[2];
	return ret;
}

int32 AThirdPersonCharacter::TryCheck()
{
	TArray<int32> Data;
	Data.Add(1);
	Data.Add(2);

	check(Data.Num() > 2);
	return Data[0];
}