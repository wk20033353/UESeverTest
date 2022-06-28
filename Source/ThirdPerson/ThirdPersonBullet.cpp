// Fill out your copyright notice in the Description page of Project Settings.
#include "ThirdPersonBullet.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"


// Sets default values
AThirdPersonBullet::AThirdPersonBullet()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	//���彫��ΪͶ���Ｐ����ײ�ĸ������SphereComponent��
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	SphereComponent->InitSphereRadius(37.5f);
	SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = SphereComponent;
	//�ڻ����¼���ע���Ͷ����ײ��������
	if (GetLocalRole() == ROLE_Authority)
	{
		SphereComponent->OnComponentHit.AddDynamic(this, &AThirdPersonBullet::OnProjectileImpact);
	}

	//���彫��Ϊ�Ӿ����ֵ������塣
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere"));
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	StaticMesh->SetupAttachment(RootComponent);

	//���ɹ��ҵ�Ҫʹ�õľ�̬�������ʲ��������øþ�̬�����弰��λ��/������
	if (DefaultMesh.Succeeded())
	{
		StaticMesh->SetStaticMesh(DefaultMesh.Object);
		StaticMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -37.5f));
		StaticMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	}

	//����ϵͳ����������Ҫʹ�õ����ݣ��ں������������ɱ�ըЧ����
	static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultExplosionEffect(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
	if (DefaultExplosionEffect.Succeeded())
	{
		ExplosionEffect = DefaultExplosionEffect.Object;
	}

	//����Ͷ�����ƶ������
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->SetUpdatedComponent(SphereComponent);
	ProjectileMovementComponent->InitialSpeed = 1500.0f;
	ProjectileMovementComponent->MaxSpeed = 1500.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

	//��ʼ��Ͷ���ｫ��Actor��ɵ��˺����Լ������˺��¼���ʹ�õ��˺���
	DamageType = UDamageType::StaticClass();
	Damage = 10.0f;
}

// Called when the game starts or when spawned
void AThirdPersonBullet::BeginPlay()
{
	Super::BeginPlay();
	
}

void AThirdPersonBullet::Destroyed()
{
	//ÿ����Actor���ݻ�ʱ���ͻ���� Destroyed ���������ӷ���������ͨ�������ƣ�������Actor�ݻٻḴ�ƣ�����֪�����ڷ������ϴݻٴ�Ͷ���
	//��������ӿͻ����ڴݻٸ��Ե�Ͷ���︱��ʱ�����ô˺����������������Ҷ��ῴ��Ͷ���ﱻ�ݻ�ʱ�ı�ըЧ����
	FVector spawnLocation = GetActorLocation();
	UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, spawnLocation, FRotator::ZeroRotator, true, EPSCPoolMethod::AutoRelease);
}

// Called every frame
void AThirdPersonBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AThirdPersonBullet::OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor)
	{
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, NormalImpulse, Hit, GetInstigator()->Controller, this, DamageType);
	}

	Destroy();
}
