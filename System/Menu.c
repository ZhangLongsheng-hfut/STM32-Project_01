#include "stm32f10x.h"
#include "Menu.h"
#include "OLED.h"
#include "AD.h"
#include "Delay.h"
#include "Key.h"
#include "MPU6050.h"
#include "PWM.h"
#include "Serial.h"

#define MENU_VISIBLE_ROWS 4

/*====================================================
    1. 类型定义
====================================================*/

/* 菜单运行状态 */
typedef enum
{
    MENU_STATE_MENU = 0,    // 当前处于菜单
    MENU_STATE_PAGE         // 当前处于功能页面

} Menu_State_t;


/* 菜单项结构体 */
typedef struct MenuItem
{
    const char *name;               // 菜单名称

    void (*action)(void);           // 功能函数

    struct MenuItem *parent;        // 父菜单

    struct MenuItem **children;     // 子菜单数组

    uint8_t childCount;             // 子菜单数量

} MenuItem;


/*====================================================
    2. 内部函数声明
====================================================*/

static void Menu_Show(void);
static void Menu_ShowCursor(void);

static void Menu_Enter(void);
static void Menu_Back(void);

static uint8_t Menu_FindChildIndex(MenuItem *parent, MenuItem *child);
static void PWM_ShowDuty(uint8_t Duty);


/* 功能页面 */
static void ADC_Page(void);
static void MPU6050_Page(void);
static void PWM_Page(void);

static void KeyTest_Page(void);
static void EncoderTest_Page(void);
static void SystemInfo_Page(void);

static void USART_Page(void);
static void Settings_Page(void);
static void About_Page(void);


/*====================================================
    3. 菜单项提前声明
====================================================*/

static MenuItem RootMenu;

static MenuItem ADCItem;
static MenuItem MPU6050Item;
static MenuItem PWMItem;
static MenuItem ToolsItem;

static MenuItem KeyTestItem;
static MenuItem EncoderTestItem;
static MenuItem SystemInfoItem;

static MenuItem USARTItem;
static MenuItem SettingsItem;
static MenuItem AboutItem;


/*====================================================
    4. 子菜单数组
====================================================*/

/* Tools下面的菜单 */
static MenuItem *ToolsChildren[] =
{
    &KeyTestItem,
    &EncoderTestItem,
    &SystemInfoItem
};


/* Root下面的菜单 */
static MenuItem *RootChildren[] =
{
    &ADCItem,
    &MPU6050Item,
    &PWMItem,
    &ToolsItem,
    &USARTItem,
    &SettingsItem,
    &AboutItem
};


/*====================================================
    5. 菜单数据
====================================================*/

/* 根菜单 */
static MenuItem RootMenu =
{
    "ROOT",             // name
    0,                  // action
    0,                  // parent
    RootChildren,       // children
    7                   // childCount
};


/* ADC菜单 */
static MenuItem ADCItem =
{
    "ADC Monitor",
    ADC_Page,
    &RootMenu,
    0,
    0
};


/* MPU6050菜单 */
static MenuItem MPU6050Item =
{
    "MPU6050",
    MPU6050_Page,
    &RootMenu,
    0,
    0
};


/* PWM菜单 */
static MenuItem PWMItem =
{
    "PWM Control",
    PWM_Page,
    &RootMenu,
    0,
    0
};


/* Tools菜单 */
static MenuItem ToolsItem =
{
    "Tools",
    0,
    &RootMenu,
    ToolsChildren,
    3
};


/* USART Debug菜单 */
static MenuItem USARTItem =
{
    "USART Debug",          // 菜单显示名称
    USART_Page,             // 按下ENTER后执行的功能函数
    &RootMenu,              // 父菜单是RootMenu
    0,                      // 没有子菜单
    0                       // 子菜单数量为0
};


/* Settings菜单 */
static MenuItem SettingsItem =
{
    "Settings",             // 菜单显示名称
    Settings_Page,          // 按下ENTER后执行的功能函数
    &RootMenu,              // 父菜单是RootMenu
    0,                      // 没有子菜单
    0                       // 子菜单数量为0
};


