#include "U8glib.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Adafruit_NeoPixel.h>

#define PIN12            12
#define PIN6            6
#define PIN5            5
#define PIN4            4
#define PIN2            2
// How many NeoPixels are attached to the Arduino
#define NUMPIXELS      4

//#define DEBUG
#define countof(a) ( sizeof(a)/sizeof(a[0]) ) 

Adafruit_NeoPixel pixelsMajor12 = Adafruit_NeoPixel(NUMPIXELS, PIN12, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel pixelsMinor6 = Adafruit_NeoPixel(NUMPIXELS, PIN6, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel pixelsMajor5 = Adafruit_NeoPixel(NUMPIXELS, PIN5, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel pixelsMinor4 = Adafruit_NeoPixel(NUMPIXELS, PIN4, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel pixelsPedestrian2 = Adafruit_NeoPixel(NUMPIXELS, PIN2, NEO_RGB + NEO_KHZ800);
U8GLIB_NHD_C12864 u8g(13, 11, 10, 9, 8);	// SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9, RST = 8

const int pinJoystick = 0;
const int pinBacklight = 7;
const int pinBuzzer = 3;
const int pinUART = 4;

uint32_t sysTime = 0;

enum Joystick   //структура для определения текущей позиции джойстика 
{
    joyNone = 0, joyLeft, joyRight, joyUp, joyDown, joyEnter
};    

enum Section   //структура для определения сигналов светофора 
{
    red = 0, yellow = 1, green = 2, left = 3, right = 4
};

struct TrafficLightState   //структура для определения одного светофора
{       
    bool section[5];
    uint8_t count;
};

struct ProgrammElem   //структура для определения конфигурации светофоров
{
    int currTime;
    TrafficLightState major, minor;
};

const ProgrammElem programmDefault[] = 
{
    {90, {{0,0,1,0,0}, 4}, {{1,0,0,0,0}, 3}},
    
    {90+30, {{1,0,0,1,0}, 4}, {{1,0,0,0,0}, 3}},
    
    {90+30+30, {{1,0,0,0,0}, 4}, {{0,0,1,0,0}, 3}},
        
    {90+30+30+15, {{1,0,0,0,0}, 4}, {{1,0,0,0,0}, 3}}
};

ProgrammElem programmExternal[4] = {0};

const ProgrammElem programmDebug[] = 
{
    {20, {{0,0,1,0,0}, 4}, {{1,0,0,0,0}, 3}},
    
    {20+20, {{1,0,0,1,0}, 4}, {{1,0,0,0,0}, 3}},
    
    {20+20+15, {{1,0,0,0,0}, 4}, {{0,0,1,0,0}, 3}},
        
    {20+20+15+10, {{1,0,0,0,0}, 4}, {{1,0,0,0,0}, 3}}
};

#ifdef DEBUG
const ProgrammElem *currentProgramm = programmDebug;
#else
const ProgrammElem *currentProgramm = programmDefault;
#endif
int currentProgrammSize = countof(programmDebug);

Joystick GetJoystick()      //функция для определения позиции джойстика 
{   
    static Joystick prevState = joyNone;    //инициализация предыдущего состояния джойстика 
    Joystick state = joyNone;   //начальная инициализация джойстика
    int analogValue = analogRead(pinJoystick);  //считывание аналогового значения 
    if (analogValue < 100)      state = joyRight;
    else if (analogValue < 300) state = joyUp;
    else if (analogValue < 500) state = joyLeft;
    else if (analogValue < 700) state = joyEnter;
    else if (analogValue < 900) state = joyDown;
    if (state == prevState) return joyNone; //установка предыдущего состояния 
    prevState = state; //установка текущего состояния
    return state;
}

void TurnOnLEDs(int angle, int light_item)
{
     if (angle == 90 || angle == 270)   //светофоры второстепенной дороги
            switch (light_item) //какой сигнал светофора нужно зажечь 
            {
               case 0:
                   pixelsMinor6.setPixelColor(2, pixelsMinor6.Color(0,150,0)); // Назначаем цвет "Красный" 
                   pixelsMinor4.setPixelColor(2, pixelsMinor4.Color(0,150,0)); // Назначаем цвет "Красный"
                   break;
               case 1:
                   pixelsMinor6.setPixelColor(1, pixelsMinor6.Color(255, 255, 0)); // Назначаем цвет "Желтый"
                   pixelsMinor4.setPixelColor(1, pixelsMinor4.Color(255, 255, 0)); // Назначаем цвет "Желтый"
                   break;
               case 2:
                   pixelsMinor6.setPixelColor(0, pixelsMinor6.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   pixelsMinor4.setPixelColor(0, pixelsMinor4.Color(255,0,0)); // Назначаем цвет "Желтый"
                   break;
               case 3:
                   pixelsMinor6.setPixelColor(0, pixelsMinor6.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   pixelsMinor4.setPixelColor(0, pixelsMinor4.Color(255,0,0)); // Назначаем цвет "Желтый"
                   break;
               case 4: 
                   pixelsMinor6.setPixelColor(0, pixelsMinor6.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   pixelsMinor4.setPixelColor(0, pixelsMinor4.Color(255,0,0)); // Назначаем цвет "Желтый"
                   break;
            }   
    if (angle == 0 || angle == 180)     //светофоры главной дороги
            switch (light_item) //какой сигнал светофора нужно зажечь
            {
               case 0:
                   pixelsMajor12.setPixelColor(0, pixelsMajor12.Color(0,150,0)); // Назначаем цвет "Красный" 
                   pixelsMajor5.setPixelColor(0, pixelsMajor5.Color(0,150,0)); // Назначаем цвет "Красный"
                   pixelsMajor12.show();
                   pixelsMajor5.show(); 
//                   *majorRed = true;
                   break;
               case 1:
                   pixelsMajor12.setPixelColor(1, pixelsMajor12.Color(255, 255, 0)); // Назначаем цвет "Желтый"
                   pixelsMajor5.setPixelColor(1, pixelsMajor5.Color(255, 255, 0)); // Назначаем цвет "Желтый"
                   break;
               case 2:
                   pixelsMajor12.setPixelColor(2, pixelsMajor12.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   pixelsMajor5.setPixelColor(2, pixelsMajor5.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   break;
               case 3:
                   pixelsMajor12.setPixelColor(3, pixelsMajor12.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   pixelsMajor5.setPixelColor(3, pixelsMajor5.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   break;
               case 4: 
                   pixelsMajor12.setPixelColor(4, pixelsMajor12.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   pixelsMajor5.setPixelColor(4, pixelsMajor5.Color(255,0,0)); // Назначаем цвет "Зеленый"
                   break;
            } 
}

void TurnOffLEDs(int angle, int light_item) 
{
    if (angle == 90 || angle == 270)     //светофоры второстепенной дороги
            switch (light_item) //какой сигнал светофора нужно зажечь
            {
               case 0:
                   pixelsMinor6.setPixelColor(2, pixelsMinor6.Color(0,0,0)); // Назначаем цвет "Черный" 
                   pixelsMinor4.setPixelColor(2, pixelsMinor4.Color(0,0,0));
                   break;
               case 1:
                   pixelsMinor6.setPixelColor(1, pixelsMinor6.Color(0,0,0));
                   pixelsMinor4.setPixelColor(1, pixelsMinor4.Color(0,0,0));
                   break;
               case 2:
                   pixelsMinor6.setPixelColor(0, pixelsMinor6.Color(0,0,0));
                   pixelsMinor4.setPixelColor(0, pixelsMinor4.Color(0,0,0));
                   break;
               case 3:
                   pixelsMinor6.setPixelColor(0, pixelsMinor6.Color(0,0,0));
                   pixelsMinor4.setPixelColor(0, pixelsMinor4.Color(0,0,0));
                   break;
               case 4: 
                   pixelsMinor6.setPixelColor(0, pixelsMinor6.Color(0,0,0));
                   pixelsMinor4.setPixelColor(0, pixelsMinor4.Color(0,0,0));
                   break;
            }   
    if (angle == 0 || angle == 180)    //светофоры главной дороги
            switch (light_item)     //какой сигнал светофора нужно зажечь
            {
               case 0:
                   pixelsMajor12.setPixelColor(0, pixelsMajor12.Color(0,0,0)); // Назначаем цвет "Черный" 
                   pixelsMajor5.setPixelColor(0, pixelsMajor5.Color(0,0,0));
                   break;
               case 1:
                   pixelsMajor12.setPixelColor(1, pixelsMajor12.Color(0,0,0));
                   pixelsMajor5.setPixelColor(1, pixelsMajor5.Color(0,0,0));
                   break;
               case 2:
                   pixelsMajor12.setPixelColor(2, pixelsMajor12.Color(0,0,0));
                   pixelsMajor5.setPixelColor(2, pixelsMajor5.Color(0,0,0));
                   break;
               case 3:
                   pixelsMajor12.setPixelColor(3, pixelsMajor12.Color(0,0,0));
                   pixelsMajor5.setPixelColor(3, pixelsMajor5.Color(0,0,0)); 
                   break;
               case 4: 
                   pixelsMajor12.setPixelColor(4, pixelsMajor12.Color(0,0,0));
                   pixelsMajor5.setPixelColor(4, pixelsMajor5.Color(0,0,0));
                   break;
            }     
}

void TurnOnPedestrian(bool flag)    //функция для установки нужного сигнала для пешеходных светофоров
{
    if (flag)
    {
       pixelsPedestrian2.setPixelColor(0, pixelsPedestrian2.Color(255,0,0)); // Назначаем цвет "Зеленый"
       pixelsPedestrian2.setPixelColor(1, pixelsPedestrian2.Color(0,0,0)); // Назначаем цвет "Черный"       
    }
    else
    { 
       pixelsPedestrian2.setPixelColor(0, pixelsPedestrian2.Color(0,0,0)); // Назначаем цвет "Черный" 
       pixelsPedestrian2.setPixelColor(1, pixelsPedestrian2.Color(0,150,0)); // Назначаем цвет "Красный"      
    }
}

void ShowLEDs()     //функция для включения светодиодных лент в соответствии с заданными значениями
{
    pixelsMajor12.show();
    pixelsMajor5.show(); 
    pixelsMinor6.show(); 
    pixelsMinor4.show(); 
    pixelsPedestrian2.show(); 
}

void DrawCrosswalk(uint8_t x, uint8_t y, int angle, uint8_t size)   //функция для отрисовки пешеходных дорог
{
    uint8_t step = angle == 0 ? 5 : 4;  //в зависимости от значения угла будет отриосван пешеходный переход для главной или второстепенной дорог
    for (uint8_t s = 0; s < size; s += step)
    {
        if (angle == 0) 
            u8g.drawBox(x+s, y, step-2, 10);
        else          
            u8g.drawBox(x, y+s, 12, step-2);
    }
}

void DrawTrafficLight(uint8_t x, uint8_t y, int angle, TrafficLightState *state)    //функция для отрисовки светофоров
{
    const uint8_t radius = 3;
    uint8_t sx[5], sy[5];
    switch (angle)  //в зависимоти от угла задаются координаты светофоров главной и второстепенной дорог
    {
        case 0:
            sx[0] = sx[1] = sx[2] = x + 3*radius;
            sx[3] = x + radius;
            sx[4] = x + 5*radius;
            sy[0] = y + radius;
            sy[1] = y + 3*radius;
            sy[2] = sy[3] = sy[4] = y + 5*radius;
            break;
        case 90:
            sx[0] = x - radius;
            sx[1] = x - 3*radius;
            sx[2] = sx[3] = sx[4] = x - 5*radius;
            sy[0] = sy[1] = sy[2] = y + 3*radius;
            sy[3] = y + radius;
            sy[4] = y + 5*radius;
            break;
        case 180:
            sx[0] = sx[1] = sx[2] = x - 3*radius;
            sx[3] = x - radius;
            sx[4] = x - 5*radius;
            sy[0] = y - radius;
            sy[1] = y - 3*radius;
            sy[2] = sy[3] = sy[4] = y - 5*radius;
            break;
        case 270:
            sx[0] = x + radius;
            sx[1] = x + 3*radius;
            sx[2] = sx[3] = sx[4] = x + 5*radius;
            sy[0] = sy[1] = sy[2] = y - 3*radius;
            sy[3] = y - radius;
            sy[4] = y - 5*radius;
            break;
    }
    for (uint8_t i = 0; i < state->count; i++)  //цикл по всем состояниям заданной конфигурации светофоров
    {
        if (state->section[i])  //если текущая секция текущего светофора включена 
        {
            u8g.drawDisc(sx[i], sy[i], radius); //отрисовать закрашенную секцию светофора
            TurnOnLEDs(angle, i);   //включить данную секцию светофора
        }
        else  //если текущая секция текущего светофора не включена 
        {
            u8g.drawCircle(sx[i], sy[i], radius); //отрисовать незакрашенную секцию светофора
            TurnOffLEDs(angle, i);   //выключить данную секцию светофора
        }
    }  
    ShowLEDs();   
}

void GetCurrentState(TrafficLightState *major, TrafficLightState *minor)    //функция определения текущего состояния светофоров 
{
    int maxTime = currentProgramm[currentProgrammSize-1].currTime;  //время последней программы в массиве
    int localSysTime = sysTime % maxTime + 1;   //вычислить из системного общего времени, какая часть прошла для светофоров. П
    int i;
    for (i=0; i<currentProgrammSize; i++)   //обновить состояния светофоров в соответствии с текущей программой
    {
        if (localSysTime<=currentProgramm[i].currTime) 
        {
            *major = currentProgramm[i].major;
            *minor = currentProgramm[i].minor;
            break;
        }
    }
    int reverseCounter = currentProgramm[i].currTime - localSysTime;    //счетчик оставшего для движения времени 

    bool isGreen = major->section[green] || minor->section[green];      //горит ли на главных или второстепенных стефорах "зеленый"?
    bool isRed = major->section[red] && minor->section[red] && !major->section[right] && !major->section[left];      //горит ли на главных или второстепенных стефорах "зеленый"?
    if (reverseCounter < 2 && isGreen)  //зажечь "желтый" при переходес "зеленый" на "красный" на главных или второстепенных стефорах 
    {
        if (major->section[green])
        {
            major->section[green] = false;
            major->section[yellow] = true;
        }
            
        if (minor->section[green])
        {
            minor->section[green] = false;
            minor->section[yellow] = true;
        }
    } 
    else if (reverseCounter <= (isGreen? 7 : 5))   //обеспечить переключение с "зеленого" и стрелки на "красный" на главных или второстепенных стефорах, без зажигания "желтого"
    {
        if (reverseCounter % 2) //обеспечить мигание "зеленый" при перехода с "зеленый" на "красный" на главных или второстепенных стефорах
        {
            for (int j=green; j<=right; j++)
            {
                major->section[j] = false;
                minor->section[j] = false;                
            }
        } 
    }
    // "красный" загружается при установке текущей программы
    TurnOnPedestrian(isRed);
    
    char timeStr[6];
    sprintf(timeStr, "%03d", reverseCounter+1);     //отрисовать оставшееся время
    u8g.drawStr(57, 28, timeStr);
}

void DrawCrossroad()
{
    const uint8_t majorRoadWidth = 45;
    const uint8_t minorRoadWidth = 19;
    uint8_t lx, rx, uy, dy;

    lx = 64 - majorRoadWidth/2;
    rx = lx + majorRoadWidth;
    uy = 32 - minorRoadWidth/2;
    dy = uy + minorRoadWidth;

    //отрисовка очертаний дорог
    u8g.drawLine(lx, 0, lx, uy);
    u8g.drawLine(rx, 0, rx, uy);
    u8g.drawLine(lx, dy, lx, 63);
    u8g.drawLine(rx, dy, rx, 63);
    u8g.drawLine(0, uy, lx, uy);
    u8g.drawLine(0, dy, lx, dy);
    u8g.drawLine(rx, uy, 127, uy);
    u8g.drawLine(rx, dy, 127, dy);

    //отрисовка пешеходных переходов для главной и второстепенной дорог
    DrawCrosswalk(lx+4, uy-14, 0, majorRoadWidth-8);
    DrawCrosswalk(lx+4, dy+4,  0, majorRoadWidth-8);
    DrawCrosswalk(lx-16, uy+3, 90, minorRoadWidth-6);
    DrawCrosswalk(rx+4, uy+3, 90, minorRoadWidth-6);

    TrafficLightState major, minor;
    GetCurrentState(&major, &minor);

    //отрисовка светофров для главной и второстепенной дорог
    DrawTrafficLight(rx+4, dy+3, 0, &major);
    DrawTrafficLight(lx-4, uy-3, 180, &major);
    DrawTrafficLight(lx-4, dy, 90, &minor);
    DrawTrafficLight(rx+4, uy, 270, &minor);
}

void RedrawScreen()
{
    u8g.firstPage();  
    do 
    {
        DrawCrossroad();
    } 
    while (u8g.nextPage());
}

//Программа для демонстрации передачи по USART
//14 00 00 00 01 00 00 04 01 00 00 00 00 03
//28 00 01 00 00 01 00 04 01 00 00 00 00 03
//37 00 01 00 00 00 00 04 00 00 01 00 00 03
//41 00 01 00 00 00 00 04 01 00 00 00 00 03 

bool LoadFromPC()   //функция для приема данных с ПЭВМ
{    
    const uint32_t startTimeout = 1000; //инициализация максимального времени передачи          
    Serial1.begin(9600);    //инициализируем последовательный интерфейс и ожидаем открытия порта
    while (!Serial1);   // ожидаем подключения последовательного порта  
    uint32_t timeout = startTimeout;    //инициализация текущего времени передачи 
    ProgrammElem programmExternalBackup[4]; 
    memcpy(programmExternalBackup, programmExternal, sizeof(programmExternal)); //сохранение прошлой программы на случай ошибки передачи
    //инициализация структур для хранения полученных данных 
    uint8_t* pointerBuffer = (uint8_t*)programmExternal; 
    uint8_t receivedBytes = 0;
    char strTimer[24] = {0};
    char strLastByte[24] = {0};
    char strBytesCount[24] = {0};
    sprintf(strLastByte, "Last received: 0");
    sprintf(strBytesCount, "All received: 0");
    while (receivedBytes < sizeof(programmExternal))    //пока кол-во принятых байт не равно размеру структуры конфигурации 
    {
        if (--timeout == 0) //если время передачи истекло - сообщить об ошибке 
        {
            Serial1.end();  //завершить передачу
            memcpy(programmExternal, programmExternalBackup, sizeof(programmExternal)); //восстановление прошлой конфигурации программы

            //вывод сообщения об ошибке
            u8g.firstPage();  
            do 
            {
                u8g.drawStr(40, 28, "Timeout!");
            } 
            while (u8g.nextPage());

            //подача звукового сигнала 
            #ifndef DEBUG
            tone(pinBuzzer, 1000);
            for (volatile uint32_t t=0; t<200000; t++);    
            noTone(pinBuzzer);
            #endif
            for (volatile uint32_t t=0; t<800000; t++);  
            return false;
        }
        uint8_t lastByte = 0; 
        // чтение посимвольно и сохранение в буфер
        if (Serial1.available() > 0) 
        {
            lastByte = *pointerBuffer++ = Serial1.read();
            receivedBytes++;
            timeout = startTimeout;
            
            sprintf(strLastByte, "Last byte: %d", lastByte);
            sprintf(strBytesCount, "All bytes: %d", receivedBytes);
        }

        // отображение оставшегося для передачи времени и кол-во принятых байт
        sprintf(strTimer, "Timer: %u", timeout);
        u8g.firstPage();  
        do 
        {               
            u8g.drawStr(0, 8, strTimer);
            u8g.drawStr(0, 24, strLastByte);
            u8g.drawStr(0, 40, strBytesCount);
        } 
        while (u8g.nextPage());
    }
    Serial1.end();
    return true;
}

void DrawMenu()
{
    // инициализация вариантов выбора меню
    const char* menuItems[] = 
    {
        "Default programm",
        "External programm",
        "Load from PC",
        "Exit",
    };

    uint8_t selectedItem = 0;   //номер выбранного элемента
    
    while (1) 
    {
        // отрисовка вариантов меню
        u8g.firstPage();  
        do 
        {
            uint8_t y = 0;
            uint8_t i;
            const uint8_t stepY = 16;
            for (i=0; i<4; i++)
            {
                if (i == selectedItem) u8g.drawFrame(0,y,128,stepY);
                u8g.drawStr(4, y+4, menuItems[i]);
                y+=stepY;
            }
        } 
        while (u8g.nextPage());

        switch (GetJoystick()) 
        {
            case joyUp:
                if (selectedItem > 0) selectedItem--;
                break;
            case joyDown:
                if (selectedItem < 3) selectedItem++;
                break;
            case joyEnter:
                switch (selectedItem) 
                {
                    case 0:
                        #ifdef DEBUG
                        currentProgramm = programmDebug;
                        #else
                        currentProgramm = programmDefault;
                        #endif
                        return;
                    case 1:
                        currentProgramm = programmExternal;
                        return;
                    case 2:
                        if (LoadFromPC())
                        {                            
                            currentProgramm = programmExternal;
                            return;
                        }
                        break;
                    case 3:
                        return;
                }
        }
    }    
}

void setup(void) 
{
    pinMode(pinBacklight, OUTPUT);  //инициализая подстветки дисплея
    digitalWrite(pinBacklight, HIGH);   //включение подсветки дисплея
    pinMode(pinBuzzer, OUTPUT); //инициализая зуммера
    pinMode(pinUART,   OUTPUT); //инициализация порта направления передачи данных по USART
    digitalWrite(pinUART, LOW); //указание направление передачи данных по USART 
    Serial.begin(9600); //для отладочного вывода в виртуальный монитор 
    //настройка отображения на дисплее
    u8g.setRot180();
    u8g.setColorIndex(1);
    u8g.setContrast(0);
    u8g.setFont(u8g_font_6x12);
    u8g.setFontRefHeightText();
    u8g.setFontPosTop();

    //инициализация светодиодных лент
    pixelsMajor12.begin();
    pixelsMajor5.begin();    
    pixelsMinor6.begin();
    pixelsMinor4.begin();
    pixelsPedestrian2.begin(); 

    //каждую секунду - прерывание 
    noInterrupts(); //отключить глобавльные прерывания
    TCCR1A = 0; //установить регистры в 0
    TCCR1B = 0;
    OCR1A = 15624; // установка регистра совпадения
    TCCR1B |= (1 << WGM12); // включить CTC режим 
    TCCR1B |= (1 << CS10); // Установить биты на коэффициент деления 1024
    TCCR1B |= (1 << CS12);
    TIMSK1 |= (1 << OCIE1A);  // включить прерывание по совпадению таймера 
    interrupts(); // включить глобальные прерывания
}

ISR(TIMER1_COMPA_vect)
{
    sysTime++;  //увеличить на единицу системный счетчик
    RedrawScreen(); //перерисовать экран дисплея 
}

void loop(void) 
{           
    while (true)
    {
        if (GetJoystick() == joyEnter)  //если был нажат джойстик
        {       
            TIMSK1 &= ~(1 << OCIE1A); // выключить прерывание по совпадению таймера
            DrawMenu(); // отрисовка меню
            sysTime = 0; // сброс системного счетчика            
            TIMSK1 |= (1 << OCIE1A); // включить прерывание по совпадению таймера
        }     
    }
}
