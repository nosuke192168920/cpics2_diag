#ifndef _LCD_BUTTON_H_
#define _LCD_BUTTON_H_

// https://arduinoinfo.mywikis.net/wiki/LCD-Pushbuttons

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

#ifdef __cplusplus
extern "C" {
#endif
    
extern int lcd_key;
extern int adc_key_in;
    
int read_LCD_buttons();

#ifdef __cplusplus
}
#endif


#endif // _LCD_BUTTON_H_
