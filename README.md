# Zonnestroom
### Beschrijving

  Het doel van dit project is om ervoor te zorgen dat de overtollige energie die de zonnepanelen produceren niet terug op het net gestuurd word maar verbruikt word. We gebruiken     een STM32F091 om een KWh meter uit te lezen en via solid state relais een verbruiker te regelen.

Dit project maakt gebruik van het nucleo development board STM32F091RCT6 meer informatie hier: https://www.st.com/en/microcontrollers-microprocessors/stm32f091rc.html#overview

### Electronische opstelling: 
![GitHub Logo](/images/schema.png)

### Componenten:
* STM32F091RCT6
* XTR-115UA
* Crydom PMP2450W 

### Lijst met gerbuikte pinnen op STM:
* DAC => PA4
* I2C => PB8 en PB9
* Interrupts => PC8, PC10, PC11, PC12
* UART => PA2, PA3
* CLOCK => PF0-OSC_IN, PF1-OSC_OUT

### Formule KWh berkenen aan de hand van pulsen:
Gemeten KWh = (3600000/Pulsen_Per_KWH)/TijdTussenPulsen;

### Formule DAC aansturen in %:
 percent = (gemeten KWh*1000)/waarde_apparaat;
 DAC output = DAC Max Output * percent;

