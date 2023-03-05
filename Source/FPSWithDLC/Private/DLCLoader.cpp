// Fill out your copyright notice in the Description page of Project Settings.


#include "DLCLoader.h"

#include "IPlatformFilePak.h"
#include "MapInfoAsset.h"
#include "Engine/AssetManager.h"

bool UDLCLoader::ReadDLCMapsInfo()
{
#if UE_BUILD_SHIPPING
	//
#else

	//Obtaining Asset Manager, which will gather all assets for us
	UAssetManager* assetManager = &UAssetManager::Get();

	//Getting all classes that contain DLC assets' metadata (MapInfoAsset's childs)
	TArray<FString> classesToLoad = GetClassesToLoad();

	//write data about all assets into RegistryData array. Returns true if successful
	//The process is not fast, but it will only run in Editor! Use GetAssets() with filter or GetAssetsByClass for optimizing the process
	TArray<FAssetData> RegistryData;
	if(assetManager->GetAssetRegistry().GetAllAssets(RegistryData))
	{
		//Iterate through all found assets and compare them against classes we need to work with to get DLC metadata
		for(FAssetData data : RegistryData)
		{
			for(FString ClassToLoad : classesToLoad)
			{
				if(data.AssetName == FName(*ClassToLoad))
				{
					UObject* objectToLoad = nullptr; //we will write here BP generated from our DLC metadata class

					//checking if the asset (data) is a BP generated out of a C++ class 
					FAssetTagValueRef generatedClassPath = data.TagsAndValues.FindTag(FName("GeneratedClass"));

					//if it was generated...
					if(generatedClassPath.IsSet())
					{
						//get path to asset class (and its name)
						const FString classObjectPath = FPackageName::ExportTextPathToObjectPath(generatedClassPath.GetValue());

						//try to load class asset by obtained path
						objectToLoad = FSoftObjectPath(classObjectPath).TryLoad();
					}

					//if we found and successfully loaded (UMapInfoAsset's) Generated Class...
					if(objectToLoad)
					{
						//we cast it to its base BP class
						UBlueprintGeneratedClass* BPGeneratedClass = Cast<UBlueprintGeneratedClass>(objectToLoad);

						//and then try to get its CDO (those are generated only for BPs) and cast to our metadata class
						UMapInfoAsset* mapInfoObject = Cast<UMapInfoAsset>(BPGeneratedClass->GetDefaultObject());

						//if the operation was successful...
						if(mapInfoObject)
						{
							//we extract the metadata from the found Generated Class and write it in our array for metadata
							MapsInfo.Add(mapInfoObject->MapInfo);
						}
					}
				}
			}
		}
	}
#endif
	
	LoadDLC(); //won't do anything if launched from editor
	
	return (MapsInfo.Num() > 0);
}

TArray<FString> UDLCLoader::GetClassesToLoad()
{
	//will write final result for return here:
	TArray<FString> outClasses;

	//this will hold names of base classes with metadata
	TArray<FName> baseClasses;
	//this will hold names for all excluded classes
	TSet<FName> excludedClasses;
	//this will hold names of all found classes that were derived from the base metadata class
	TSet<FName> derivedClasses;

	//Here we add all names of StaticClass for the classes that hold our DLCs' metadata
	baseClasses.Add(UMapInfoAsset::StaticClass()->GetFName());

	//Obtaining Asset Manager, which will find names for all classes that were derived from the base classes in the array.
	UAssetManager* assetManager = &UAssetManager::Get();
	assetManager->GetAssetRegistry().GetDerivedClassNames(baseClasses, excludedClasses, derivedClasses);

	for(FName derivedClass : derivedClasses)
	{
		//Trim class name to get ClasName instead of ClassName.ClassName_C
		//also convert to designated output type - FStrings
		FString derivedClassName = derivedClass.ToString();

		//checking if the class name is the name of a derived class indeed. If true..
		if(derivedClassName.EndsWith("_C"))
		{
			//...trim last two letters
			derivedClassName = derivedClassName.Mid(0, derivedClassName.Len() - 2);
		}

		//now add the resulting string to out output array
		outClasses.Add(derivedClassName);
	}

	return outClasses;
}

