// Fill out your copyright notice in the Description page of Project Settings.

#include "GAssetManager.h"

#define g_ReleaseTime 600 //10 min
UGAssetManager* UGAssetManager::Get()
{
	return Cast<UGAssetManager>(GEngine->AssetManager);
}

UGAssetManager::UGAssetManager() : Super()
{
	RegisterWorldBeginTearDown();
}

void UGAssetManager::OnWorldBeginTearDown(UWorld* InWorld)
{
	EmptyObjectPoolMap();
	AssetDelayLoadQueue.Empty();
}

FSoftObjectPath UGAssetManager::ChangeToSoftObjectPath(const FString& AssetPath)
{
	int32_t Index = 0;
	if (!AssetPath.FindLastChar('/', Index))
	{
		AssetPath.FindLastChar('\\', Index);
	}
	const FString Path = "/Game/" + AssetPath + "." + AssetPath.Right(AssetPath.Len() - Index - 1);
	return  FSoftObjectPath(Path);
}

FSoftClassPath UGAssetManager::ChangeToSoftClassPath(const FString& AssetPath)
{
	int32_t Index = 0;
	if (!AssetPath.FindLastChar('/', Index))
	{
		AssetPath.FindLastChar('\\', Index);
	}
	const FString Path = "/Game/" + AssetPath + "." + AssetPath.Right(AssetPath.Len() - Index - 1) + "_C";
	return  FSoftClassPath(Path);
}

FString UGAssetManager::ChangeToResPath(const FSoftClassPath& SoftClassPath)
{
	FString Path = SoftClassPath.ToString();
	int32_t Index = 0;
	if (!Path.FindLastChar('.', Index))
	{
		Index = Path.Len();
	}
	return Path.Mid(6, Index - 6); // Len("/Game/") = 6
}

FString UGAssetManager::ChangeToResPath(const FSoftObjectPath& SoftObjectPath)
{
	FString Path = SoftObjectPath.ToString();
	int32_t Index = 0;
	if (!Path.FindLastChar('.', Index))
	{
		Index = Path.Len();
	}
	return Path.Mid(6, Index - 6); // Len("/Game/") = 6
}

bool UGAssetManager::IsAssetExist(const FString& AssetPath)
{
	FSoftObjectPath path = UGAssetManager::ChangeToSoftObjectPath(AssetPath);
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(path.GetAssetPathName());
	return AssetData.IsValid();
}

//异步加载纯资源文件
void UGAssetManager::AsyncLoadResource(const FString& AssetPath, const TFunction<void(UObject*)>& Callback, bool Delay/* = false*/)
{
	const FSoftObjectPath SoftObjectPath = ChangeToSoftObjectPath(AssetPath);
	AsyncLoadResource(SoftObjectPath, Callback, Delay);
}

void UGAssetManager::AsyncLoadResource(const FSoftObjectPath& SoftObjectPath, const TFunction<void(UObject*)>& Callback, bool Delay/* = false*/)
{
	UObject* Object = TryGetObjectFromPool(SoftObjectPath.ToString());
	if (Object)
	{
		UE_LOG(LogTemp, Log, TEXT("UGAssetManager::AsyncLoadResource from Pool %s"), *SoftObjectPath.ToString());
		Callback(Object);
		return;
	}
	if(Delay)
	{
		AssetDelayLoadQueue.Enqueue(FGAssetDelayLoad(SoftObjectPath, Callback));
		return;
	}
	const TSharedPtr<FStreamableHandle> Handle = GetStreamableManager().RequestAsyncLoad(TArray<FSoftObjectPath>{SoftObjectPath});
	BindStreamableCompleteDelegate(Handle, Callback);
}

void UGAssetManager::RequestAsyncLoad(const TArray<FSoftObjectPath>& TargetsToStream, TFunction<void(const TArray<UObject*>&)> Callback)
{
	TArray<FSoftObjectPath> RequestPaths;
	TArray<FString> TargetStringArray;
	TArray<UObject*> Caches;
	for (const FSoftObjectPath& SoftObjectPath : TargetsToStream)
	{
		FString PathName = SoftObjectPath.ToString();
		TargetStringArray.Push(PathName);
		UObject* Object = TryGetObjectFromPool(PathName);
		if (Object)
		{
			Object->AddToRoot();
			Caches.Add(Object);
		}
		else
		{
			RequestPaths.Add(SoftObjectPath);
		}
	}
	if (RequestPaths.Num() == 0)
	{
		Callback(Caches);
		UE_LOG(LogTemp, Log, TEXT("UGAssetManager::RequestAsyncLoad All Cache %d"), Caches.Num());
		return;
	}
	const TSharedPtr<FStreamableHandle> StreamableHandle = GetStreamableManager().RequestAsyncLoad(RequestPaths);
	if (StreamableHandle->HasLoadCompleted())
	{
		StreamableCompleteDelegates(StreamableHandle, Callback, Caches, TargetStringArray);
		return;
	}

	FStreamableDelegate Delegate;
	Delegate.BindLambda([StreamableHandle, Callback, Caches, TargetStringArray]()
	{
		StreamableCompleteDelegates(StreamableHandle, Callback, Caches, TargetStringArray);
	});
	StreamableHandle->BindCompleteDelegate(MoveTemp(Delegate));
}

