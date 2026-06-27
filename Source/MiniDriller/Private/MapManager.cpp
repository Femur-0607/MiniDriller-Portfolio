// 블록 풀링과 라인 재배치를 담당하는 맵 매니저 구현부


#include "MapManager.h"
#include "Block.h"
#include "ObstacleBlock.h"
#include "OxygenBlock.h"
#include "BarrierBlock.h"
#include "DrillerCharacter.h"
#include "GameStatusSubsystem.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AMapManager::AMapManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AMapManager::BeginPlay()
{
	Super::BeginPlay();
	
	// 1. 게임 시작 시 바구니를 블록으로 가득 채웁니다.
	InitializePools();

	// 2. 초기 맵 세팅 (예: 시작할 때 화면에 MaxRows만큼 생성)
	for (int32 i = CurrentStartRow; i < CurrentStartRow + LevelHeight; ++i)
	{
		SpawnNewRow(i);
	}
}
//MapManager.cpp
void AMapManager::InitializePools()
{
	// 각 타입별로 Count만큼 미리 생성
	auto preSpawn = [&](TSubclassOf<ABlock> Class, TArray<ABlock*>& Pool, int32 Count) {
		for (int32 i = 0; i < Count; ++i) {
			ABlock* newBlock = GetWorld()->SpawnActor<ABlock>(Class, FVector::ZeroVector, FRotator::ZeroRotator);
			if (newBlock) {
				newBlock->SetActorHiddenInGame(true);
				newBlock->SetActorEnableCollision(false);
				newBlock->onBlockDestroyedDelegate.AddUObject(this, &AMapManager::ReturnBlockToPool);
				Pool.Add(newBlock);
			}
		}
	};

	preSpawn(normalBlockClass, normalPool, normalPoolSize);
	preSpawn(oxygenBlockClass, oxygenPool, oxygenPoolSize);
	preSpawn(obstacleBlockClass, obstaclePool, obstaclePoolSize);
	preSpawn(barrierBlockClass, barrierPool, barrierPoolSize);
}

ABlock* AMapManager::GetBlockFromPool(EBlockType Type)
{
	TArray<ABlock*>* targetPool = nullptr;
	TSubclassOf<ABlock> targetClass;

	// 타입에 따라 알맞은 바구니(Pool)를 선택합니다.
	switch (Type) 
	{
		case EBlockType::Normal:   targetPool = &normalPool;   targetClass = normalBlockClass; break;
		case EBlockType::Oxygen:   targetPool = &oxygenPool;   targetClass = oxygenBlockClass; break;
		case EBlockType::Obstacle: targetPool = &obstaclePool; targetClass = obstacleBlockClass; break;
		case EBlockType::Barrier:  targetPool = &barrierPool;  targetClass = barrierBlockClass; break;
	}

	ABlock* Block = nullptr;

	// 1. 바구니에 남은 블록이 있다면 하나 꺼내서 재사용합니다.
	if (targetPool && targetPool->Num() > 0)
	{
		Block = targetPool->Pop();
		if (Block)
		{
			Block->ResetBlockState();
		}
	}
	// 2. 바구니가 비어있다면 새로 생성합니다.
	else if (targetClass)
	{
		Block = GetWorld()->SpawnActor<ABlock>(targetClass, FVector::ZeroVector, FRotator::ZeroRotator);
		
		// 새로 생성된 블록도 나중에 파괴되면 반드시 풀로 돌아오도록 델리게이트를 연결합니다!
		// AddUObject()는 언리얼 엔진(Unreal Engine)에서 UObject 기반의 C++ 멤버 함수를 델리게이트(Delegate)에 바인딩(등록)할 때 사용하는 함수
		if (Block)
		{
			Block->onBlockDestroyedDelegate.AddUObject(this, &AMapManager::ReturnBlockToPool);
		}
	}

	// 3. 꺼내거나 새로 만든 블록을 활성화 상태로 초기화하여 반환합니다.
	if (Block)
	{
		Block->bIsInPool = false;
		Block->SetActorHiddenInGame(false);
		Block->SetActorEnableCollision(true);
		return Block;
	}

	return Block;
}

