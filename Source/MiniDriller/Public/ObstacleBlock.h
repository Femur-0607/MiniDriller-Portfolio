// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "ObstacleBlock.generated.h"

UCLASS()
class MINIDRILLER_API AObstacleBlock : public ABlock
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AObstacleBlock();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 currentHp = 5; // 장애물블록의 체력 (5번 공격 시 파괴)

public:
	virtual void ResetBlockState() override;
	virtual void OnInteracted(class ADrillerCharacter* Player) override;
	
	// 타격 시 재생될 효과음
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* HitSound;

	// 체력별 스프라이트 배열 (인덱스 1 = 체력 1일 때 이미지, 인덱스 2 = 체력 2일 때 이미지)
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<class UPaperSprite*> DamagedSprites;
};
