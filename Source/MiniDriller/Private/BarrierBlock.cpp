// Fill out your copyright notice in the Description page of Project Settings.


#include "BarrierBlock.h"
#include "DrillerCharacter.h"
#include "GameStatusSubsystem.h"
#include "MapManager.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"

// Sets default values
ABarrierBlock::ABarrierBlock(){}

// Called when the game starts or when spawned
void ABarrierBlock::BeginPlay()
{
	Super::BeginPlay();
}

void ABarrierBlock::OnInteracted(ADrillerCharacter* instigator)
{
	if (currentState != EBlockState::Idle) return;
	
	// 1. 타격감 사운드 및 점수 부여
	if (statusSubsystemRef)
	{
		// 배리어 파괴 시 엄청난 폭발음을 위해 사운드 중첩(15)과 보너스 점수(1000) 지급
		statusSubsystemRef->PlayBlockPopSFX(popSound, 15);
		statusSubsystemRef->AddScore(1000); 
		statusSubsystemRef->LevelUp();
	}

	// 2. 🌟 [충돌 방지 핵심] 기존의 즉시 삭제/스폰 코드를 지우고, 
	// 매니저에게 내 좌표를 주며 "방사형 폭발 후 알아서 새 맵 깔아주세요!" 라고 위임합니다.
	if (mapManagerRef)
	{
		mapManagerRef->ClearScreenBlocksRadially(gridX, gridY);
	}
}

void ABarrierBlock::ForcePop()
{
	// 시각적 파괴만 진행
	SetActorEnableCollision(false);
	
	if (spriteComponent) 
	{
		spriteComponent->SetHiddenInGame(true, true);
	}
	
	if (destructionComponent)
	{
		destructionComponent->SetHiddenInGame(false);
		destructionComponent->PlayFromStart();
	}
	
	if (destructionEffectComponent)
	{
		destructionEffectComponent->SetHiddenInGame(false);
		destructionEffectComponent->PlayFromStart();
	}
}
