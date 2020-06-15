#ifndef ARMXX_ISR_H
#define ARMXX_ISR_H

using int_handler = void (*)();

inline void InfiniteLoop()
{
    while ( 1 );
}

extern "C" void Default_Handler();

extern "C" void Reset_Handler();
void NMI_Handler();
void HardFault_Handler();
void MemManage_Handler();
void BusFault_Handler();
void UsageFault_Handler();

void SVC_Handler();
void DebugMon_Handler();

void PendSV_Handler();
void SysTick_Handler();
void WWDG_IRQHandler();
void PVD_IRQHandler();
void TAMPER_IRQHandler();
void RTC_IRQHandler();
void FLASH_IRQHandler();
void RCC_IRQHandler();
void EXTI0_IRQHandler();
void EXTI1_IRQHandler();
void EXTI2_IRQHandler();
void EXTI3_IRQHandler();
void EXTI4_IRQHandler();


void DMA1_Channel1_IRQHandler();
void DMA1_Channel2_IRQHandler();
void DMA1_Channel3_IRQHandler();
void DMA1_Channel4_IRQHandler();
void DMA1_Channel5_IRQHandler();
void DMA1_Channel6_IRQHandler();
void DMA1_Channel7_IRQHandler();
void ADC1_IRQHandler();

void EXTI9_5_IRQHandler();
void TIM1_BRK_TIM15_IRQHandler();
void TIM1_UP_TIM16_IRQHandler();
void TIM1_TRG_COM_TIM17_IRQHandler();
void TIM1_CC_IRQHandler();
void TIM2_IRQHandler();
void TIM3_IRQHandler();
void TIM4_IRQHandler();
void I2C1_EV_IRQHandler();
void I2C1_ER_IRQHandler();
void I2C2_EV_IRQHandler();
void I2C2_ER_IRQHandler();
void SPI1_IRQHandler();
void SPI2_IRQHandler();
void USART1_IRQHandler();
void USART2_IRQHandler();
void USART3_IRQHandler();
void EXTI15_10_IRQHandler();
void RTC_Alarm_IRQHandler();
void CEC_IRQHandler();

void TIM6_DAC_IRQHandler();
void TIM7_IRQHandler();

#endif // ARMXX_ISR_H
