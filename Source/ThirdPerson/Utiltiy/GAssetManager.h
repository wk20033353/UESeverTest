// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "GAssetManager.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSON_API UGAssetManager : public UAssetManager
{
	GENERATED_BODY()


public:
	static UGAssetManager* Get();

public:
	UGAssetManager();
private:
	void OnWorldBeginTearDown(UWorld* InWorld);
public:
	//异步加载纯资源文件
	void AsyncLoadResource(const FString& AssetPath, const TFunction<void(UObject*)>& Callback, bool Delay = false);
	void AsyncLoadResource(const FSoftObjectPath& SoftObjectPath, const TFunction<void(UObject*)>& Callback, bool Delay = false);
	//异步加载蓝图类文件
	void AsyncLoadBluePrint(const FString& Path, const TFunction<void(UObject*)>& Callback, bool Delay = false);
	void AsyncLoadBluePrint(const FSoftClassPath& SoftClassPath, const TFunction<void(UObject*)>& Callback, bool Delay = false);
	//异步加载多个文件 
	void RequestAsyncLoad(const TArray<FSoftObjectPath>& TargetsToStream, TFunction<void(const TArray<UObject*>&)> Callback);
public:
	//同步加载纯资源文件
	UFUNCTION(BlueprintCallable, CallInEditor, Category = UGAssetManager)
	UObject* SyncLoadResource(const FString& AssetPath) const;
	//同步加载蓝图类文件
	UFUNCTION(BlueprintCallable, CallInEditor, Category = UGAssetManager)
	UObject* SyncLoadBluePrint(const FString& Path) const;
	void Recycle(UObject* Object);
	void RecycleActor(AActor* Object);
public:
	template< class T >
	static T* LoadClass(const FString& Path, UObject* Outer)
	{
		const FSoftClassPath& SoftClassPath = ChangeToSoftClassPath(Path);
		UClass* BlueprintVar = StaticLoadClass(T::StaticClass(), nullptr, *SoftClassPath.ToString());
		if (BlueprintVar != nullptr)
		{
			return NewObject<T>(Outer, BlueprintVar);
		}
		return nullptr;
	}

	template< class T >
	static T* LoadClass(const FSoftClassPath& SoftClassPath, UObject* Outer)
	{
		UClass* BlueprintVar = StaticLoadClass(T::StaticClass(), nullptr, *SoftClassPath.ToString());
		if (BlueprintVar != nullptr)
		{
			return NewObject<T>(Outer, BlueprintVar);
		}
		return nullptr;
	}

	template< class T >
	static T* SpawnActor(const FSoftClassPath& SoftClassPath, UWorld* World = nullptr, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters())
	{
		//Find From Pool
		FString path = SoftClassPath.GetAssetPathName().ToString();
		UObject* pObject = UGAssetManager::Get()->TryGetObjectFromPool(path);
		if (pObject != nullptr)
		{
			return Cast<T>(pObject);
		}

		//Spawn new Actor
		if (!World)
		{
			World = GetCurWorld();
		}
		UClass* BlueprintVar = StaticLoadClass(T::StaticClass(), nullptr, *SoftClassPath.ToString());
		if (BlueprintVar != nullptr)
		{
			return World->SpawnActor<T>(BlueprintVar, SpawnParameters);
		}
		return nullptr;
	}
	template< class T >
	void AsyncSpawnActor(const FString& Path, const TFunction<void(T*)>& Callback)
	{
		const FSoftClassPath SoftClassPath = ChangeToSoftClassPath(Path);
		AsyncSpawnActor(SoftClassPath, Callback);
	}
	template< class T >
	void AsyncSpawnActor(const FSoftClassPath& SoftClassPath, const TFunction<void(T*)>& Callback, bool Delay = false)
	{
		//Find From Pool
		FString path = SoftClassPath.GetAssetPathName().ToString();
		UObject* pObject = UGAssetManager::Get()->TryGetObjectFromPool(path);
		if (pObject != nullptr)
		{
			T* pActor = Cast<T>(pObject);
			if (Callback)
			{
				Callback(pActor);
			}
			return;
		}

		//Real Async
		AsyncLoadBluePrint(SoftClassPath, [this, Callback](UObject* pObj)
		{
			if (!Callback)
			{
				return;
			}
			if (!pObj)
			{
				Callback(nullptr);
				return;
			}
			UBlueprintGeneratedClass* Class = Cast<UBlueprintGeneratedClass>(pObj);
			if (Class)
			{
				T* Actor = GetCurWorld()->SpawnActor<T>(Class);
				Callback(Actor);
			}
			else
			{
				Callback(nullptr);
				UE_LOG(LogTemp, Warning, TEXT("GAssetManager %s is not Blueprint Class"), *pObj->GetName());
			}

		}, Delay);
	}
	template< class T >
	void AsyncLoadClass(UObject* Outer, const FString& Path, const TFunction<void(T*)>& Callback)
	{
		const FSoftClassPath SoftClassPath = ChangeToSoftClassPath(Path);
		AsyncLoadClass(Outer, SoftClassPath, Callback);
	}
	template< class T >
	void AsyncLoadClass(UObject* Outer, const FSoftClassPath& SoftClassPath, const TFunction<void(T*)>& Callback)
	{
		AsyncLoadBluePrint(SoftClassPath, [this, Outer, Callback](UObject* pObj)
		{
			if (!Callback)
			{
				return;
			}
			if (!pObj)
			{
				Callback(nullptr);
				return;
			}
			UBlueprintGeneratedClass* Class = Cast<UBlueprintGeneratedClass>(pObj);
			if (Class)
			{
				Callback(NewObject<T>(Outer, Class));
			}
			else
			{
				Callback(Cast<T>(pObj));
				UE_LOG(LogTemp, Warning, TEXT("GAssetManager %s is not Blueprint Class"), *pObj->GetName());
			}
		});
	}

	template<class T>
	void AsyncLoadSoftObject(TSoftObjectPtr<T> SoftObjectPtr, const TFunction<void(T*)>& Callback)
	{
		const FSoftObjectPath& SoftObjectPath = SoftObjectPtr.ToSoftObjectPath();
		AsyncLoadResource(SoftObjectPath, [Callback](UObject* Object)
		{
			Callback(Cast<T>(Object));
		});
	}
public:
	static FSoftObjectPath ChangeToSoftObjectPath(const FString& AssetPath);
	static FSoftClassPath ChangeToSoftClassPath(const FString& AssetPath);
	static FString ChangeToResPath(const FSoftClassPath& SoftClassPath);
	static FString ChangeToResPath(const FSoftObjectPath& SoftObjectPath);
	static bool IsAssetExist(const FString& AssetPath);
	template<typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer)
	{
		AssetType* ReturnVal = nullptr;
		if (AssetPointer.ToSoftObjectPath().IsValid())
		{
			ReturnVal = AssetPointer.Get();
			if (!ReturnVal)
			{
				AssetType* LoadedAsset = Cast<AssetType>(AssetPointer.ToSoftObjectPath().TryLoad());
				if (ensureMsgf(LoadedAsset, TEXT("Failed to load asset pointer %s"), *AssetPointer.ToString()))
				{
					ReturnVal = LoadedAsset;
				}
			}
		}
		return ReturnVal;
	}
