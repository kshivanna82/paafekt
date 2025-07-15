// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FurniLife.generated.h"

UCLASS()
class INTELLISPACE_API AFurniLife : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFurniLife();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
    
    UFUNCTION(BluePrintCallable, Category = "File I/O")
    static FString LoadFileToString(FString Filename);

};