//异步加载蓝图类文件
void UGAssetManager::AsyncLoadBluePrint(const FString& Path, const TFunction<void(UObject*)>& Callback, bool Delay/* = false*/)
{
	static FSoftClassPath s_SoftClassPath;
	s_SoftClassPath = ChangeToSoftClassPath(Path);
	AsyncLoadBluePrint(s_SoftClassPath, Callback, Delay);
}

void UGAssetManager::AsyncLoadBluePrint(const FSoftClassPath& SoftClassPath, const TFunction<void(UObject*)>& Callback, bool Delay/* = false*/)
{
	UObject* Object = TryGetObjectFromPool(SoftClassPath.ToString());
	if (Object)
	{
		UE_LOG(LogTemp, Log, TEXT("UGAssetManager::AsyncLoadBluePrint from Pool %s"), *SoftClassPath.ToString());
		Callback(Object);
		return;
	}
	if(Delay)
	{
		AssetDelayLoadQueue.Enqueue(FGAssetDelayLoad(SoftClassPath, Callback));
		return;
	}
	TSharedPtr<FStreamableHandle> Handle = GetStreamableManager().RequestAsyncLoad(TArray<FSoftObjectPath>{SoftClassPath});
	BindStreamableCompleteDelegate(Handle, Callback);
}

//同步加载纯资源文件
UObject* UGAssetManager::SyncLoadResource(const FString& AssetPath) const
{
	const FSoftObjectPath SoftObjectPath = ChangeToSoftObjectPath(AssetPath);
	UObject* pObj = GetStreamableManager().LoadSynchronous(SoftObjectPath);
	return pObj;
}

//同步加载蓝图类文件
UObject* UGAssetManager::SyncLoadBluePrint(const FString& Path) const
{
	const FSoftObjectPath SoftClassPath = ChangeToSoftClassPath(Path);
	UObject* pObj = GetStreamableManager().LoadSynchronous(SoftClassPath);
	return pObj;
}

void UGAssetManager::Recycle(UObject* Object)
{
	if (!Object)
	{
		return;
	}
	CheckObjectPool();	// Remove OutData Object First
	FString PathName = Object->GetPathName();
	UE_LOG(LogTemp, Log, TEXT("UGAssetManager::Recycle %s"), *PathName);
	FObjectPool& Value = ObjectPoolMap.FindOrAdd(PathName);
	Value.Add(Object);
}

void UGAssetManager::RecycleActor(AActor* Object)
{
	if (!Object)
	{
		return;
	}
	CheckObjectPool();	// Remove OutData Object First

	UClass* pClass = Object->GetClass();
	if (pClass == nullptr)
	{
		return;
	}
	FString PathName = pClass->GetPathName();
	UE_LOG(LogTemp, Log, TEXT("UGAssetManager::Recycle %s"), *PathName);
	FObjectPool& Value = ObjectPoolMap.FindOrAdd(PathName);
	Value.IsActor = true;
	Value.Add(Object);
}

void UGAssetManager::BindStreamableCompleteDelegate(TSharedPtr<FStreamableHandle> StreamableHandle, TFunction<void(UObject*)> Callback) const
{
	UObject* Asset = StreamableHandle->GetLoadedAsset();
	if (Asset)
	{
		if (Callback)
		{
			Callback(Asset);
			StreamableHandle->ReleaseHandle();
		}
		return;
	}
	FStreamableDelegate Delegate;
	Delegate.BindLambda([Callback, StreamableHandle, this]()
	{
		this->StreamableCompleteDelegate(StreamableHandle, Callback);
		StreamableHandle->ReleaseHandle();
	});
	StreamableHandle->BindCompleteDelegate(MoveTemp(Delegate));
}

void UGAssetManager::StreamableCompleteDelegate(TSharedPtr<FStreamableHandle> StreamableHandle, TFunction<void(UObject*)> Callback)
{
	UObject* Asset = StreamableHandle->GetLoadedAsset();
	if (Callback)
	{
		Callback(Asset);
	}
}

