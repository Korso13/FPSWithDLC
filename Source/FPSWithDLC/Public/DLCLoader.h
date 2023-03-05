// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapInfoAsset.h"
#include "UObject/NoExportTypes.h"
#include "DLCLoader.generated.h"


class FPSWITHDLC_API FDLCLoaderFileVisitor : public IPlatformFile::FDirectoryVisitor
{
public:

	TArray<FString> FilesPaths;
	
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if(!bIsDirectory)
		{
			FilesPaths.Add(FilenameOrDirectory);
		}
		return true;
	}
};

/**
 * 
 */
UCLASS()
class FPSWITHDLC_API UDLCLoader : public UObject
{
	GENERATED_BODY()
	
private:

	UPROPERTY()
	TArray<FMapInfo> MapsInfo;

	class FPakPlatformFile* DLCFile = nullptr;

	
#if UE_BUILD_SHIPPING == 0
	IPlatformFile* OriginalPlatformFile = nullptr;
#endif
	
public:

	bool ReadDLCMapsInfo();
	
	TArray<FMapInfo> GetMapsInfo();

	void Clear();

private:

	TArray<FString> GetClassesToLoad();
	
	TArray<FString> LoadDLC();

	//DLC methods:
	bool MountDLC(const FString& PakFileName);
	bool UnMountDLC(const FString& PakFileName);
	int32 GetDLCOrder(const FString& PakFilePath);

	void RegisterMountPoint(const FString& RootPath, const FString& ContentPath);

	TArray<FString> GetFilesInDLC(const FString& Directory);
	
	bool ReadPakFile(FString PakFileName);
	
	UClass* LoadClassFromDLC(const FString& FileName);

	FPakPlatformFile* GetDLCFile();
	
};