void AMapManager::ReturnBlockToPool(ABlock* returnedBlock)
{
	if (returnedBlock == nullptr || returnedBlock->bIsInPool) return;
	
	// 바구니에 들어간다고 이름표를 갱신해 줍니다.
	returnedBlock->bIsInPool = true;
	
	// 바구니에 넣기 전에 모든 과거의 기억(상태, 위치)을 깨끗하게 포맷합니다!
	returnedBlock->ResetBlockState();
	returnedBlock->SetActorHiddenInGame(true);
	returnedBlock->SetActorEnableCollision(false);

	// [안전장치] 에디터에서 클래스 할당이 누락되었을 경우를 대비해 null 체크를 추가합니다.
	if (oxygenBlockClass && returnedBlock->IsA(oxygenBlockClass)) 
	{
		oxygenPool.Add(returnedBlock);
	} 
	else if (obstacleBlockClass && returnedBlock->IsA(obstacleBlockClass)) 
	{
		obstaclePool.Add(returnedBlock);
	} 
	else if (barrierBlockClass && returnedBlock->IsA(barrierBlockClass)) 
	{
		barrierPool.Add(returnedBlock);
	}
	else 
	{
		normalPool.Add(returnedBlock);
	}
}

void AMapManager::SpawnNewRow(int32 RowIndex)
{
	for (int32 Col = 0; Col < MaxCols; ++Col) 
	{ 
		int32 LocalRow = RowIndex % LevelHeight; // 0 ~ LevelHeight 사이의 반복
		
		EBlockType selectedType = EBlockType::Normal;
		
		// 현재 줄이 하단부 줄(BarrierThickness)에 속한다면 배리어 스폰!
		bool bIsBarrierRow = (RowIndex > 0) && (LocalRow >= (LevelHeight - BarrierThickness));
		
		// 배리어 줄이라면 무조건 배리어를 스폰, 아니라면 기존 확률 적용
		if (bIsBarrierRow)
		{
			selectedType = EBlockType::Barrier;
		}
		else
		{
			float rand = FMath::FRand(); 
			
			// 서브시스템에서 현재 레벨을 읽어옵니다.
			int32 Level = 1;
			if (UGameStatusSubsystem* Status = GetWorld()->GetSubsystem<UGameStatusSubsystem>())
			{
				Level = Status->currentLevel;
			}

			// [난이도 스케일링] 장애물 확률: 기본 10% + (레벨당 2% 증가). 단, 최대 30%까지만!
			float ObstacleProb = FMath::Clamp(0.10f + (Level * 0.02f), 0.10f, 0.30f);

			// 산소는 고정 5%, 장애물은 점진적 증가
			if (rand < 0.05f) 
			{
				selectedType = EBlockType::Oxygen;   
			}
			else if (rand < 0.05f + ObstacleProb) 
			{
				selectedType = EBlockType::Obstacle; 
			}
		}
		ABlock* newBlock = GetBlockFromPool(selectedType);
		if (newBlock)
		{
			// 일반 블록일 경우에만 색상과 스프라이트를 랜덤으로 부여합니다!
			if (selectedType == EBlockType::Normal)
			{
				// 1(Blue) ~ 4(Yellow) 사이의 랜덤한 색상 인덱스 추출
				int32 ColorIndex = FMath::RandRange(1, 4);
				EBlockColor randomColor = static_cast<EBlockColor>(ColorIndex);

				// 에디터에서 할당한 배열(크기 4)에서 꺼내오기 위해 인덱스 - 1을 해줍니다 (0~3)
				UPaperSprite* SelectedSprite = BlockSprites.IsValidIndex(ColorIndex - 1) ? BlockSprites[ColorIndex - 1] : nullptr;
				UPaperFlipbook* SelectedFlipbook = BlockDestructionFlipbooks.IsValidIndex(ColorIndex - 1) ? BlockDestructionFlipbooks[ColorIndex - 1] : nullptr;
				UPaperFlipbook* SelectedEffectFlipbook = BlockEffectFlipbooks.IsValidIndex(ColorIndex - 1) ? BlockEffectFlipbooks[ColorIndex - 1] : nullptr;

				// 블록에 랜덤 색상과 스프라이트 적용
				newBlock->SetBlockColor(randomColor, SelectedSprite, SelectedFlipbook, SelectedEffectFlipbook);
			}
			
			// 1. 물리적 위치 설정 (Z축을 밑으로 내림)
			FVector SpawnPos = FVector(Col * TileSize, 0.0f, -RowIndex * TileSize);
			newBlock->SetActorLocation(SpawnPos);

			// 2. [매우 중요] 매니저 참조를 넘겨줍니다. 그래야 블록이 매니저의 장부에 접근할 수 있습니다.
			newBlock->mapManagerRef = this;

			// 3. 매니저의 장부에 기록합니다.
			RegisterBlock(Col, RowIndex, newBlock);
		}
	}
}

