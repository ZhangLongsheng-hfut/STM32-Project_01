#ifndef __MENU_H
#define __MENU_H

#include "stm32f10x.h"
#include "Input.h"

/* 菜单初始化 */
void Menu_Init(void);

/* 菜单事件处理 */
void Menu_Process(Input_Event_t Event);

#endif