TArray<FMapInfo> UDLCLoader::GetMapsInfo()
{
	if(MapsInfo.Num() == 0)
	{
		ReadDLCMapsInfo();
	}
	return MapsInfo;
}

void UDLCLoader::Clear()
{
	MapsInfo.Empty();
}

TArray<FString> UDLCLoader::LoadDLC()
{
	TArray<FString> LoadedPlugins;
#if WITH_EDITOR
	UE_LOG(LogTemp, Warning, TEXT("Launched in Editor. Aborting DLC loading"));
	return LoadedPlugins;
#endif

	//Get DLC folder full path
	FString PathToDLCFolder = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "DLC";
	UE_LOG(LogTemp, Warning, TEXT("PATH: %s "), *PathToDLCFolder); 

	//Get file manager to check if DLC folder exists
	IFileManager& FileManager = IFileManager::Get();
	if(!FPaths::DirectoryExists(PathToDLCFolder)) //if DLC folder doesn't exist...
	{
		UE_LOG(LogTemp, Error, TEXT("DLC directory %s not found!"), *PathToDLCFolder);
		//Create one and return empty list of DLCs
		FileManager.MakeDirectory(*PathToDLCFolder, true);
		return LoadedPlugins;
	}

	// Get all *.pak files and try to load plugins
	TArray<FString> PAKFiles;
	FString DLCExtention = "*.pak";
	FileManager.FindFilesRecursive(PAKFiles, *PathToDLCFolder, *DLCExtention, true, false);

	for(auto PAKFile : PAKFiles)
	{
		MountDLC(PAKFile);
		ReadPakFile(PAKFile);
	}
	return LoadedPlugins;
}

bool UDLCLoader::MountDLC(const FString& PakFileName)
{
	//Get pak's assets' load priority
	int32 PakOrder = GetDLCOrder(PakFileName);

	//Getting FPakPlatformFile interface
	if(!DLCFile)
	{
		DLCFile = GetDLCFile();
		if(!DLCFile)
		{
			return false;
		}
	}

	//get all mounted pak files' names for DEBUG purposes
	TArray<FString> mountedPoints1;
	DLCFile->GetMountedPakFilenames(mountedPoints1);

	//mount passed pak file
	bool result = DLCFile->Mount(*PakFileName, PakOrder,NULL);

	//get all mounted pak files' names AFTER mounting for DEBUG purposes
	TArray<FString> mountedPoints2;
	DLCFile->GetMountedPakFilenames(mountedPoints2);

	//can be called directly during .Mount() call, if we're not using debugging code above
	return result;
}