void UGAssetManager::StreamableCompleteDelegates(TSharedPtr<FStreamableHandle> StreamableHandle,
	TFunction<void(const TArray<UObject*>&)> Callback, TArray<UObject*> Caches, const TArray<FString>& PathStringArray)
{
	StreamableHandle->GetLoadedAssets(Caches);
	TArray<UObject*> Result(nullptr, PathStringArray.Num());
	for (UObject* Object : Caches)
	{
		const FString PathName = Object->GetPathName();
		for (int Index = 0; Index < PathStringArray.Num(); Index++)
		{
			if (PathName == PathStringArray[Index])
			{
				Result[Index] = Object;
				break;
			}
		}
	}
	if (Callback)
	{
		Callback(Result);
	}
}

void UGAssetManager::OnTick(float DeltaTime)
{
	TickAssetDelayLoad(DeltaTime);
}

void UGAssetManager::TickAssetDelayLoad(float DeltaTime)
{
	if(AssetDelayLoadQueue.IsEmpty())
	{
		return;
	}

	FGAssetDelayLoad AssetDelayLoad;
	if(AssetDelayLoadQueue.Dequeue(AssetDelayLoad) == false)
	{
		return;
	}

	const TSharedPtr<FStreamableHandle> Handle = GetStreamableManager().RequestAsyncLoad(TArray<FSoftObjectPath>{AssetDelayLoad.SoftObjectPath});
	BindStreamableCompleteDelegate(Handle, AssetDelayLoad.Callback);
}

UObject* UGAssetManager::TryGetObjectFromPool(const FString& Path)
{
	//todo 异常崩溃
	return nullptr;
	FObjectPool* ObjectPool = ObjectPoolMap.Find(Path);
	if (!ObjectPool)
	{
		return nullptr;
	}
	return ObjectPool->Pop();
}

void UGAssetManager::RegisterWorldBeginTearDown()
{
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UGAssetManager::OnWorldBeginTearDown);
}

void UGAssetManager::EmptyObjectPoolMap()
{
	ObjectPoolMap.Empty();
}

void UGAssetManager::CheckObjectPool()
{
	int64 Timestamp = FDateTime::Now().GetTicks() / ETimespan::TicksPerMillisecond;
	float Delta = (Timestamp - PrevPoolCheckTimestamp) * 0.001f;	//Second;
	if (Delta < g_ReleaseTime)
	{
		return;
	}

	PrevPoolCheckTimestamp = Timestamp;
	TArray<FString> EmptyKey;
	for (auto& it : ObjectPoolMap)
	{
		if (it.Value.TickOutDate(Delta) == 0)
		{
			EmptyKey.Add(it.Key);
		}
	}

	for (FString& Key : EmptyKey)
	{
		ObjectPoolMap.Remove(Key);
		UE_LOG(LogTemp, Log, TEXT("UGAssetManager Object Remove Empty Pool %s"), *Key);
	}
}

void UGAssetManager::FObjectPool::Add(UObject* Object)
{
	FObjectCache& Cache = ObjectCaches.AddDefaulted_GetRef();
	Cache.Object = Object;
	Object->AddToRoot();
	Cache.Seconds = g_ReleaseTime;
}

UObject* UGAssetManager::FObjectPool::Pop()
{
	if (ObjectCaches.Num())
	{
		auto Index = ObjectCaches.Num() - 1;
		UObject* Object = ObjectCaches[Index].Object;
		Object->RemoveFromRoot();
		ObjectCaches.RemoveAtSwap(Index, 1, false);
		return Object;
	}
	return nullptr;
}

int UGAssetManager::FObjectPool::TickOutDate(float Seconds)
{
	int Num = ObjectCaches.Num();
	for (int Index = Num - 1; Index >= 0; Index--)
	{
		FObjectCache& ObjectCache = ObjectCaches[Index];
		if (ObjectCache.Seconds > Seconds)
		{
			ObjectCache.Seconds -= Seconds;
			continue;
		}
		UE_LOG(LogTemp, Log, TEXT("UGAssetManager Object Remove OutDate %s"), *ObjectCache.Object->GetName());
		ObjectCache.Object->RemoveFromRoot();
		if (IsActor)
		{
			AActor* pActor = Cast<AActor>(ObjectCache.Object);
			pActor->Destroy();
		}
		ObjectCache.Object = nullptr;
		ObjectCaches.RemoveAtSwap(Index, 1, false);
	}
	return ObjectCaches.Num();
}

UGAssetManager::FObjectPool::FObjectPool()
{
	IsActor = false;
}

UGAssetManager::FObjectPool::~FObjectPool()
{
	for (FObjectCache& ObjectCache : ObjectCaches)
	{
		ObjectCache.Object->RemoveFromRoot();
	}
}
