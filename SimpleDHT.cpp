/*
The MIT License (MIT)

Copyright (c) 2016 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "SimpleDHT.h"

int SimpleDHT11::ErrSuccess = 0;
int SimpleDHT11::ErrStartLow = 100;
int SimpleDHT11::ErrStartHigh = 101;
int SimpleDHT11::ErrDataLow = 102;
int SimpleDHT11::ErrDataRead = 103;
int SimpleDHT11::ErrDataEOF = 104;
int SimpleDHT11::ErrDataChecksum = 105;

int SimpleDHT11::confirm(int pin, int us, byte level) {
    // wait one more count to ensure.
    int cnt = us / 10 + 1;

    bool ok = false;
    for (int i = 0; i < cnt; i++) {
        if (digitalRead(pin) != level) {
            ok = true;
            break;
        }
        delayMicroseconds(10);
    }

    if (!ok) {
        return -1;
    }
    return ErrSuccess;
}

byte SimpleDHT11::bits2byte(byte data[8]) {
    byte v = 0;
    for (int i = 0; i < 8; i++) {
        v += data[i] << (7 - i);
    }
    return v;
}

int SimpleDHT11::sample(int pin, byte data[40]) {
    // empty output data.
    memset(data, 0, 40);

    // notify DHT11 to start: 
    //    1. PULL LOW 20ms.
    //    2. PULL HIGH 20-40us.
    //    3. SET TO INPUT.
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(20);
    digitalWrite(pin, HIGH);
    pinMode(pin, INPUT);
    delayMicroseconds(30);

    // DHT11 starting:
    //    1. PULL LOW 80us
    //    2. PULL HIGH 80us
    if (confirm(pin, 80, LOW)) {
        return ErrStartLow;
    }
    if (confirm(pin, 80, HIGH)) {
        return ErrStartHigh;
    }

    // DHT11 data transmite:
    //    1. 1bit start, PULL LOW 50us
    //    2. PULL HIGH 26-28us, bit(0)
    //    3. PULL HIGH 70us, bit(1)
    for (int j = 0; j < 40; j++) {
        if (confirm(pin, 50, LOW)) {
            return ErrDataLow;
        }

        // read a bit, should never call method,
        // for the method call use more than 20us,
        // so it maybe failed to detect the bit0.
        bool ok = false;
        int tick = 0;
        for (int i = 0; i < 8; i++, tick++) {
            if (digitalRead(pin) != HIGH) {
                ok = true;
                break;
            }
            delayMicroseconds(10);
        }
        if (!ok) {
            return ErrDataRead;
        }
        data[j] = (tick > 3? 1:0);
    }

    // DHT11 EOF:
    //    1. PULL LOW 50us.
    if (confirm(pin, 50, LOW)) {
        return ErrDataEOF;
    }

    return ErrSuccess;
}

int SimpleDHT11::parse(byte data[40], byte* ptemperature, byte* phumidity) {
    byte humidity = bits2byte(data);
    byte humidity2 = bits2byte(data + 8);
    byte temperature = bits2byte(data + 16);
    byte temperature2 = bits2byte(data + 24);
    byte check = bits2byte(data + 32);
    byte expect = humidity + humidity2 + temperature + temperature2;
    if (check != expect) {
        return ErrDataChecksum;
    }
    *ptemperature = temperature;
    *phumidity = humidity;
    return ErrSuccess;
}

int SimpleDHT11::read(int pin, byte* ptemperature, byte* phumidity, byte pdata[40]) {
    int ret = ErrSuccess;

    byte data[40] = {0};
    if ((ret = sample(pin, data)) != ErrSuccess) {
        return ret;
    }

    byte temperature = 0;
    byte humidity = 0;
    if ((ret = parse(data, &temperature, &humidity)) != ErrSuccess) {
        return ret;
    }

    if (pdata) {
        memcpy(pdata, data, 40);
    }
    if (ptemperature) {
        *ptemperature = temperature;
    }
    if (phumidity) {
        *phumidity = humidity;
    }

    return ret;
}