bool UDLCLoader::UnMountDLC(const FString& PakFileName)
{
	//Get pak's assets' load priority
	int32 PakOrder = GetDLCOrder(PakFileName);

	//Getting FPakPlatformFile interface
	if(!DLCFile)
	{
		DLCFile = GetDLCFile();
		if(!DLCFile)
		{
			return false;
		}
	}

	//get all mounted pak files' names to  check if the pak file we are unmounting had been mounted
	//also useful for debugging
	TArray<FString> mountedPoints;
	DLCFile->GetMountedPakFilenames(mountedPoints);

	if(mountedPoints.Contains(PakFileName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Mount point %s exists! Unmount started..."), *PakFileName);

		//unmounted passed pak file
		if(DLCFile->Unmount(*PakFileName))
        {
			UE_LOG(LogTemp, Warning, TEXT("Unmounted!"));
			return true;
        }
        else
        {
        	UE_LOG(LogTemp, Error, TEXT("Unmounting error!"));
			return false;
        }
	}

	//pak file was not mounted so... success?)
	return true;
}

int32 UDLCLoader::GetDLCOrder(const FString& PakFilePath)
{
	//if pak is in the folder with other content Paks - set priority to 4
	if (PakFilePath.StartsWith(FString::Printf(TEXT("%sPaks/%s-"), *FPaths::ProjectContentDir(), FApp::GetProjectName())))
	{
		return 4;
	}
	//if pak is in the content root directory - set priority to 3
	else if (PakFilePath.StartsWith(FPaths::ProjectContentDir()))
	{
		return 3;
	}
	//if pak is with Engine content - set priority to 2
	else if (PakFilePath.StartsWith(FPaths::EngineContentDir()))
	{
		return 2;
	}
	//if pak is a part of the Saved directory - set priority to 1
	else if (PakFilePath.StartsWith(FPaths::ProjectSavedDir()))
	{
		return 1;
	}

	//in all other cases set priority to 0 
	return 0;
}

void UDLCLoader::RegisterMountPoint(const FString& RootPath, const FString& ContentPath)
{
	FPackageName::RegisterMountPoint(RootPath, ContentPath);
}

TArray<FString> UDLCLoader::GetFilesInDLC(const FString& Directory)
{
	FDLCLoaderFileVisitor Visitor;
	if(!DLCFile)
	{
		GetDLCFile();
	}
	if(DLCFile)
	{
		DLCFile->IterateDirectory(*Directory, Visitor);
	}
	
    return Visitor.FilesPaths;
}

bool UDLCLoader::ReadPakFile(FString PakFileName)
{
	//just logging
	UE_LOG(LogTemp, Warning, TEXT("ReadPakFile: Mount pak file : %s"), *PakFileName);

	//declare local Pak platform file
	FPakPlatformFile *PakPlatformFile;
    {
		//get the name for our pak platform file
		FString PlatformFileName = FPlatformFileManager::Get().GetPlatformFile().GetName();
		if (PlatformFileName.Equals(FString(TEXT("PakFile"))))
		{
			//if we found our platform pak file, cast it and write down into a local variable
			PakPlatformFile = static_cast<FPakPlatformFile*>(&FPlatformFileManager::Get().GetPlatformFile());
		}
		else
		{
			//if not - we create a new one (comments for this section can be looked up in code for GetDLCFile()
			PakPlatformFile = new FPakPlatformFile;
			if(!PakPlatformFile->Initialize(&FPlatformFileManager::Get().GetPlatformFile(),TEXT("")))
			{
				UE_LOG(LogTemp, Error, TEXT("FPakPlatformFile failed to initialize"));
				return false;
			}

			FPlatformFileManager::Get().SetPlatformFile(*PakPlatformFile);
		}
    }

	//Get pak file's full path
	FString PakFilePathFull = FPaths::ConvertRelativePathToFull(PakFileName);

	//var containing our pak file. Opened using FPakPlatformFile and full path to file.
	//bool var depends on whether pak file was signed!
	FPakFile* PakFile = new FPakFile(PakPlatformFile, *PakFilePathFull, false);

	//List of files within pak archive. To be filled several lines below
	TArray<FString> FileList;

	//name of the pak file
	FString packName = FPaths::GetBaseFilename(PakFileName);

	//Mount point for teh pak file
	FString MountPoint = PakFile->GetMountPoint();

	//Fill the array with names of files within this pak file. Directories excluded (change second bool if needed)
	//PakFile->FindFilesAtPath(FileList, *MountPoint, true, false, true);
	//Line above might be depricated, recomended to use this one instead:
	PakFile->FindPrunedFilesAtPath(FileList, *MountPoint, true, false, true);
	
	FStreamableManager StreamableManager;

	//we go over the files within the pak file
	for (int32 k = 0; k < FileList.Num(); k++)
	{
		//form the name of the asset based on the name of the file within the pak
		FString AssetName = FileList[k];

		//check if the file is a Blueprint asset
		//IMPORTANT!! Won't work if you don't respect naming conventions!!
		if (AssetName.Contains("BP_") && AssetName.Contains(".uasset"))
		{
			//just logging
			UE_LOG(LogTemp, Log, TEXT("Loading Pak: %s from File %s ..."), *packName, *FileList[k]);

			FString AssetShortName = FPackageName::GetShortName(AssetName);

			//split the short name of the asset into name and extension
			FString FileName, FileExt;
			AssetShortName.Split(TEXT("."), &FileName, &FileExt);

			//make a new name for the asset by format: /Pack's_name/Asset_File_Name.Asset_File_Name_C
			//Asset_File_Name here is effectively a class name and _C means we want a class, not an object
			FString NewAssetName = FString("/") + packName + "/" + FileName + TEXT(".") + FileName + "_C";

			//just logging
			UE_LOG(LogTemp, Log, TEXT("Loading Asset %s ..."), *NewAssetName);

			//will load our asset's object here:
			UObject * objectToLoad = nullptr;

			//FStringAssetReference is kinda SoftObjectPath we get using new asset name.
			//We use it here to get FSoftObjectPath (masked as FStringAssetReference) for the asset
			FStringAssetReference assetRef = FStringAssetReference(NewAssetName);

			//just logging
			UE_LOG(LogTemp, Warning, TEXT("load object..."));

			//trying to load asset using its FSoftObjectPath
			objectToLoad = assetRef.TryLoad();

			//in case it succesfully loaded...
			if (objectToLoad)
			{
				//casting it to base Blueprint class to get its CDO, which we will cast to DLC metadata-holding class: 
				UBlueprintGeneratedClass* generatedClass = Cast<UBlueprintGeneratedClass>(objectToLoad);
				UMapInfoAsset * mapInfoObject = Cast<UMapInfoAsset>(generatedClass->GetDefaultObject());

				// if cast was successful...
				if (mapInfoObject)
				{
					//extract metadata from the object and add it to our array of metadata
					MapsInfo.Add(mapInfoObject->MapInfo);
					
					//just logging
					UE_LOG(LogTemp, Warning, TEXT("Add data %s"), *mapInfoObject->GetFullName());
				}
				else
				{
					//if it's not out metadata holder, print error
					UE_LOG(LogTemp, Log, TEXT("File %s loading error!"), *FileList[k]);
				}
			}//might add else -> error message here as well, which will pop up if we fail to load the asset at all
		}
	} //end of "for" cycle's code

	return true;
}

UClass* UDLCLoader::LoadClassFromDLC(const FString& FileName)
{
	const FString Name = FileName + TEXT(".") + FPackageName::GetShortName(FileName) + TEXT("_C");
	return StaticLoadClass(UObject::StaticClass(), nullptr, *Name);
}

FPakPlatformFile* UDLCLoader::GetDLCFile()
{
	if (!DLCFile)
    {
		//find PlatformFileManager fit for working with pak files on the current platform
		IPlatformFile* CurrentPlatformFile = FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile"));

		//if one was found - cast it to FPakPlatformFile and write in our class field
		if (CurrentPlatformFile)
		{
			DLCFile = static_cast<FPakPlatformFile*>(CurrentPlatformFile);
		}
		//if getting platform file failed...
		else 
		{
			//...create one!
			DLCFile = new FPakPlatformFile();
			ensure(DLCFile != nullptr);

			//try to get a IPlatformFile to initialize the newly created FPakPlatformFile
			IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

			#if UE_BUILD_SHIPPING == 0
			OriginalPlatformFile = &PlatformFile;
			#endif

			//initialize the FPakPlatformFile.
			if (DLCFile->Initialize(&PlatformFile, TEXT("")))
			{
				// If successful - pass it to the FPlatformFileManager as the new PlatformFile for paks
				FPlatformFileManager::Get().SetPlatformFile(*DLCFile);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("DLCFile initialization error!"));
			}
		}
    }

	ensure(DLCFile != nullptr);
    return DLCFile;
}
