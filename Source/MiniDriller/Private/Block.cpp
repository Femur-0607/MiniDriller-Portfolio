// 파괴 및 중력 로직을 포함한 블록 액터 구현부


#include "Block.h"
#include "DrillerCharacter.h"
#include "GameStatusSubsystem.h"
#include "MapManager.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbookComponent.h"


// Sets default values
ABlock::ABlock()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// 태어날 때는 틱을 꺼둡니다. 바구니(Pool) 속 유령 틱 연산 원천 차단!
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	// 1. 형태가 없는 빈 씬 컴포넌트를 생성하고 루트(중심)로 설정합니다.
	USceneComponent* defaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = defaultRoot;

	// 2. 스프라이트를 생성하고 루트 컴포넌트의 자식으로 부착합니다.
	// 스프라이트를 상위로 할 경우 현재 위치한 로컬을 받아오므로 빈 상위 컴포넌트를 만들어야 효과(흔들림)을 구현할 수 있음
	spriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("MySprite"));
	spriteComponent->SetupAttachment(RootComponent);
	
	// 3. 파괴 이펙트(플립북) 컴포넌트 부착 및 초기 세팅
	destructionComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Destruction"));
	destructionComponent->SetupAttachment(RootComponent);
	destructionComponent->SetLooping(false); // 반복 재생 금지
	
	// 3. 파괴효과 이펙트(플립북) 컴포넌트 부착 및 초기 세팅
	destructionEffectComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("DestructionEffect"));
	destructionEffectComponent->SetupAttachment(destructionComponent);
	destructionEffectComponent->SetLooping(false); // 반복 재생 금지
}

// Called when the game starts or when spawned
void ABlock::BeginPlay()
{
	Super::BeginPlay();
	
	statusSubsystemRef = GetWorld()->GetSubsystem<UGameStatusSubsystem>();
	
	// 시작할 때는 이펙트가 안 보이게 숨겨둡니다.
	if (destructionComponent || destructionEffectComponent)
	{
		destructionComponent->SetHiddenInGame(true, true);
        
		// 애니메이션이 끝났을 때(OnFinishedPlaying) 내 함수(OnDestructionEffectFinished)를 실행하도록 연결합니다.
		//OnFinishedPlaying는 사운드, 비디오, 애니메이션 또는 레벨 시퀀스가 재생을 마치고 끝에 도달했을때 실행되는 콜백 함수나 이벤트
		destructionComponent->OnFinishedPlaying.AddDynamic(this, &ABlock::OnDestructionEffectFinished);
	}
}

void ABlock::OnDestructionEffectFinished()
{
	// 애니메이션이 완전히 끝났으므로, 이제 MapManager에게 나를 회수(Pool)하라고 알립니다.
	if (onBlockDestroyedDelegate.IsBound())
	{
		onBlockDestroyedDelegate.Broadcast(this);
	}

	// 풀에 들어갔다가 다시 나올 때를 대비해 상태를 초기화합니다.
	destructionComponent->SetHiddenInGame(true,true);
}

void ABlock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    
	switch (currentState)
	{
	case EBlockState::Idle:
		break;
          
	case EBlockState::Anticipating:
		// * 50.f (주파수/속도): 시간값에 큰 수를 곱하여 \(\sin \) 함수의 주기를 빠르게 만듭니다. 이 숫자가 클수록 진동이 빨라집니다.
		// * 3.f (진폭/세기): \(\sin \) 함수의 결과값(\(-1 \sim 1\))에 곱해져 최종적인 진동의 크기(범위)를 결정합니다. 여기서는 \(-3.0\)에서 \(3.0\) 사이의 값을 가집니다.
		shakeOffset = FMath::Sin(GetWorld()->GetTimeSeconds() * 50.f) * 3.f;
		spriteComponent->SetRelativeLocation(FVector(0.f, 0.f, shakeOffset));
		break;

	case EBlockState::Falling:
		FVector currentLocation = GetActorLocation();

		// [시각적 연출] 설정된 targetLocation까지 프레임마다 조금씩 이동
		FVector nextLocation = FMath::VInterpConstantTo(currentLocation, targetLocation, DeltaTime, 400.f);
		SetActorLocation(nextLocation);

		// [도착 판정] 목표 지점에 거의 다 왔다면?
		// Dist() 함수는 주로 프로그래밍 언어나 통계 패키지에서 데이터 포인트(행)들 간의 거리(유사도)나 특정 확률 분포의 값을 계산할 때 사용합니다.
		if (FVector::Dist(nextLocation, targetLocation) < 1.0f)
		{
			SetActorLocation(targetLocation); // 그리드 오차 스냅
			currentState = EBlockState::Idle; // 편안하게 휴식
			SetActorTickEnabled(false);       // 심장 정지 (대기)
             
			// ⭐ [연쇄 파괴의 핵심] 새로운 자리에 멈췄으니, 여기서 또 4개가 모였는지 검사!
			CheckMatch(); 
		}
		break;
	}
}

