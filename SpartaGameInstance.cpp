#include "SpartaGameInstance.h"

USpartaGameInstance::USpartaGameInstance()
{
	TotalScore = 0;
	CurrentLevelIndex = 0;
	TotalTime = 0.0f;
	CollectCoin = 0;
}

void USpartaGameInstance::AddToScore(int32 Amount)
{
	TotalScore += Amount;
}

void USpartaGameInstance::AddToCollectCoin(int32 Amount)
{
	CollectCoin += Amount;
}

void USpartaGameInstance::AddToTotalTime(float Amount)
{
	TotalTime += Amount;
}