/* About菜单 */
static MenuItem AboutItem =
{
    "About",                // 菜单显示名称
    About_Page,             // 按下ENTER后执行的功能函数
    &RootMenu,              // 父菜单是RootMenu
    0,                      // 没有子菜单
    0                       // 子菜单数量为0
};
/* Key Test */
static MenuItem KeyTestItem =
{
    "Key Test",
    KeyTest_Page,
    &ToolsItem,
    0,
    0
};


/* Encoder Test */
static MenuItem EncoderTestItem =
{
    "Encoder Test",
    EncoderTest_Page,
    &ToolsItem,
    0,
    0
};


/* System Info */
static MenuItem SystemInfoItem =
{
    "System Info",
    SystemInfo_Page,
    &ToolsItem,
    0,
    0
};


/*====================================================
    6. 当前菜单状态
====================================================*/

/* 当前所在菜单 */
static MenuItem *CurrentMenu = &RootMenu;


/* 当前选中的菜单项
   例如：
   selected = 0 表示选中第1项
   selected = 4 表示选中第5项 */
static uint8_t selected = 0;


/* OLED当前第一行所显示的菜单项编号
   例如：
   topIndex = 0 -> OLED从第1个菜单项开始显示
   topIndex = 1 -> OLED从第2个菜单项开始显示
   topIndex = 2 -> OLED从第3个菜单项开始显示 */
static uint8_t topIndex = 0;


/* 当前程序状态 */
static Menu_State_t Menu_State = MENU_STATE_MENU;


/*====================================================
    7. 菜单初始化
====================================================*/

void Menu_Init(void)
{
    /* 初始进入根菜单 */
    CurrentMenu = &RootMenu;

    /* 默认选中第1个菜单项 */
    selected = 0;

    /* OLED默认从第1个菜单项开始显示 */
    topIndex = 0;

    /* 当前处于菜单状态 */
    Menu_State = MENU_STATE_MENU;

    /* 显示菜单内容 */
    Menu_Show();

    /* 显示菜单光标 */
    Menu_ShowCursor();
}


/*====================================================
    8. 菜单事件处理
====================================================*/

void Menu_Process(Input_Event_t Event)
{
    /* 当前处于菜单状态 */
    if (Menu_State == MENU_STATE_MENU)
    {
        /* 向下 */
		if (Event == INPUT_DOWN)
		{
			/* 只要当前不是最后一个菜单项，就允许继续向下 */
			if (selected < CurrentMenu->childCount - 1)
			{
				/* 当前选中项向下移动一项 */
				selected++;

				/* 判断选中项是否已经跑出了OLED的显示范围
				   
				   例如：
				   topIndex = 0
				   OLED能显示0、1、2、3

				   如果selected变成4，
				   说明第4号菜单项已经超出了当前4行显示范围 */
				if (selected >= topIndex + MENU_VISIBLE_ROWS)
				{
					/* OLED显示窗口向下移动一项 */
					topIndex++;

					/* 因为显示内容发生变化，所以重新显示菜单 */
					Menu_Show();
				}

				/* 更新光标位置 */
				Menu_ShowCursor();
			}
		}

        /* 向上 */
		else if (Event == INPUT_UP)
		{
			/* selected大于0时才允许继续向上 */
			if (selected > 0)
			{
				/* 当前选中项向上移动一项 */
				selected--;

				/* 如果selected已经跑到了当前OLED显示范围上面
				
				   例如：
				   topIndex = 2
				   OLED当前显示2、3、4、5

				   如果selected变成1，
				   说明第1号菜单项已经在屏幕上方看不到了 */
				if (selected < topIndex)
				{
					/* OLED显示窗口向上移动一项 */
					topIndex--;

					/* 显示内容发生变化，重新显示菜单 */
					Menu_Show();
				}

				/* 更新光标位置 */
				Menu_ShowCursor();
			}
		}

        /* 进入 */
        else if (Event == INPUT_ENTER)
        {
            Menu_Enter();
        }

        /* 返回 */
        else if (Event == INPUT_BACK)
        {
            Menu_Back();
        }
    }

    /* 当前处于功能页面 */
    else if (Menu_State == MENU_STATE_PAGE)
    {
        if (Event == INPUT_BACK)
        {
            Menu_Back();
        }
    }
}


