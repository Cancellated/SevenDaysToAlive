// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAPlayer_Murdock.h"
#include "SevenDaysToAlive.h"

// Sets default values
ASDTAPlayer_Murdock::ASDTAPlayer_Murdock()
{
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer_Murdock] %s - 构造函数初始化"), *GetName());
}

// Called when the game starts or when spawned
void ASDTAPlayer_Murdock::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogSevenDaysToAlive, Log, TEXT("[SDTAPlayer_Murdock] %s - BeginPlay开始"), *GetName());
}

// Called every frame
void ASDTAPlayer_Murdock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
