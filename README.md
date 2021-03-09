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
* DAC => PA4 (DAC om xtr115 aan te sturen)
* I2C => PB8 en PB9 (LCD)
* Interrupts => PC6, PC8, PC11, PC12 (PC12 = drukknop rotary encoder, PC6 = OUTA rotrary encoder, PC8 = OUTA rotrary encoder,  PC11 = interrupt om pulsen kWh te meten)
* UART => PA2, PA3 (Debugging)
* CLOCK => PF0-OSC_IN, PF1-OSC_OUT

### Formule KWh berkenen aan de hand van pulsen:
Gemeten KWh = (3600000/Pulsen_Per_KWH)/TijdTussenPulsen;

### Formule DAC aansturen in %:
 percent = (gemeten KWh*1000)/waarde_apparaat;
 DAC output = DAC Max Output * percent;

