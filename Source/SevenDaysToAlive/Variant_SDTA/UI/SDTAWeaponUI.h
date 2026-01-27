#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SDTAWeaponUI.generated.h"

/**
 * 武器UI，用于显示当前武器的弹药信息
 */
UCLASS()
class SEVENDAYSTOALIVE_API USDTAWeaponUI : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** 弹匣容量，用于UI绑定 */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon UI", Meta = (ExposeOnSpawn = true, DisplayName = "Magazine Size", OnChanged="OnMagazineSizeChanged"))
	int32 MagazineSize = 30;

	/** 当前弹药量，用于UI绑定 */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon UI", Meta = (ExposeOnSpawn = true, DisplayName = "Current Ammo", OnChanged="OnCurrentAmmoChanged"))
	int32 CurrentAmmo = 30;

	/** 准星是否可见 */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon UI", Meta = (ExposeOnSpawn = true, DisplayName = "Crosshair Visible", OnChanged="OnCrosshairVisibleChanged"))
	bool bCrosshairVisible = true;

	/** 击中反馈是否可见 */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon UI", Meta = (ExposeOnSpawn = true, DisplayName = "Hit Feedback Visible", OnChanged="OnHitFeedbackVisibleChanged"))
	bool bHitFeedbackVisible = false;

	/** 击中反馈持续时间（秒） */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon UI", Meta = (ExposeOnSpawn = true, DisplayName = "Hit Feedback Duration"))
	float HitFeedbackDuration = 0.2f;

protected:
	/** 弹匣容量变化时的回调 */
	UFUNCTION()
	void OnMagazineSizeChanged();

	/** 当前弹药量变化时的回调 */
	UFUNCTION()
	void OnCurrentAmmoChanged();

	/** 准星可见性变化时的回调 */
	UFUNCTION()
	void OnCrosshairVisibleChanged();

	/** 击中反馈可见性变化时的回调 */
	UFUNCTION()
	void OnHitFeedbackVisibleChanged();

public:
	/** 允许蓝图更新弹药计数器（保留用于向后兼容） */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon", meta=(DisplayName = "UpdateBulletCounter"))
	void BP_UpdateBulletCounter(int32 InMagazineSize, int32 InCurrentAmmo);

	/** 显示击中反馈 */
	UFUNCTION(BlueprintCallable, Category = "Weapon UI")
	void ShowHitFeedback();

	/** 隐藏击中反馈 */
	UFUNCTION(BlueprintCallable, Category = "Weapon UI")
	void HideHitFeedback();

private:
	/** 击中反馈定时器 */
	FTimerHandle HitFeedbackTimer;
};