/*====================================================
    9. 显示当前菜单
====================================================*/

static void Menu_Show(void)
{
    uint8_t i;

    /* 当前OLED这一行对应的真实菜单项编号 */
    uint8_t itemIndex;

    /* 先清空OLED */
    OLED_Clear();

    /* OLED最多只能显示4行菜单 */
    for (i = 0; i < MENU_VISIBLE_ROWS; i++)
    {
        /* 计算OLED当前这一行应该显示哪个菜单项
           
           例如：
           topIndex = 2

           i = 0 -> itemIndex = 2
           i = 1 -> itemIndex = 3
           i = 2 -> itemIndex = 4
           i = 3 -> itemIndex = 5 */
        itemIndex = topIndex + i;

        /* 如果已经超过当前菜单的总数量，
           就不再继续显示 */
        if (itemIndex >= CurrentMenu->childCount)
        {
            break;
        }

        /* 显示菜单名称
           
           i + 1：
           决定显示在OLED第几行

           itemIndex：
           决定读取真正菜单中的第几项 */
        OLED_ShowString(
            i + 1,
            3,
            CurrentMenu->children[itemIndex]->name
        );
    }
}


/*====================================================
    10. 显示光标
====================================================*/

static void Menu_ShowCursor(void)
{
    uint8_t i;

    /* 先把OLED四行原来的光标全部清除 */
    for (i = 0; i < MENU_VISIBLE_ROWS; i++)
    {
        OLED_ShowString(i + 1, 1, " ");
    }

    /* 显示新的光标
       
       selected：
       当前真正选中的菜单编号

       topIndex：
       OLED第一行对应的菜单编号

       selected - topIndex：
       得到选中项在当前屏幕中的相对位置

       最后 +1：
       因为OLED行号是从1开始 */
    OLED_ShowString(
        selected - topIndex + 1,
        1,
        ">"
    );
}


/*====================================================
    11. ENTER处理
====================================================*/

static void Menu_Enter(void)
{
    MenuItem *item;

    /* 找到当前选中的菜单项 */
    item = CurrentMenu->children[selected];


    /* 有子菜单 */
    if (item->childCount > 0)
	{
		/* 进入当前选中的子菜单 */
		CurrentMenu = item;

		/* 新菜单默认选中第1项 */
		selected = 0;

		/* 新菜单默认从第1项开始显示 */
		topIndex = 0;

		/* 显示新菜单 */
		Menu_Show();

		/* 显示光标 */
		Menu_ShowCursor();
	}

    /* 没有子菜单，但有功能 */
    else if (item->action != 0)
    {
		Menu_State = MENU_STATE_PAGE;
		
        item->action();
    }
}


/*====================================================
    12. BACK处理
====================================================*/

static void Menu_Back(void)
{
    /* 当前处于功能页面 */
    if (Menu_State == MENU_STATE_PAGE)
    {
        Menu_State = MENU_STATE_MENU;

        Menu_Show();

        Menu_ShowCursor();

        return;
    }


    /* 当前处于子菜单 */
    if (CurrentMenu->parent != 0)
    {
        MenuItem *OldMenu;

        OldMenu = CurrentMenu;

        /* 回到父菜单 */
        CurrentMenu = CurrentMenu->parent;

        /* 找到刚才的菜单在父菜单中的位置 */
		selected = Menu_FindChildIndex(CurrentMenu, OldMenu);


		/* 根据selected重新计算OLED应该从哪里开始显示

		   如果selected小于4：
		   说明它可以直接显示在第一页，
		   所以topIndex = 0

		   如果selected大于等于4：
		   说明已经超过OLED第4行，
		   需要向下移动显示窗口 */
		if (selected >= MENU_VISIBLE_ROWS)
		{
			/* 让当前选中项显示在OLED最后一行 */
			topIndex = selected - MENU_VISIBLE_ROWS + 1;
		}
		else
		{
			/* 当前选中项在第一页范围内 */
			topIndex = 0;
		}


		/* 重新显示父菜单 */
		Menu_Show();

		/* 重新显示光标 */
		Menu_ShowCursor();
	}
}


