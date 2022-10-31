// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbleCore/Classes/ablAbilityComponent.h"
#include "ThirdPersonCharacter.generated.h"

class UGInterActiveComponent;

UCLASS(config=Game)
class THIRDPERSON_API AThirdPersonCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AThirdPersonCharacter();

	/** ���Ը��� */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


public:
	/** ��ҵ�Ψһ��ʶ */
	int64 GetGuid() { return Guid; };

	/** �������ֵ��ȡֵ������*/
	UFUNCTION(BlueprintPure, Category = "Health")
		FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** ��ǰ����ֵ��ȡֵ������*/
	UFUNCTION(BlueprintPure, Category = "Health")
		FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	/** ��ǰ����ֵ�Ĵ�ֵ����������ֵ�ķ�Χ�޶���0��MaxHealth֮�䣬������OnHealthUpdate�����ڷ������ϵ��á�*/
	UFUNCTION(BlueprintCallable, Category = "Health")
		void SetCurrentHealth(float healthValue);

	/** �����˺����¼�����APawn���ǡ�*/
	UFUNCTION(BlueprintCallable, Category = "Health")
		float TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Skill")
		void PlaySkill(const FString& filePath, AActor* Sender = nullptr, bool bPlayImmediately = true);
	UFUNCTION(BlueprintCallable, Category = "Skill")
		void StopSkill(EAblAbilityTaskResult reason);

	UFUNCTION(BlueprintCallable, Category = "IK")
		void ClacIK(float deltaSecs);
	UFUNCTION(BlueprintCallable, Category = "IK")
		bool SweapCollisionTrace(AActor* pActor, const FVector& start, const FVector& end, FHitResult& Hit);

	/** �������߼� */
	UFUNCTION(Server, Reliable)
		void HandleInteractive(UGInterActiveComponent* target, bool bStart);

	/** �������� */
	UFUNCTION(BlueprintCallable, Category = "Crash")
		int32 TryCrash();

	UFUNCTION(BlueprintCallable, Category = "Crash")
		int32 TryCheck();

protected:
	/** RepNotify������ͬ��Guid */
	UFUNCTION()
		void OnRep_Guid();

	/** RepNotify������ͬ���Ե�ǰ����ֵ�����ĸ��ġ�*/
	UFUNCTION()
		void OnRep_CurrentHealth();

	/** ��ӦҪ���µ�����ֵ���޸ĺ������ڷ������ϵ��ã����ڿͻ����ϵ�������ӦRepNotify*/
	void OnHealthUpdate();


	/** ����������������ĺ�����*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		void StartFire();

	/** ���ڽ�����������ĺ�����һ��������δ��룬��ҿ��ٴ�ʹ��StartFire��*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		void StopFire();

	/** ��������Ͷ����ķ�����������*/
	UFUNCTION(Server, Reliable)
		void HandleFire();

protected:
	/** ��ҵ�Ψһ��ʶ */
	UPROPERTY(ReplicatedUsing = OnRep_Guid)
		int64 Guid;

	/** ��ҵ��������ֵ��������ҵ��������ֵ��Ҳ�ǳ���ʱ������ֵ��*/
	UPROPERTY(EditDefaultsOnly, Category = "Health")
		float MaxHealth;

	/** ��ҵĵ�ǰ����ֵ������0�ͱ�ʾ������*/
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
		float CurrentHealth;

	// ���
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Projectile")
		TSubclassOf<class AThirdPersonBullet> ProjectileClass;

	/** ���֮����ӳ٣���λΪ�롣���ڿ��Ʋ��Է����������ٶȣ����ɷ�ֹ������������������½�SpawnProjectileֱ�Ӱ������롣*/
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
		float FireRate;

	/** ��Ϊtrue�������ڷ���Ͷ���*/
	bool bIsFiringWeapon;

	/** ��ʱ������������ṩ���ɼ��ʱ���ڵ������ӳ١�*/
	FTimerHandle FiringTimer;

	/** ������� */
	UAblAbilityComponent* m_SkillComp;

	/** IK */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		bool m_RightFootHit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		FRotator m_RightFootRotOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_RightFootZOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		bool m_LeftFootHit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		FRotator m_LeftFootRotOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_LeftFootZOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_PelvisZOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_FootRollMin = -30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_FootRollMax = 30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_FootPitchMin = -30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_FootPitchMax = 30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_FootZMin = -30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationIK", AdvancedDisplay)
		float m_FootZMax = 20.0f;
};