private:
	static void StreamableCompleteDelegate(TSharedPtr<FStreamableHandle> StreamableHandle, TFunction<void(UObject*)> Callback);
	static void StreamableCompleteDelegates(TSharedPtr<FStreamableHandle> StreamableHandle,
		TFunction<void(const TArray<UObject*>&)> Callback, TArray<UObject*> Caches, const TArray<FString>& PathStringArray);
	void BindStreamableCompleteDelegate(TSharedPtr<FStreamableHandle> StreamableHandle, TFunction<void(UObject*)> Callback) const;

public:
	void OnTick(float DeltaTime);
private:
	void TickAssetDelayLoad(float DeltaTime);
private:
	struct FGAssetDelayLoad
	{
		FGAssetDelayLoad(const FSoftObjectPath& InSoftObjectPath, const TFunction<void(UObject*)>& InCallback)
			:SoftObjectPath(InSoftObjectPath), Callback(InCallback)
		{
			
		}
		FGAssetDelayLoad(){}
		FSoftObjectPath SoftObjectPath;
		TFunction<void(UObject*)> Callback;
	};
	TQueue<FGAssetDelayLoad> AssetDelayLoadQueue;
	////////////////////
		///ObjectPool
	UObject* TryGetObjectFromPool(const FString& Path);
	void CheckObjectPool();
	void RegisterWorldBeginTearDown();
	void EmptyObjectPoolMap();
private:
	struct FObjectCache
	{
		UObject* Object;
		float Seconds;
	};
	struct FObjectPool
	{
		TArray<FObjectCache> ObjectCaches;
		bool IsActor;

		FObjectPool();
		~FObjectPool();
		void Add(UObject* Object);
		UObject* Pop();
		int TickOutDate(float Seconds);
	};
private:
	TMap<FString, FObjectPool> ObjectPoolMap;
	int64 PrevPoolCheckTimestamp = 0;
};