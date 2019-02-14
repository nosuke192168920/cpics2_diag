/* -*- Mode:C -*- */
/* 
 * CPicS2 diag for Arduino Uno
 * Copyright (c) 2019 sasugaanija@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 */

#define VERSION "1.0"

#include "sha1.h"
#include "cps2_key_sha1.h"

#define NAMELEN 16

#define __M(TITLE) \
    const char name_##TITLE[NAMELEN] PROGMEM = #TITLE;
#include "cps2_list.h"
#undef __M

const char *const name_list[] PROGMEM = {
#define __M(TITLE) \
    name_##TITLE, 
#include "cps2_list.h"
#undef __M    
};


#define __M(TITLE) \
    const uint32_t key_sha1_##TITLE[SHA1HashSize / sizeof(uint32_t)] PROGMEM = SHA1_##TITLE;
#include "cps2_list.h"
#undef __M

const uint32_t *key_sha1_list[] = {
#define __M(TITLE) \
    key_sha1_##TITLE, 
#include "cps2_list.h"
#undef __M    
};


#include <LiquidCrystal.h>
#include <stdio.h>

#define KEYLEN 20

#define DATA        2
#define WP_         3
#define CLOCK       11
#define RESET_      12

// use reference code at https://arduinoinfo.mywikis.net/wiki/LCD-Pushbuttons

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int lcd_key       = btnNONE;
int adc_key_in    = 0;
int adc_key_prev  = 0;

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




#define LCDW 16
#define LCDH 2

void linePrint(uint32_t line, char *str)
{
    char buffer[LCDW + 1];
    int len;

    if (line >= LCDH)
        line = 1;

    lcd.setCursor(0, line);

    len = strlen(str);
    for (int i = 0; i < LCDW; i++) {
        if (i < len)
            buffer[i] = str[i];
        else
            buffer[i] = ' ';
    }
    buffer[LCDW] = '\0';
    lcd.print(buffer);
}


void linePrintNum(uint32_t line, int num)
{
    if (line >= LCDH)
        line = 1;

    lcd.setCursor(0, line);
    lcd.print(num);
}


void updatePortStatus()
{
    int val;
    char buf[16];
    buf[0] = 'D';
    buf[1] = digitalRead(DATA) == 0 ? '0' : '1';
    buf[2] = ' ';
    buf[3] = 'W';
    buf[4] = digitalRead(WP_) == 0 ? '0' : '1';
    buf[5] = ' ';
    buf[6] = 'C';
    buf[7] = digitalRead(CLOCK) == 0 ? '0' : '1';
    buf[8] = ' ';
    buf[9] = 'R';
    buf[10] = digitalRead(RESET_) == 0 ? '0' : '1';
    buf[11] = '\0';
    linePrint(1, buf);
}


void waitPort(int d, int port, int num)
{
    int val;
    do {
        if (d > 0)
            delay(d);
        val = digitalRead(port);
    } while (val != num);
}

void waitPort2(int d, int port1, int num1, int port2, int num2)
{
    int val1, val2;
    do {
        if (d > 0)
            delay(d);
        updatePortStatus();
        val1 = digitalRead(port1);
        val2 = digitalRead(port2);
    } while (val1 != num1 || val2 != num2);
}


#define DIGEST32LEN ((SHA1HashSize + sizeof(uint32_t) - 1) / sizeof(uint32_t))
#define RAWKEY32LEN ((KEYLEN + sizeof(uint32_t) - 1) / sizeof(uint32_t))
uint32_t digest32[DIGEST32LEN];
uint32_t rawkey32[RAWKEY32LEN];

enum displayMode {
    MODE_SHA1 = 0,
    MODE_RAW
};

int displayMode = 0; // 0: SHA1 digest, 1: RAW
int displayIndexSha1 = 0;
int displayIndexRaw = 0;