/*====================================================
    13. 查找子菜单位置
====================================================*/

static uint8_t Menu_FindChildIndex(MenuItem *parent, MenuItem *child)
{
    uint8_t i;

    for (i = 0; i < parent->childCount; i++)
    {
        if (parent->children[i] == child)
        {
            return i;
        }
    }

    return 0;
}


/*====================================================
    14. 功能页面
====================================================*/

static void ADC_Page(void)
{
	Input_Event_t Event;
	uint16_t ADValue;			//定义AD值变量
	float Voltage;				//定义电压变量

    OLED_Clear();
	AD_Init();				//AD初始化
		
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "ADC Monitor");
	OLED_ShowString(2, 1, "ADValue:");
	OLED_ShowString(3, 1, "Voltage:0.00V");
	
	Serial_Printf("[ADC] Enter ADC Monitor\r\n");
		
	uint8_t Count = 0;
	uint8_t Serial_Count = 0;

	while (1)
	{
		/* 每次循环都检查输入 */
		Event = Input_GetEvent();

		if (Event == INPUT_BACK)
		{
			Menu_Process(Event);
			/* 串口记录退出ADC Monitor页面 */
			Serial_Printf("[ADC Monitor] Exit ADC Monitor\r\n");
			Serial_Printf("\r\n");
			return;
		}

		/* 每50ms左右更新一次ADC */
		Count++;
		Serial_Count++;

		if (Count >= 5)
		{
			Count = 0;

			ADValue = AD_GetValue();

			Voltage = (float)ADValue / 4095 * 3.3;

			OLED_ShowNum(2, 9, ADValue, 4);

			OLED_ShowNum(3, 9, Voltage, 1);

			OLED_ShowNum(3,11,(uint16_t)(Voltage * 100) % 100,2);
			
		}
		
		if (Serial_Count >= 100)
		{
			Serial_Count = 0;
			Serial_Printf("[ADC]ADValue:");
			Serial_SendNumber(ADValue,4);
			Serial_Printf("\r\n");
		}
		Delay_ms(10);
	}
}


static void MPU6050_Page(void)
{
	uint8_t Count = 0;
	uint8_t MPU6050_Count = 0;
	Input_Event_t Event;
	uint8_t ID;								//定义用于存放ID号的变量
	int16_t AX, AY, AZ, GX, GY, GZ;			//定义用于存放各个数据的变量
	
	OLED_Clear();  
	MPU6050_Init();		//MPU6050初始化
	Serial_Printf("[MPU6050] Enter MPU6050\r\n");
	
	while (1)
	{	
			
		/* 每次循环都检查输入 */
		Event = Input_GetEvent();

		if (Event == INPUT_BACK)
		{
			Menu_Process(Event);
			/* 串口记录退出MPU6050页面 */
			Serial_Printf("[MPU6050] Exit MPU6050\r\n");
			Serial_Printf("\r\n");
			return;
		}

		/* 每50ms左右更新一次MPU6050数据 */
		Count++;
		MPU6050_Count++;
		
		if (Count >= 5)
		{
			Count = 0;
			/*显示ID号*/
			OLED_ShowString(1, 1, "MPU6050");
			ID = MPU6050_GetID();				//获取MPU6050的ID号
			OLED_ShowHexNum(1, 4, ID, 2);		//OLED显示ID号
			
			
			MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);		//获取MPU6050的数据
			OLED_ShowSignedNum(2, 1, AX, 5);					//OLED显示数据
			OLED_ShowSignedNum(3, 1, AY, 5);
			OLED_ShowSignedNum(4, 1, AZ, 5);
			OLED_ShowSignedNum(2, 8, GX, 5);
			OLED_ShowSignedNum(3, 8, GY, 5);
			OLED_ShowSignedNum(4, 8, GZ, 5);
			
		}
		if(MPU6050_Count >= 100)
		{
			MPU6050_Count = 0;
			Serial_Printf("MPU6050 DATA\r\n");
			Serial_Printf("AX:%d    AY:%d\r\n", AX, AY);
			Serial_Printf("AZ:%d    GX:%d\r\n", AZ, GX);
			Serial_Printf("GY:%d    GZ:%d\r\n", GY, GZ);
			Serial_Printf("\r\n");
		}
		Delay_ms(10);
	}
}

