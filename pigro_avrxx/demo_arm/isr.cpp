
#include "isr.h"

#define DEFAULT_ISR __attribute__ ((alias("Default_Handler")))

void Default_Handler()
{
    InfiniteLoop();
}

void NMI_Handler() DEFAULT_ISR;

void HardFault_Handler() DEFAULT_ISR;
void MemManage_Handler() DEFAULT_ISR;
void BusFault_Handler() DEFAULT_ISR;
void UsageFault_Handler() DEFAULT_ISR;

void SVC_Handler() DEFAULT_ISR;
void DebugMon_Handler() DEFAULT_ISR;

void PendSV_Handler() DEFAULT_ISR;
// void SysTick_Handler() DEFAULT_ISR;
void WWDG_IRQHandler() DEFAULT_ISR;
void PVD_IRQHandler() DEFAULT_ISR;
void TAMPER_IRQHandler() DEFAULT_ISR;
void RTC_IRQHandler() DEFAULT_ISR;
void FLASH_IRQHandler() DEFAULT_ISR;
void RCC_IRQHandler() DEFAULT_ISR;
void EXTI0_IRQHandler() DEFAULT_ISR;
void EXTI1_IRQHandler() DEFAULT_ISR;
void EXTI2_IRQHandler() DEFAULT_ISR;
void EXTI3_IRQHandler() DEFAULT_ISR;
void EXTI4_IRQHandler() DEFAULT_ISR;


void DMA1_Channel1_IRQHandler() DEFAULT_ISR;
void DMA1_Channel2_IRQHandler() DEFAULT_ISR;
void DMA1_Channel3_IRQHandler() DEFAULT_ISR;
void DMA1_Channel4_IRQHandler() DEFAULT_ISR;
void DMA1_Channel5_IRQHandler() DEFAULT_ISR;
void DMA1_Channel6_IRQHandler() DEFAULT_ISR;
void DMA1_Channel7_IRQHandler() DEFAULT_ISR;
void ADC1_IRQHandler() DEFAULT_ISR;

void EXTI9_5_IRQHandler() DEFAULT_ISR;
void TIM1_BRK_TIM15_IRQHandler() DEFAULT_ISR;
void TIM1_UP_TIM16_IRQHandler() DEFAULT_ISR;
void TIM1_TRG_COM_TIM17_IRQHandler() DEFAULT_ISR;
void TIM1_CC_IRQHandler() DEFAULT_ISR;
void TIM2_IRQHandler() DEFAULT_ISR;
void TIM3_IRQHandler() DEFAULT_ISR;
void TIM4_IRQHandler() DEFAULT_ISR;
void I2C1_EV_IRQHandler() DEFAULT_ISR;
void I2C1_ER_IRQHandler() DEFAULT_ISR;
void I2C2_EV_IRQHandler() DEFAULT_ISR;
void I2C2_ER_IRQHandler() DEFAULT_ISR;
void SPI1_IRQHandler() DEFAULT_ISR;
void SPI2_IRQHandler() DEFAULT_ISR;
//void USART1_IRQHandler() DEFAULT_ISR;
void USART2_IRQHandler() DEFAULT_ISR;
void USART3_IRQHandler() DEFAULT_ISR;
void EXTI15_10_IRQHandler() DEFAULT_ISR;
void RTC_Alarm_IRQHandler() DEFAULT_ISR;
void CEC_IRQHandler() DEFAULT_ISR;

void TIM6_DAC_IRQHandler() DEFAULT_ISR;
void TIM7_IRQHandler() DEFAULT_ISR;
