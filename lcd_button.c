#include "lcd_button.h"

// https://arduinoinfo.mywikis.net/wiki/LCD-Pushbuttons

int lcd_key      = btnNONE;
int adc_key_in   = 0;

int read_LCD_buttons()
{
    adc_key_in = analogRead(0);

    // uncomment here to check button value
    //linePrint(0, "");
    //linePrintNum(0, adc_key_in);
    delay(5);
    int k = (analogRead(0) - adc_key_in);
    if (abs(k) > 50) return lcd_key;

    if (adc_key_in > 1000) return btnNONE;
    if (adc_key_in < 100)  return btnRIGHT;
    if (adc_key_in < 195)  return btnUP;
    if (adc_key_in < 380)  return btnDOWN;
    if (adc_key_in < 555)  return btnLEFT;
    if (adc_key_in < 790)  return btnSELECT;

    return btnNONE;
}