void ABlock::ResetBlockState()
{
	// 낙하 상태를 완전히 끕니다.
	currentState = EBlockState::Idle;
	SetActorTickEnabled(false);
	
	// (만약 타이머를 써서 움직였다면 타이머도 여기서 클리어 해줍니다)
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	
	// 🌟 [버그 픽스] 바구니로 돌아가기 전, 스프라이트의 위치를 완벽한 정중앙(0,0,0)으로 교정합니다!
	if (spriteComponent)
	{
		spriteComponent->SetRelativeLocation(FVector::ZeroVector);
	}
	
	// 재생 중이던 파괴 애니메이션 강제 정지 (이중 풀링 방지)
	if (destructionComponent) {
		destructionComponent->Stop();
	}
	if (destructionEffectComponent) {
		destructionEffectComponent->Stop();
	}
	
	if (spriteComponent)
	{
		spriteComponent->SetHiddenInGame(false, false);
	}
	
	// [추가] 유령 좌표 반납
	gridX = -1;
	gridY = -1;
}

void ABlock::OnInteracted(class ADrillerCharacter* Player)
{
	// 편안하게 쉬고 있는(Idle) 상태가 아니면, 곡괭이질을 무시합니다!
	// (공중에서 흔들리거나 추락 중인 블록은 파괴 불가)
	if (currentState != EBlockState::Idle) return;
	
	// 1. 같은 색상 블록들을 담을 바구니(TSet)를 준비합니다.
	TSet<ABlock*> connectedBlocks;
	// 2. [중요] 레이캐스트 대신, 우리가 리팩토링한 Grid 기반 FloodFill을 실행합니다!ResetBlockState
	// 내 색상과 똑같은 인접 블록들을 'connectedBlocks'에 싹 다 담아옵니다.
	ExecuteFloodFill(this->blockColor, this, connectedBlocks);
	
	// [점수 시스템] 
	if (connectedBlocks.Num() > 0)
	{
		if (statusSubsystemRef)
		{
			// 4개 이상 터지는 콤보 상황이고 콤보 사운드가 등록되어 있다면 comboSound를, 아니라면 일반 popSound를 서브시스템에 보냅니다!
			USoundBase* SoundToPlay = (connectedBlocks.Num() >= 4 && comboSound) ? comboSound : popSound;
			statusSubsystemRef->ProcessBlockDestructionScore(connectedBlocks.Num(), false, SoundToPlay);
		}
	}

	// 3. 바구니에 담긴 블록들을 한꺼번에 팝
	// 4개 미만이어도 플레이어가 직접 캔 것이므로 연결된 건 다 터뜨림
	for (ABlock* block : connectedBlocks)
	{
		if (block)
		{
			block->Pop();
		}
	}
}

void ABlock::CheckMatch()
{
	// 색상이 없는 특수 블록(산소, 장애물)은 연쇄 파괴(Match) 검사를 패스합니다!
	if (blockColor == EBlockColor::None) return;
	
	TSet<ABlock*> matchedBlocks;
	// 재귀 함수 실행 (내 색상, 나 자신, 빈 상자 전달)
	ExecuteFloodFill(this->blockColor, this, matchedBlocks);
    
	// 4개 이상 모였는지 판정!
	if (matchedBlocks.Num() >= 4)
	{
		if (statusSubsystemRef)
		{
			// 4개 이상 터지는 콤보 상황이고 콤보 사운드가 등록되어 있다면 comboSound를, 아니라면 일반 popSound를 서브시스템에 보냅니다!
			USoundBase* SoundToPlay = (matchedBlocks.Num() >= 4 && comboSound) ? comboSound : popSound;
			statusSubsystemRef->ProcessBlockDestructionScore(matchedBlocks.Num(), false, SoundToPlay);
		}

		// 연쇄 반응 시작!
		for (ABlock* block : matchedBlocks)
		{
			block->Pop(); 
		}
	}
}

void ABlock::Pop()
{
	// 1. 사장님(mapManager)을 모셔옵니다.
	if (mapManagerRef)
	{
		int32 SavedX = gridX;
		int32 SavedY = gridY;
       
		// 2. 사장님! 저 장부에서 지워주시고, 제 위에 있는 애들 떨어뜨려 주세요!
		mapManagerRef->RemoveBlockFromGrid(SavedX, SavedY);
		mapManagerRef->ProcessFalling(SavedX, SavedY);
       
		// 3. [유령 버그 방지] 나는 이제 장부에 없으니 사원증(좌표) 반납!
		gridX = -1;
		gridY = -1;
	}
    
	// 파괴 이펙트 재생 및 충돌 끄기
	if (destructionComponent || destructionEffectComponent)
	{
		destructionComponent->SetHiddenInGame(false);
		destructionEffectComponent->SetHiddenInGame(false);
		destructionComponent->PlayFromStart();
		destructionEffectComponent->PlayFromStart();
	}
	
	GetWorld()->GetTimerManager().ClearTimer(anticipationTimerHandle);
	spriteComponent->SetHiddenInGame(true, true);
	SetActorEnableCollision(false);
}

