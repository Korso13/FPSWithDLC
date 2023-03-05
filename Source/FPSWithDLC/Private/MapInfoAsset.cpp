// Fill out your copyright notice in the Description page of Project Settings.


#include "MapInfoAsset.h"

#include "Engine/PrimaryAssetLabel.h"

FString FMapInfo::GetLevelName() const
{
	return Name.ToString();
}

FText FMapInfo::GetLevelDescription() const
{
	return Description;
}

FString FMapInfo::GetLevelReference()
{
	if(!PrimaryAssetLabel)
	{
		PrimaryAssetLabel = Cast<UPrimaryAssetLabel>(FStringAssetReference(PrimaryAssetPath).TryLoad());
	}

	if(PrimaryAssetLabel && PrimaryAssetLabel->ExplicitAssets.Num() > 0)
	{
		return PrimaryAssetLabel->ExplicitAssets[0].GetLongPackageName();
	}

	FString AssetPath = PrimaryAssetPath.GetAssetPathString();

	int32 DotPoint = AssetPath.Find(".");
	AssetPath = AssetPath.Right(AssetPath.Len() - DotPoint - 1);
	return AssetPath;
}
