// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleBlock.h"
#include "GameStatusSubsystem.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AObstacleBlock::AObstacleBlock(){ }

// Called when the game starts or when spawned
void AObstacleBlock::BeginPlay()
{
	Super::BeginPlay();
}

void AObstacleBlock::ResetBlockState()
{
	// 1. 부모(ABlock)가 담당하는 기본 세탁(좌표 리셋, 안보이게 하기 등)을 먼저 실행합니다.
	Super::ResetBlockState();

	// 2. 장애물 블록만의 고유 변수인 체력을 다시 만땅(5)으로 채웁니다.
	currentHp = 5;

	// 3. 금이 가 있던 스프라이트 이미지를 다시 멀쩡한 풀 피(5번 인덱스) 이미지로 원상복구 시킵니다.
	if (spriteComponent && DamagedSprites.IsValidIndex(currentHp))
	{
		spriteComponent->SetSprite(DamagedSprites[currentHp]);
	}
}

void AObstacleBlock::OnInteracted(class ADrillerCharacter* Player)
{
	if (currentState != EBlockState::Idle) return;
	
	currentHp--;
	
	//  체력이 깎였으니 무조건 소리를 냅니다!
	if (HitSound)
	{
		UGameplayStatics::PlaySound2D(this, HitSound, 1.0f, FMath::RandRange(0.9f, 1.1f)); // 피치를 살짝 흔들면 타격감이 더 좋습니다.
	}
	
	if (currentHp > 0)
	{
		// [스프라이트 변경] 남은 체력(currentHp)에 맞는 이미지를 배열에서 꺼내 입힙니다!
		if (spriteComponent && DamagedSprites.IsValidIndex(currentHp))
		{
			spriteComponent->SetSprite(DamagedSprites[currentHp]);
		}
	}
	else
	{
		// 1. 산소 패널티 부여 (안전한 널 체크 적용)
		if (statusSubsystemRef)
		{
			statusSubsystemRef->ConsumeOxygen(20.0f);
		}
       
		// 2. [중요] 체력이 0이 되었으므로 파괴!
		Pop();
	}
}