void verifyMode()
{
    SHA1Context sha;
    uint8_t digest[SHA1HashSize];
    uint8_t rawkey[KEYLEN];  
    int keynum =  sizeof(key_sha1_list) / sizeof(key_sha1_list[0]);
    int keyIdx = -1;
    int step;

    // reset SHA1 context
    SHA1Reset(&sha);
    
    // wait RESET_ enable and WP_ disable
    linePrint(0, "Wait Unlock");
    waitPort2(10, RESET_, 0, WP_, 1);

    linePrint(0, "Read Data...");
    linePrint(1, "");

    step = 0;
    for (int i = 0; i < KEYLEN; i++) {
        rawkey[i] = 0;
        for (int j = 7; j >= 0; j--) {
            int d;
            linePrintNum(1, step++);
            waitPort(0, CLOCK, 1);
            d = digitalRead(DATA);
            rawkey[i] |= ((d << j) & 0xFF);
            linePrintNum(1, step++);
            waitPort(0, CLOCK, 0);
        }
    }

    for (int i = 0, j = 0; i < KEYLEN; i += 4, j++) {
        rawkey32[j] = ((i < KEYLEN ? (uint32_t)rawkey[i] : 0) << 24)
            | ((i + 1 < KEYLEN ? (uint32_t)rawkey[i + 1] : 0) << 16)
            | ((i + 2 < KEYLEN ? (uint32_t)rawkey[i + 2] : 0) << 8)
            | (i + 3 < KEYLEN ? (uint32_t)rawkey[i + 3] : 0);
    }
    
    linePrint(0, "Wait Lock");
    waitPort2(10, RESET_, 1, WP_, 0);

  
    SHA1Input(&sha, (const uint8_t *)rawkey, KEYLEN);
    SHA1Result(&sha, digest);

    for (int i = 0, j = 0; i < SHA1HashSize; i += 4, j++) {
        digest32[j] = ((i < KEYLEN ? (uint32_t)digest[i] : 0) << 24)
            | ((i + 1 < KEYLEN ? (uint32_t)digest[i + 1] : 0) << 16)
            | ((i + 2 < KEYLEN ? (uint32_t)digest[i + 2] : 0) << 8)
            | (i + 3 < KEYLEN ? (uint32_t)digest[i + 3] : 0);
    }

 
    // serach matched key
    linePrint(0, "Search DB...");
    step = 0;
    
    for (int i = 0; i < keynum; i++) {
        keyIdx = i;
        linePrintNum(1, keyIdx);
        for (int j = 0; j < SHA1HashSize / sizeof(uint32_t); j++) {
            uint32_t sha1_part = pgm_read_dword(key_sha1_list[i] + j);
            if (digest32[j] != sha1_part) {
                keyIdx = -1;
                break;
            }
        }
        if (keyIdx >= 0) {
            break;
        }
    }

    char buffer[20];
    if (keyIdx >= 0) {
        // valid key
        char title[NAMELEN];
        strcpy_P(title, (char *)pgm_read_word(&(name_list[keyIdx])));
        sprintf(buffer, "OK! %s", title);
        linePrint(0, buffer);
    } else {
        linePrint(0, "Unknown key...");
    }
}


void showInitialScreen()
{
    char buffer[20];
    sprintf(buffer, "CPicS2 diag V%s", VERSION);
    linePrint(0, buffer);
    delay(3000);
}


////////////////////////////////////////////////////////////////

void setup()
{
    pinMode(WP_, INPUT);
    pinMode(RESET_, INPUT);
    pinMode(CLOCK, INPUT);
    pinMode(DATA, INPUT);

    lcd.begin(LCDW, LCDH);
    showInitialScreen();
    verifyMode();
}



void loop()
{
    char buffer[20];
    
    adc_key_prev = lcd_key ;       // Looking for changes
    lcd_key = read_LCD_buttons();  // read the buttons

    if (adc_key_prev != lcd_key) {
        switch (lcd_key) {
        case btnRIGHT:
            if (displayMode == MODE_SHA1 && displayIndexSha1 < DIGEST32LEN - 1) {
                displayIndexSha1++;
            } else if (displayMode == MODE_RAW && displayIndexRaw < RAWKEY32LEN - 1) {
                displayIndexRaw++;
            }
            break;
        case btnLEFT:
            if (displayMode == MODE_SHA1 && displayIndexSha1 > 0) {
                displayIndexSha1--;
            } else if (displayMode == MODE_RAW && displayIndexRaw > 0) {
                displayIndexRaw--;
            }
            break;
        case btnDOWN:
            if (displayMode == MODE_SHA1)
                displayMode = MODE_RAW;
            break;
        case btnUP:
            if (displayMode == MODE_RAW)
                displayMode = MODE_SHA1;
            break;
        case btnSELECT:
            break;
        }
    }

    if (displayMode == MODE_SHA1) {
        sprintf(buffer, "SHA1[%d] %08lX", displayIndexSha1, digest32[displayIndexSha1]);
    } else {
        sprintf(buffer, "RAW [%d] %08lX", displayIndexRaw, rawkey32[displayIndexRaw]);
    }

    linePrint(1, buffer);

    delay(10);
}
