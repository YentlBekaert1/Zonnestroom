# Zonnestroom

Dit project maakt gebruik van het nucleo development board STM32F091RCT6 meer informatie hier: https://www.st.com/en/microcontrollers-microprocessors/stm32f091rc.html#overview

![GitHub Logo](/images/schema.png)

### Lijst met gerbuikte pinnen op STM:
* DAC => PA4
* I2C => PB8 en PB9
* Interrupts => PC8, PC10, PC11, PC12
* UART => PA2, PA3
* CLOCK => PF0-OSC_IN, PF1-OSC_OUT

### Formule KWh berkenen aan de hand van pulsen:
Gemeten KWh = (3600000/Pulsen_Per_KWH)/TijdTussenPulsen;



