// Fill out your copyright notice in the Description page of Project Settings.

#include "Variant_SDTA/UI/Upgrade/SDTAUpgradeUIController.h"
#include "Variant_SDTA/Characters/SDTAPlayerBase.h"
#include "Kismet/GameplayStatics.h"

void USDTAUpgradeUIController::SetUpgradeOptions(TArray<USDTAUpgradeUIModel*> UpgradeOptions)
{
    CurrentUpgradeOptions = UpgradeOptions;
    
    // 获取本地玩家
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        TargetPlayer = PC->GetPawn();
    }
}

void USDTAUpgradeUIController::ShowUpgradeUI()
{
    if (IsInViewport())
    {
        return;
    }
    
    AddToViewport();
    SetVisibility(ESlateVisibility::Visible);
    
    // 暂停游戏（如果需要）
    // UGameplayStatics::SetGamePaused(GetWorld(), true);
}

void USDTAUpgradeUIController::HideUpgradeUI()
{
    if (!IsInViewport())
    {
        return;
    }
    
    RemoveFromParent();
    SetVisibility(ESlateVisibility::Hidden);
    
    // 恢复游戏（如果需要）
    // UGameplayStatics::SetGamePaused(GetWorld(), false);
    
    // 触发关闭委托
    OnUpgradeUIClosed.Broadcast();
}

void USDTAUpgradeUIController::SelectUpgradeOption(int32 OptionIndex)
{
    if (OptionIndex < 0 || OptionIndex >= CurrentUpgradeOptions.Num())
    {
        return;
    }
    
    USDTAUpgradeUIModel* SelectedUpgrade = CurrentUpgradeOptions[OptionIndex];
    if (SelectedUpgrade && TargetPlayer)
    {
        // 检查是否有足够的灵魂
        if (HasEnoughSouls(SelectedUpgrade->SoulCost))
        {
            // 扣除灵魂
		SoulFragments -= SelectedUpgrade->SoulCost;
            
            // 应用升级
            SelectedUpgrade->ApplyUpgrade(TargetPlayer);
            
            // 触发选择委托
            OnUpgradeSelected.Broadcast(SelectedUpgrade);
            
            // 隐藏UI
            HideUpgradeUI();
        }
        else
        {
            // 灵魂不足，这里可以添加提示逻辑
            // 例如：显示"灵魂不足"的文本提示
        }
    }
}
TArray<USDTAUpgradeUIModel*> USDTAUpgradeUIController::GetCurrentUpgradeOptions() const
{
    return CurrentUpgradeOptions;
}

void USDTAUpgradeUIController::SetSoulFragments(int32 NewSoulFragments)
{
	SoulFragments = NewSoulFragments;
}

int32 USDTAUpgradeUIController::GetSoulFragments() const
{
	return SoulFragments;
}

void USDTAUpgradeUIController::RefreshUpgradeOptions()
{
    // 这里应该实现刷新升级选项的逻辑
    // 例如：从升级系统获取新的随机选项
    // 暂时留空，由蓝图或其他系统实现
}

bool USDTAUpgradeUIController::HasEnoughSouls(int32 Cost) const
{
	return SoulFragments >= Cost;
}