#include "cmd.h"

char *sys_tips = "\
Welcom to esp32 system!\n \
You can use these commands to change functions:\n\
1. set uart b [baudrate]\n \
2. set apmode ssid [WF01]\n \
3. set wlan join [4]\n \
4. set wlan chan [1]\n \
5. set ip address [192.168.1.1]\n\
6. set ip gateway [192.168.1.1]\n\
7. set ip netmask [255.255.255.0]\n\
8. set ip dhcp [4]\n \
9. set tcp -- switch to tcp/ip mode\n \
10. set udp -- switch to udp mode\n \
11. set bt  -- switch to bluetooth mode\n \
12. hi		 -- show this welcom message\n \
Have a nice day!\n";

int get_cmd_header(char *str)
{
	return 0;
}

char *get_cmd(char *str)
{
	return NULL;
}
