//消息处理模块、对云盾服务端发来的消息进行过滤处理

#include "./../PublicAPI.h"
#include "SolveMsg.h"
#include "./../Log/Log.h"
#include "./../Plugin/Plugin.h"
#include "./../cJSON/cJSON.h"
#include "./../Plugin/PluginOperate.h"

//==================================================
//数据定义
//==================================================
#define MAXLEN 40960


//==================================================
//对内声明函数
//==================================================


//消息处理中心
//负责处理从服务端发来的消息
//对接收消息进行严格的过滤、防止恶意攻击字符与溢出
void Tcp_Msg_Solve(char* buf)
{
	//printf("%s\n", buf);
	Agent_Log(buf);
	
	cJSON *json , *json_cmd;  
	cJSON *json_json, *json_title, *json_second, *json_times, *json_parameter;
	cJSON *json_operate_name, *json_operate_imid;
    // 解析数据包
    json = cJSON_Parse(buf);
    if (!json)
    {  
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());  
    }  
    else
    {
        // 解析cmd  
        json_cmd = cJSON_GetObjectItem(json , "cmd");  
        if( json_cmd->type == cJSON_Number )  
        {
            // 从valueint中获得结果  
            printf("value:%d\r\n",json_cmd->valueint);  
        }  
        // 根据cmd的值来判断是哪种操作
        switch(json_cmd->valueint)
        {
        	case 0002://注册成功
        		printf("agent register sucess!\n");
        		break;
        	case 3108://同步脚本信息
				AgentPluginMsgToServer();
        		break;
        	case 3103://修改执行频率和次数
				printf("修改执行频率和次数\n");
				json_json = cJSON_GetObjectItem(json , "json");
				if(json_json)
				{
					json_title = cJSON_GetObjectItem(json_json , "script_title");
					json_second = cJSON_GetObjectItem(json_json , "run_second");
					json_times = cJSON_GetObjectItem(json_json , "run_times");
				}
				if(!(json_title && json_second && json_times))
				{
					Agent_Log("修改执行频率和次数参数错误");
					printf("修改执行频率和次数参数错误\n");
					break;
				}
				printf("修改执行频率和次数\n");
				AgentPluginRateTime(json_title->valuestring, json_second->valueint, json_times->valueint);
        		break;
        	case 3101://脚本开启
				printf("脚本开启\n");
				
				json_json = cJSON_GetObjectItem(json , "json");
				if(json_json)
				{
					json_title = cJSON_GetObjectItem(json_json , "script_title");
					json_second = cJSON_GetObjectItem(json_json , "run_second");
					json_times = cJSON_GetObjectItem(json_json , "run_times");
					json_parameter = cJSON_GetObjectItem(json_json , "custom_parameter");//参数有点问题
					
					json_operate_name = cJSON_GetObjectItem(json_json , "operate_name");
					json_operate_imid = cJSON_GetObjectItem(json_json , "operate_imid");
				}
				if(!(json_title && json_second && json_times && json_operate_name && json_operate_imid && json_parameter))
				{
					Agent_Log("脚本开启参数错误!");
					printf("脚本开启参数错误!");
					break;
				}
				//char log[1024] = {0};
				//sprintf("3101   titile:%s  second:%d  times:%d  parameter")
				Agent_Log("脚本开启");
				AgentPluginStart(json_title->valuestring, json_second->valueint, json_times->valueint, json_parameter->valuestring,json_operate_name->valuestring, json_operate_imid->valueint);
        		break;
        	case 3102://脚本停用
				printf("脚本停用\n");
				json_json = cJSON_GetObjectItem(json , "json");
				if(json_json)
				{
					json_title = cJSON_GetObjectItem(json_json , "script_title");
					json_operate_name = cJSON_GetObjectItem(json_json , "operate_name");
					json_operate_imid = cJSON_GetObjectItem(json_json , "operate_imid");
				}
				if(!(json_title && json_operate_name && json_operate_imid))
				{
					Agent_Log("脚本停用参数错误!");
					printf("脚本停用参数错误!");
					break;
				}
				Agent_Log("脚本停用");
				AgentPluginStop(json_title->valuestring,json_operate_name->valuestring, json_operate_imid->valueint);
        		break;
        	case 3104://临时执行一次脚本
        		json_json = cJSON_GetObjectItem(json , "json");
				if(json_json)
				{
					json_title = cJSON_GetObjectItem(json_json , "script_title");
					json_operate_name = cJSON_GetObjectItem(json_json , "operate_name");
					json_operate_imid = cJSON_GetObjectItem(json_json , "operate_imid");
				}
				if(!(json_title && json_operate_name && json_operate_imid))
				{
					Agent_Log("脚本临时执行参数错误!");
					printf("脚本临时执行参数错误!");
					break;
				}
				AgentPluginRunOnce(json_title->valuestring, json_operate_name->valuestring, json_operate_imid->valueint);
				break;
        	default:
				printf("else operation!\n\n");
        }

        // 释放内存空间  
        cJSON_Delete(json);  
    }  
}

