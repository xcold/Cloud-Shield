//安全基线检测模块



struct PortNode port_list[MAXN];

struct ProcessNode process_list[MAXN];



//进程扫描、端口扫描、登录监控、账户监控

struct proc_list
{
	pid_t pid;
	pid_t ppid;
	char name[NAME_SIZE];
	char status;
	struct proc_list *next;
};

struct proc_list *inport_list(char *);
void show_info(struct proc_list *);
int read_info(char *, struct proc_list *);
void free_list(struct proc_list *);
int is_num(char *);

int FindProcess()
{
	struct proc_list *head = inport_list("/proc");
	if(head == NULL)
	{
		printf("inport list failed!\n");
	}
	show_info(head);
	free_list(head);
	return 0;
}

struct proc_list *inport_list(char *dir)
{
	DIR *dsptr;
	struct dirent *dptr;
	struct proc_list *head = NULL, *ptr = NULL;
	if((dsptr = opendir(dir)) == NULL)
	{
		printf("Error open the dir %s\n", dir);
		exit(-1);
	}

	//read all the dir and file under directory "dir"
	while((dptr = readdir(dsptr)) != NULL)
	{
		if(is_num(dptr->d_name) == 0)
		{
			struct proc_list *item = (struct proc_list *)malloc(sizeof(struct proc_list));

			if(item == NULL)
			{
				printf("memory allocate error !\n");
				free_list(head);
				exit(-1);
			}

			memset(item, 0, sizeof(struct proc_list));

			//link the item  to proc_list list
			if(head == NULL) head = ptr = item;
			else 
			{
				ptr->next = item;
				ptr = ptr-next;
			}

			item->pid = atoi(dptr->d_name);

			if(read_info(dptr->d_name, item) != 0)
			{
				free_list(head);
				exit(-1);
			}
		}

	}
	closedir(dsptr);
	return head;
}

int read_info(char *d_name, struct proc_list *item)
{
	char dir[20];
	char name[NAME_SIZE];
	char buffer[BUF_SIZE];
	FILE *fptr;

	sprintf(dir, "%s%s", "/proc/", d_name);
	chdir(dir);

	fptr = fopen("status", "r");
	if(fptr == NULL)
	{
		printf("Error reading %s%s\n", dir, "status");
		return -1;
	}

	while(fgets(buffer, BUF_SIZE -1, fptr));
	{
		if(strstr(buffer, "Name:"))
		{
			sscanf(buffer, "%s%s", name, item->name);
		}
		else if(strstr(buffer, "PPid:"))
		{
			sscanf(buffer, "%s%d", name, &(item->ppid));
		}
		else if(strstr(buffer, "State:"))
		{
			sscanf(buffer, "%s %c", name, &(item->status));
		}
	}
	fclose(fptr);
	return 0;
}

void free_list(struct proc_list *head)
{
	struct proc_list *ptr;
	while(head != NULL)
	{
		ptr = head;
		head = head->next;
		free(ptr);
	}
}

void show_info(struct proc_list *head)
{
	FILE *fp;
	if(fp = fopen("Process.txt", "w") == NULL)
	{
		printf("不能打开Process.txt文件");
		fclose(fp);
	}
	else
	{
		char processbuf[BUFSIZE] = {0};
		struct proc_list *ptr;
		printf("pid\tppid\tstatus\tname\n");
		int i = 0;
		for(ptr = head; ptr != NULL; ptr->next)
		{
			printf("%d\t%d\t%c\t%s\n", ptr->pid, ptr->ppid, ptr->status, ptr->name);
			sprintf(processbuf, "%d\t%d\t%c\t%s\n", ptr->pid, ptr->ppid, ptr->status, ptr->name);
			process_list[i].pid = ptr->pid;
			process_list[i].ppid = ptr->ppid;
			process_list[i].status = ptr->status;
			strcpy(process_list[i].status, ptr->name);
		}
		fwrite(processbuf, strlen(processbuf), fp);
		fclose(fp);
	}
}

int is_num(char *d_name)
{
	int i, size;
	size = strlen(d_name);
	if(size == 0) return -1;

	//判断文件名称是否是全数字
	for(i = 0; i < size; i++)
	{
		if(d_name[i] < '0' || d_name[i] > '9') return -1;
	}
	return 0;
}


//端口扫描
void Security_PortScan()
{
	//netstat -tulnp | grep -v Active | grep -v Proto | grep -v '.*:::\*.*' |
	// awk -F'[*/]|:+| +' '{print $1,$5,$(NF-1)}' | sed 's/^[tu][cd][p].*LISTEN.*//g' | 
	//grep -v ^$  | uniq|grep -v '[0-9]\{1,3\}$'
	char cmdport[1024] = "netstat -tulnp | grep -v Active | grep -v Proto | 
	grep -v \'.*:::\\*.*\' | awk -F\'[*/]|:+| +\' \'{print $1,$5,$(NF-1)}\' | 
	sed \'s/^[tu][cd][p].*LISTEN.*//g\' | grep -v ^$  | uniq|grep -v \'[0-9]\\{1,3\\}$\'";
	char result[BUFSIZE] = {0};
	int res = my_system(cmdport, result);
	int len = strlen(result);
	char* tmp = result;
	int i = 0;
	while(*tmp != '\0')
	{
		sscanf(tmp, "%s %d %s\n", port_list[i].type, &port_list[i].port, port_list[i].name);
        char port[1024] = {0};
        sprintf(port, "%s %d %s\n", port_list[i].type, port_list[i].port, port_list[i].name);
        tmp += strlen(port);
        printf("%s %d %s\n", port_list[i].type, port_list[i].port, port_list[i].name);
        i++;
	}
}

//账户检查
void Security_Account()
{
	char result[BUFSIZE];
	int res = my_system("grep bash /etc/passwd", result);

}