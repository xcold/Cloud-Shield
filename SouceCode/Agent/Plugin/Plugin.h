#ifndef _PLUGIN_H
#define _PLUGIN_H

//==================================================
//数据定义
//==================================================






//==================================================
//对外函数借口
//==================================================
int Plugin_Init();

void Plugin_Manage();

int Plugin_GetStatus();

void Plugin_SetStatus(int status);

#endif
