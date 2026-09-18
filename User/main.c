#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Encoder.h"
#include "Key.h"
#include "Input.h"
#include "Menu.h"
#include "Serial.h"


int main(void)
{
    Input_Event_t Event;

    OLED_Init();
    Encoder_Init();
    Key_Init();
	Serial_Init();

    Menu_Init();
	

    while (1)
    {
        Event = Input_GetEvent();

        Menu_Process(Event);
    }
}