void AMapManager::ResetAndSpawnNextLevel()
{
	// 1. 기준 줄 번호를 다음 레벨로 갱신 (예: 0 -> 50 -> 100)
	CurrentStartRow += LevelHeight;

	// 2. 갱신된 발밑 위치부터 새로운 맵을을 쫙 깔아줍니다!
	for (int32 i = CurrentStartRow; i < CurrentStartRow + LevelHeight; ++i)
	{
		SpawnNewRow(i);
	}
	
	// 맵 스폰이 끝났으니 플레이어의 중력을 돌려주고, 끼임 방지 보정을 넣습니다.
	if (ADrillerCharacter* player = Cast<ADrillerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		player -> OnMapTransitionComplete();
	}
}
// MapManager.cpp
void AMapManager::ClearScreenBlocksRadially(int32 CenterX, int32 CenterY){
	// 이미 방사형 폭발이 진행 중이라 장부가 비워졌다면 즉시 무시합니다! (연속 타격 크래시 방지)
	if (GridMap.IsEmpty()) return;
	
	// 폭발 연출 동안 플레이어를 허공에 띄워둡니다! (타임 스탑)
	if (ADrillerCharacter* player = Cast<ADrillerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
		player -> OnMapTransitionStart();
	
	float MaxDelay = 0.0f;
	
	// 🌟 [스냅샷 패턴]
	// TMap을 순회하면서 객체를 건드리면 이터레이터가 깨집니다. 
	// 안전하게 TArray(배열)로 값만 쏙 빼서 복사해옵니다.
	// GenerateValueArray()는 언리얼 엔진(Unreal Engine) C++에서 TMap 컨테이너의 모든 값(Value)들을 TArray 형태로 추출할 때 사용하는 함수입니다.
	TArray<ABlock*> BlocksToClear;
	GridMap.GenerateValueArray(BlocksToClear);
	
	// 장부는 이제 아무도 건드릴 수 없게 미리 백지화합니다.
	GridMap.Empty();

	// 복사된 안전한 배열을 순회하며 폭발과 청소를 지시합니다.
	for (ABlock* Block : BlocksToClear){
		if (IsValid(Block)){
			if (ABarrierBlock* Barrier = Cast<ABarrierBlock>(Block)) {
				int32 Dist = FMath::Abs(Barrier->gridX - CenterX) + FMath::Abs(Barrier->gridY - CenterY);
				float Delay = Dist * 0.05f; 

				if (Delay > MaxDelay) MaxDelay = Delay; 

				if (Delay > 0.0f){
					FTimerHandle TempHandle;
					GetWorld()->GetTimerManager().SetTimer(TempHandle, Barrier, &ABarrierBlock::ForcePop, Delay, false);
				}
				else{
					Barrier->ForcePop();
				}
			}
			else {
				// 일반 블록 청소
				Block->ResetBlockState();
				Block->SetActorHiddenInGame(true);
				Block->SetActorEnableCollision(false);
				ReturnBlockToPool(Block);
			}
		}
	}

	// 가장 멀리 있는 배리어가 터진 후(MaxDelay) + 0.1초 뒤 새 레벨 스폰!
	FTimerHandle ResetHandle;
	GetWorld()->GetTimerManager().SetTimer(ResetHandle, this, &AMapManager::ResetAndSpawnNextLevel, MaxDelay + 0.1f, false);
}
// MapManager.cpp
void AMapManager::RegisterBlock(int32 Col, int32 Row, ABlock* Block)
{
	if (Block != nullptr){
		// 1. 매니저의 장부(GridMap)에 기록합니다.
		GridMap.Add(FIntPoint(Col, Row), Block);
        
		// 2. 직원의 사원증(내부 변수)에 자기 자리를 각인시킵니다.
		Block->gridX = Col; 
		Block->gridY = Row;
	}
}

ABlock* AMapManager::GetBlockAtGrid(int32 Col, int32 Row)
{
	FIntPoint Pos(Col, Row);
	// 장부에 해당 좌표가 기록되어 있다면 그 블록을 반환하고, 아니면 빈 공간(nullptr)을 반환합니다.
	if (GridMap.Contains(Pos))
	{
		return GridMap[Pos];
	}
	return nullptr;
}
// MapManager.cpp
void AMapManager::ProcessFalling(int32 Col, int32 StartRow){
	// 현재 레벨의 진짜 바닥 높이 계산
	int32 currentBottom = CurrentStartRow + LevelHeight;
	
	// 터진 블록의 바로 윗칸(StartRow - 1)부터 맨 꼭대기(0)까지 역순으로 훑어 올라갑니다.
	for (int32 currentRow = StartRow - 1; currentRow >= 0; --currentRow){
		// 현재 칸에 블록이 있는가?
		ABlock* blockToFall = GetBlockAtGrid(Col, currentRow);
		if (blockToFall != nullptr){
			int32 targetRow = currentRow;
            
			// [핵심 로직] 내 밑칸(targetRow + 1)이 비어있고, 바닥(CurrentBottom)을 뚫지 않는 한 끝까지 탐색!
			while (IsGridEmpty(Col, targetRow + 1) && (targetRow + 1 < currentBottom)){
				targetRow++; // 빈칸 수만큼 목표 좌표를 아래로 낮춤
			}

			// 밑으로 한 칸이라도 내려갈 공간이 있다면 이동 지시!
			if (targetRow != currentRow){
				// [데이터 선점] 화면에서 움직이기 전에 장부부터 고침 (매우 중요!)
				RemoveBlockFromGrid(Col, currentRow); // 옛날 자리 지우고
				RegisterBlock(Col, targetRow, blockToFall); // 새 자리 예약

				// [연출 예약] 블록에게 "너 나중에 여기까지 내려와야 해"라고 목적지 전달
				FVector newTargetPos = FVector(Col * TileSize, 0.0f, -targetRow * TileSize);
				blockToFall->PrepareToFall(newTargetPos);
			}
		}
	}
}
// MapManager.cpp
bool AMapManager::IsGridEmpty(int32 Col, int32 Row){
	// 현재 레벨의 진짜 바닥 높이 계산
	int32 CurrentBottom = CurrentStartRow + LevelHeight;
	
	// 1. 맵의 양옆 벽이나 바닥(MaxRows)을 뚫고 내려가려 하면 "빈 공간이 아니다(false)"라고 막음
	if (Col < 0 || Col >= MaxCols || Row < 0 || Row >= CurrentBottom) { return false; }
    
	// 2. 장부(GridMap)에 해당 좌표(Col, Row)가 없으면(true) 빈 공간임!
	return !GridMap.Contains(FIntPoint(Col, Row));
}