static void PWM_Page(void)
{
    Input_Event_t Event;
    uint8_t Duty = 50;

    PWM_Init();

    OLED_Clear();

    OLED_ShowString(1, 1, "PWM Control");
    OLED_ShowString(2, 1, "Duty:");

    /* 设置初始占空比 */
    PWM_SetCompare2(Duty);

    /* 显示初始Duty */
    PWM_ShowDuty(Duty);
	/* 串口输出初始状态 */
	Serial_Printf("[PWM] Enter PWM Control\r\n");
	Serial_Printf("[PWM] Duty:%d%%\r\n", Duty);

    while (1)
    {
        Event = Input_GetEvent();

        /* 返回 */
        if (Event == INPUT_BACK)
		{
			PWM_SetCompare2(0);

			/* 串口记录退出PWM页面 */
			Serial_Printf("[PWM] Exit PWM Control\r\n");
			Serial_Printf("\r\n");

			Menu_Process(Event);
			return;
		}

        /* 顺时针，占空比增加 */
        else if (Event == INPUT_DOWN)
        {
            if (Duty < 100)
            {
                Duty++;

                PWM_SetCompare2(Duty);

                PWM_ShowDuty(Duty);
				
				/* 串口记录新的Duty */
				Serial_Printf("[PWM] Duty:%d%%\r\n", Duty);
            }
        }

        /* 逆时针，占空比减少 */
        else if (Event == INPUT_UP)
        {
            if (Duty > 0)
            {
                Duty--;

                PWM_SetCompare2(Duty);

                PWM_ShowDuty(Duty);
				
				/* 串口记录新的Duty */
				Serial_Printf("[PWM] Duty:%d%%\r\n", Duty);
            }
        }

        Delay_ms(10);
    }
}

static void KeyTest_Page(void)
{
    OLED_Clear();

    OLED_ShowString(1, 1, "Key Test");
    OLED_ShowString(2, 1, "Coming soon...");
}


static void EncoderTest_Page(void)
{
    OLED_Clear();

    OLED_ShowString(1, 1, "Encoder Test");
    OLED_ShowString(2, 1, "Coming soon...");
}


static void SystemInfo_Page(void)
{
    OLED_Clear();

    OLED_ShowString(1, 1, "System Info");
    OLED_ShowString(2, 1, "Coming soon...");
}

/* USART Debug功能页面 */
static void USART_Page(void)
{
    /* 清空OLED */
    OLED_Clear();

    /* 临时显示页面名称 */
    OLED_ShowString(1, 1, "USART Debug");

    /* 目前USART功能还没有实现 */
    OLED_ShowString(2, 1, "Coming soon...");
}


/* Settings功能页面 */
static void Settings_Page(void)
{
    /* 清空OLED */
    OLED_Clear();

    /* 临时显示页面名称 */
    OLED_ShowString(1, 1, "Settings");

    /* 目前Settings功能还没有实现 */
    OLED_ShowString(2, 1, "Coming soon...");
}


/* About功能页面 */
static void About_Page(void)
{
    /* 清空OLED */
    OLED_Clear();

    /* 临时显示页面名称 */
    OLED_ShowString(1, 1, "About");

    /* 目前About功能还没有实现 */
    OLED_ShowString(2, 1, "Coming soon...");
}

//PWM 中 Duty 显示函数
static void PWM_ShowDuty(uint8_t Duty)
{
    /* 先清空原来的数字和百分号 */
    OLED_ShowString(2, 7, "    ");

    if (Duty >= 100)
    {
        OLED_ShowNum(2, 7, Duty, 3);
        OLED_ShowString(2, 10, "%");
    }
    else if (Duty >= 10)
    {
        OLED_ShowNum(2, 7, Duty, 2);
        OLED_ShowString(2, 9, "%");
    }
    else
    {
        OLED_ShowNum(2, 7, Duty, 1);
        OLED_ShowString(2, 8, "%");
    }
}
