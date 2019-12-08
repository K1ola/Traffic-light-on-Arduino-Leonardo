#include "U8glib.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define countof(a) ( sizeof(a)/sizeof(a[0]) ) 

U8GLIB_NHD_C12864 u8g(13, 11, 10, 9, 8);	// SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9, RST = 8

const int pinJoystick = 0;
const int pinBacklight = 7;
const int pinBuzzer = 3;

uint32_t sysTime = 0;

enum Joystick
{
    joyNone = 0, joyLeft, joyRight, joyUp, joyDown, joyEnter
};    

enum Section
{
    red = 0, yellow = 1, green = 2, left = 3, right = 4
};

struct TrafficLightState
{       
    bool section[5];
    uint8_t count;
};

struct ProgrammElem
{
    int currTime;
    TrafficLightState major, minor;
};

const ProgrammElem programm1[] = 
{
    {90, {{0,0,1,0,0}, 4}, {{1,0,0,0,0}, 3}},
    
    {90+30, {{1,0,0,1,0}, 4}, {{1,0,0,0,0}, 3}},
    
    {90+30+30, {{1,0,0,0,0}, 4}, {{0,0,1,0,0}, 3}},
        
    {90+30+30+15, {{1,0,0,0,0}, 4}, {{1,0,0,0,0}, 3}}
};

const ProgrammElem programmDebug[] = 
{
    {20, {{0,0,1,0,0}, 4}, {{1,0,0,0,0}, 3}},
    
    {20+20, {{1,0,0,1,0}, 4}, {{1,0,0,0,0}, 3}},
    
    {20+20+15, {{1,0,0,0,0}, 4}, {{0,0,1,0,0}, 3}},
        
    {20+20+15+10, {{1,0,0,0,0}, 4}, {{1,0,0,0,0}, 3}}
};

const ProgrammElem *currentProgramm = programmDebug;
int currentProgrammSize = countof(programmDebug);

Joystick GetJoystick()
{   
    static Joystick prevState = joyNone;
    Joystick state = joyNone;
    int analogValue = analogRead(pinJoystick);
    if (analogValue < 100)      state = joyRight;
    else if (analogValue < 300) state = joyUp;
    else if (analogValue < 500) state = joyLeft;
    else if (analogValue < 700) state = joyEnter;
    else if (analogValue < 900) state = joyDown;
    if (state == prevState) return joyNone;
    prevState = state;
    return state;
}

void DrawCrosswalk(uint8_t x, uint8_t y, int angle, uint8_t size)
{
    uint8_t step = angle == 0 ? 5 : 4;
    for (uint8_t s = 0; s < size; s += step)
    {
        if (angle == 0) 
            u8g.drawBox(x+s, y, step-2, 10);
        else          
            u8g.drawBox(x, y+s, 12, step-2);
    }
}

void DrawTrafficLight(uint8_t x, uint8_t y, int angle, TrafficLightState *state)
{
    const uint8_t radius = 3;
    uint8_t sx[5], sy[5];
    switch (angle)
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
    for (uint8_t i = 0; i < state->count; i++)
    {
        if (state->section[i])
            //lights are on
            u8g.drawDisc(sx[i], sy[i], radius);
        else
            //lights are off
            u8g.drawCircle(sx[i], sy[i], radius);
    }      
}

void GetCurrentState(TrafficLightState *major, TrafficLightState *minor) 
{
    int maxTime = currentProgramm[currentProgrammSize-1].currTime;
    int localSysTime = sysTime % maxTime + 1;
    int i;
    for (i=0; i<currentProgrammSize; i++)
    {
        if (localSysTime<=currentProgramm[i].currTime) 
        {
            *major = currentProgramm[i].major;
            *minor = currentProgramm[i].minor;
            break;
        }
    }
    int reverseCounter = currentProgramm[i].currTime - localSysTime;

    bool isGreen = major->section[green] || minor->section[green];
    if (reverseCounter < 2 && isGreen) 
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
    else if (reverseCounter <= (isGreen? 7 : 5))
    {
        if (reverseCounter % 2)
        {
            for (int j=green; j<=right; j++)
            {
                major->section[j] = false;
                minor->section[j] = false;                
            }
        } 
    }

    char timeStr[6];
    sprintf(timeStr, "%03d", reverseCounter+1);
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

    //draw road lines
    u8g.drawLine(lx, 0, lx, uy);
    u8g.drawLine(rx, 0, rx, uy);
    u8g.drawLine(lx, dy, lx, 63);
    u8g.drawLine(rx, dy, rx, 63);
    u8g.drawLine(0, uy, lx, uy);
    u8g.drawLine(0, dy, lx, dy);
    u8g.drawLine(rx, uy, 127, uy);
    u8g.drawLine(rx, dy, 127, dy);

    DrawCrosswalk(lx+4, uy-14, 0, majorRoadWidth-8);
    DrawCrosswalk(lx+4, dy+4,  0, majorRoadWidth-8);
    DrawCrosswalk(lx-16, uy+3, 90, minorRoadWidth-6);
    DrawCrosswalk(rx+4, uy+3, 90, minorRoadWidth-6);

    TrafficLightState major, minor;
    GetCurrentState(&major, &minor);
    
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

void setup(void) 
{
    pinMode(pinBacklight, OUTPUT);
    digitalWrite(pinBacklight, HIGH);
    pinMode(pinBuzzer, OUTPUT);
    u8g.setRot180();
    u8g.setColorIndex(1);
    u8g.setContrast(0);
    u8g.setFont(u8g_font_6x12);
    u8g.setFontRefHeightText();
    u8g.setFontPosTop();

    // every sec interrupt
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    OCR1A = 15624;
    TCCR1B |= (1 << WGM12); 
    TCCR1B |= (1 << CS10);
    TCCR1B |= (1 << CS12);
    TIMSK1 |= (1 << OCIE1A);
    interrupts();
}

ISR(TIMER1_COMPA_vect)
{
    sysTime++;
    RedrawScreen();
}

void loop(void) 
{
    bool backlight = true;
    while (true)
    {
        if (GetJoystick() == joyEnter)
        { 
            backlight = !backlight; 
            digitalWrite(pinBacklight, backlight);
        }
        //RedrawScreen();
    }

  
  
}
