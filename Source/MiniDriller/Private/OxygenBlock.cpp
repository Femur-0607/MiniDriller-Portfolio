// Fill out your copyright notice in the Description page of Project Settings.


#include "OxygenBlock.h"
#include "DrillerCharacter.h"
#include "GameStatusSubsystem.h"
#include "PaperFlipbookComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AOxygenBlock::AOxygenBlock(){ }

// Called when the game starts or when spawned
void AOxygenBlock::BeginPlay()
{
	Super::BeginPlay();
}

void AOxygenBlock::OnInteracted(class ADrillerCharacter* Player)
{
	// [효과음 재생]
	if (HealSound) UGameplayStatics::PlaySound2D(this, HealSound);
    
	// 몸으로 닿았을 때와 똑같이 산소를 주고 파괴되도록 처리합니다.
	if (statusSubsystemRef)
	{
		statusSubsystemRef->AddOxygen(20.0f);
	}
	Pop();
}

void AOxygenBlock::NotifyActorBeginOverlap(AActor* OtherActor)
{
	// 나랑 겹친 액터가 '플레이어(ADrillerCharacter)'인지 확인 후 OnInteracted() 진행
	if (class ADrillerCharacter* Player = Cast<ADrillerCharacter>(OtherActor))
	{
		OnInteracted(Player);
	}
}

void AOxygenBlock::ResetBlockState()
{
	// 1. 부모(ABlock)의 기본 복구 로직(충돌체 켜기 등)을 먼저 실행합니다.
	Super::ResetBlockState();

	// 2. 이 액터가 가지고 있는 모든 플립북 컴포넌트를 배열로 가져옵니다.
	TArray<UPaperFlipbookComponent*> Flipbooks;
	GetComponents<UPaperFlipbookComponent>(Flipbooks);

	// 3. 그 중에서 메인 애니메이션을 담당하는 플립북을 찾아 다시 켭니다!
	for (UPaperFlipbookComponent* FB : Flipbooks)
	{
		// 컴포넌트 이름으로 찾기
		if (FB->GetName().Contains(TEXT("PaperFlipbook")) && !FB->GetName().Contains(TEXT("Destruction")))
		{
			FB->SetHiddenInGame(false); // 다시 화면에 보이게 하고
			FB->PlayFromStart();        // 애니메이션을 처음부터 다시 재생!
		}
	}
}