void ABlock::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	// 1. 부딪힌 대상이 존재하고, 현재 블록이 '낙하 중(Falling)'일 때만 검사합니다.
	if (OtherActor != nullptr && currentState == EBlockState::Falling)
	{
		TryKillPlayer(OtherActor);
	}
}

void ABlock::TryKillPlayer(AActor* TargetActor)
{
	// 1. 방어 코드 (null 체크)
	if (TargetActor == nullptr) return;

	// 2. 플레이어인지 확인하고 맞다면 처형!
	if (ADrillerCharacter* Player = Cast<ADrillerCharacter>(TargetActor))
	{
		Player->HandleDeath();
	}
}

void ABlock::PrepareToFall(FVector NewTargetLocation)
{
	targetLocation = NewTargetLocation;
	currentState = EBlockState::Anticipating; // 흔들림 시작!
	SetActorTickEnabled(true);

	// 1.5초 정도 대기 후 실제 낙하 시작
	float Delay = 1.5f; 
	GetWorld()->GetTimerManager().SetTimer(anticipationTimerHandle, this, &ABlock::ActuallyStartFalling, Delay, false);
}

void ABlock::ActuallyStartFalling()
{
	currentState = EBlockState::Falling;
	spriteComponent->SetRelativeLocation(FVector::ZeroVector); // 흔들림 오프셋 리셋
	// 낙하를 시작하는 순간, 이미 내 영역에 들어와서 겹쳐있는 액터가 있는지 즉시 검사!
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	
	for (AActor* Actor : OverlappingActors)
	{
		TryKillPlayer(Actor);
	}
}

void ABlock::SetBlockColor(EBlockColor NewColor, class UPaperSprite* NewSprite,class UPaperFlipbook* NewFlipbook, class UPaperFlipbook* NewEffectFlipbook)
{
	blockColor = NewColor;
    
	// 1. 스프라이트 교체 (스프라이트 에셋이 정상적으로 들어왔을 때만)
	if (spriteComponent && NewSprite)
	{
		spriteComponent->SetSprite(NewSprite);
	}

	// 2. 파괴플립북 교체 (플립북 에셋이 정상적으로 들어왔을 때만)
	if (destructionComponent && NewFlipbook)

	{
		destructionComponent->SetFlipbook(NewFlipbook);
	}
	
	// 2. 파괴효과플립북 교체 (플립북 에셋이 정상적으로 들어왔을 때만)
	if (destructionEffectComponent && NewEffectFlipbook)

	{
		destructionEffectComponent->SetFlipbook(NewEffectFlipbook);
	}
}
// Block.cpp
void ABlock::ExecuteFloodFill(EBlockColor TargetColor, ABlock* CurrentBlock, TSet<ABlock*>& OutMatchedBlocks){
	// [단계 1: 검사 거르기] 
	// 빈 공간이거나, 색깔이 다르거나, '현재 공중에서 떨어지거나 흔들리는 상태(Not Idle)이거나', 이미 찾은 명단에 있으면 패스!
	if (CurrentBlock == nullptr || 
		CurrentBlock->blockColor != TargetColor || 
		CurrentBlock->currentState != EBlockState::Idle ||
		OutMatchedBlocks.Contains(CurrentBlock)) return;

	// [단계 2: 명단 등록] 현재 블록을 "찾은 친구들" 바구니에 담음
	OutMatchedBlocks.Add(CurrentBlock);
    
	// [단계 3: 상하좌우 좌표 계산] 
	// 내 좌표 (gridX, gridY)를 기준으로 동서남북 좌표값만 계산함
	if (!mapManagerRef) return;

	// 상, 하, 좌, 우 4방향의 격자 좌표 오프셋
	FIntPoint Dirs[4] = { 
		FIntPoint(0, 1),  // 하
		FIntPoint(0, -1), // 상
		FIntPoint(1, 0),  // 우
		FIntPoint(-1, 0)  // 좌
	};
    
	for (int32 i = 0; i < 4; ++i){
		int32 NextX = CurrentBlock->gridX + Dirs[i].X;
		int32 NextY = CurrentBlock->gridY + Dirs[i].Y;
		
		// [단계 4: 장부 조회] 사장님께 해당 좌표에 누가 있는지 물어봄
		ABlock* NextBlock = mapManagerRef->GetBlockAtGrid(NextX, NextY);
        
		// [단계 5: 재귀] 그 친구에게 가서 다시 1단계부터 실행해라! (퍼져나가는 방식)
		ExecuteFloodFill(TargetColor, NextBlock, OutMatchedBlocks);
	}
}