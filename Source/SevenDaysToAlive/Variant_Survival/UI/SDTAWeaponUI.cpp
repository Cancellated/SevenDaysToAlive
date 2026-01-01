#include "Variant_Survival/UI/SDTAWeaponUI.h"
#include "Engine/World.h"
#include "TimerManager.h"

/** 弹匣容量变化时的回调实现 */
void USDTAWeaponUI::OnMagazineSizeChanged()
{
	BP_UpdateBulletCounter(MagazineSize, CurrentAmmo);
}

/** 当前弹药量变化时的回调实现 */
void USDTAWeaponUI::OnCurrentAmmoChanged()
{
	BP_UpdateBulletCounter(MagazineSize, CurrentAmmo);
}

/** 准星可见性变化时的回调实现 */
void USDTAWeaponUI::OnCrosshairVisibleChanged()
{
	// 准星可见性变化时的逻辑可以在蓝图中实现
}

/** 击中反馈可见性变化时的回调实现 */
void USDTAWeaponUI::OnHitFeedbackVisibleChanged()
{
	// 击中反馈可见性变化时的逻辑可以在蓝图中实现
}

/** 显示击中反馈 */
void USDTAWeaponUI::ShowHitFeedback()
{
	if (GetWorld())
	{
		// 清除之前的定时器
		GetWorld()->GetTimerManager().ClearTimer(HitFeedbackTimer);
		
		// 设置击中反馈可见
		bHitFeedbackVisible = true;
		
		// 设置定时器在指定时间后隐藏击中反馈
		GetWorld()->GetTimerManager().SetTimer(
			HitFeedbackTimer,
			this,
			&USDTAWeaponUI::HideHitFeedback,
			HitFeedbackDuration,
			false
		);
	}
}

/** 隐藏击中反馈 */
void USDTAWeaponUI::HideHitFeedback()
{
	bHitFeedbackVisible = false;
}
