// 2차원 논리적 장부(GridMap)를 기반으로 블록의 데이터와 중력을 중앙 통제하며,
// 배리어 파괴 시 이전 블록들을 풀(Pool)로 회수하고 발밑으로 새로운 레벨을 생성하는 컨트롤 타워 클래스.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapManager.generated.h"

// 블록 타입을 관리하기 위한 열거형
UENUM(BlueprintType)
enum class EBlockType : uint8
{
	Normal,
	Oxygen,
	Obstacle,
	Barrier
};

UCLASS()
class MINIDRILLER_API AMapManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMapManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// --- 블럭 오브젝트 풀링 시스템 ---
#pragma region PoolSystem
protected:
	// 각 블록 타입별 초기 오브젝트 풀 생성 개수
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	int32 normalPoolSize = 200;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	int32 oxygenPoolSize = 50;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	int32 obstaclePoolSize = 50;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	int32 barrierPoolSize = 100;
public:
	// 1. 에디터에서 할당할 클래스 종류들
	UPROPERTY(EditAnywhere, Category = "Pool | Classes")
	TSubclassOf<class ABlock> normalBlockClass;
	UPROPERTY(EditAnywhere, Category = "Pool | Classes")
	TSubclassOf<class AOxygenBlock> oxygenBlockClass;
	UPROPERTY(EditAnywhere, Category = "Pool | Classes")
	TSubclassOf<class AObstacleBlock> obstacleBlockClass;
	UPROPERTY(EditAnywhere, Category = "Pool | Classes")
	TSubclassOf<class ABarrierBlock> barrierBlockClass;
	// 2. 비활성화된 블록들을 담아둘 바구니 (Pools)
	TArray<ABlock*> normalPool;
	TArray<ABlock*> oxygenPool;
	TArray<ABlock*> obstaclePool;
	TArray<ABlock*> barrierPool;
	
	// 풀 초기화 함수
	void InitializePools();
	// 풀에서 블록을 꺼내오는 핵심 함수
	ABlock* GetBlockFromPool(EBlockType Type);
	// 블록이 Pop 되었을 때 다시 풀로 돌려받는 함수
	UFUNCTION()
	void ReturnBlockToPool(ABlock* returnedBlock);
#pragma endregion
	// --- 맵 생성 및 레벨 시스템 ---
#pragma region Level & Spawn System
public:
	// 에디터에서 [0]:파랑, [1]:초록, [2]:빨강, [3]:노랑 스프라이트를 직접 할당할 배열
	UPROPERTY(EditAnywhere)
	TArray<class UPaperSprite*> BlockSprites;
	// 에디터에서 [0]:파랑, [1]:초록, [2]:빨강, [3]:노랑 플립북(파괴 이펙트)를 직접 할당할 배열
	UPROPERTY(EditAnywhere)
	TArray<class UPaperFlipbook*> BlockDestructionFlipbooks;
	// 에디터에서 [0]:파랑, [1]:초록, [2]:빨강, [3]:노랑 플립북(파괴효과 이펙트)를 직접 할당할 배열
	UPROPERTY(EditAnywhere)
	TArray<class UPaperFlipbook*> BlockEffectFlipbooks;
	// 맵의 가로 세로 최대 크기 변수
	UPROPERTY(EditAnywhere)
	int32 MaxRows = 10; 
	UPROPERTY(EditAnywhere)
	int32 MaxCols = 10;
	UPROPERTY(EditAnywhere)
	int32 BarrierThickness = 5;
	
	// 기존의 줄 생성 로직에 확률을 더합니다.
	void SpawnNewRow(int32 RowIndex);
	// 화면이 지워진 직후, 플레이어 발밑에 다음 레벨의 블록을 즉시 스폰하는 함수
	void ResetAndSpawnNextLevel();
	// 배리어 파괴 시 중심 좌표를 기준으로 방사형 연쇄 폭발을 일으킵니다.
	void ClearScreenBlocksRadially(int32 CenterX, int32 CenterY); 
#pragma endregion
	// --- 중앙 장부 및 중력 제어 시스템 ---
#pragma region Grid & Logic System
public:
	// MapManager.h
	// FIntPoint 언리얼에서 사용하는 정수형 좌표(2D 벡터)를 표현하는 구조체
	UPROPERTY()
	TMap<FIntPoint, class ABlock*> GridMap; 
	const float TileSize = 42.0f; // 타일의 기준 크기
	// 현재 맵이 시작되는 기준 줄(Row) 번호 (초기값 0)
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentStartRow = 0;
	// 1레벨(구간)당 줄 개수
	UPROPERTY(EditAnywhere)
	int32 LevelHeight = 20;
	
	void RegisterBlock(int32 Col, int32 Row, class ABlock* Block);  // 블록들을 장부에 기록함
	// 장부에서 좌표를 지우는 함수
	void RemoveBlockFromGrid(int32 Col, int32 Row) { GridMap.Remove(FIntPoint(Col, Row)); }
	class ABlock* GetBlockAtGrid(int32 Col, int32 Row); // 장부를 읽는 함수
	void ProcessFalling(int32 Col, int32 StartRow); // 장부를 보고 블록 하락 시스템을 지시하는 역활
	bool IsGridEmpty(int32 Col, int32 Row); // 레벨의 바닥을 계산하고 블록이 맵 밖으로 이탈하는 것을 방지함
	
#pragma endregion
};