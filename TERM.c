
#include <windows.h>
#include <string.h>
#include <fcntl.h> 
#include <io.h> 
#include <stdlib.h> 
#include <stdio.h>




int		hCrt;
FILE *file=0;
int filelen=0;
char filename[300];
char *fbuffer;
int echo=FALSE;
int ansi_mode=FALSE;



void clrscr ()
{
	DWORD nchars;
	HANDLE handle = GetStdHandle (STD_OUTPUT_HANDLE);
	COORD xy_first = { 0, 0 };
	
	// fill scr buf with spaces, 'nchars' is not needed here
	FillConsoleOutputCharacter (handle, ' ',  80*50, xy_first, &nchars);
	SetConsoleCursorPosition (handle, xy_first); 
}


UINT_PTR CALLBACK OFNHookProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
		SetForegroundWindow(hDlg);
		return TRUE;
	default:  
		return FALSE;
	}

}

char * GetFile()
{
	static TCHAR szFilter[] = TEXT ("Hex Files (*.hex)\0*.hex;\0All Files\0*.*;\0\0") ;
	static TCHAR szFileName [MAX_PATH], szTitleName [MAX_PATH];
	static OPENFILENAME ofn ;
	memset(&ofn,0,sizeof (OPENFILENAME));
	memset(szFileName,0,sizeof(szFileName));

	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.lpstrFilter       = szFilter ;
	ofn.lpstrFile         = szFileName ;          // Set in Open and Close functions
	ofn.nMaxFile          = MAX_PATH ;
	ofn.lpstrFileTitle    = szTitleName ;          // Set in Open and Close functions
	ofn.nMaxFileTitle     = MAX_PATH ;
	ofn.lpfnHook		  = OFNHookProc;
	ofn.Flags			  = OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING;
	ofn.lpstrInitialDir   = ".";

	GetOpenFileName (&ofn) ;
	return ofn.lpstrFile;
}

int key_ctrl=FALSE;
int key_shift=FALSE;
int extended_key=FALSE;
int getkey()
{
	int i=0;
	key_ctrl=FALSE;
	key_shift=FALSE;
	extended_key=FALSE;
	i=getch();
	if (GetKeyState(VK_SHIFT) < 0)
		key_shift=TRUE;
	if(GetKeyState(VK_CONTROL) < 0)
		key_ctrl=TRUE;
	if(i==0 || i==0xE0)
	{
		i=getch();
		extended_key=TRUE;
	}
	return i&0xFF;

}
int getkey2()
{
	int i=0;
	key_ctrl=FALSE;
	key_shift=FALSE;
	extended_key=FALSE;
	if(kbhit())
	{
		i=getch();
		if(i==0 || i==0xE0)
		{
			i=getch();
			extended_key=TRUE;
		}
		if (GetKeyState(VK_SHIFT) < 0)
			key_shift=TRUE;
		if(GetKeyState(VK_CONTROL) < 0)
			key_ctrl=TRUE;

	}
	return i&0xFF;

}
int set_baud(HANDLE hComm,int baud_rate)
{
	DCB	config_comm;

    if (GetCommState(hComm,&config_comm) == 0)
    {
      printf("Get configuration port failed\r\n");
      return FALSE;
    }

	config_comm.BaudRate=baud_rate;
	config_comm.ByteSize=8;
	config_comm.Parity=NOPARITY;
	config_comm.StopBits=ONESTOPBIT;
	config_comm.fOutX=0;
	config_comm.fInX=0;
	if (SetCommState(hComm,&config_comm) == 0)
	{
      printf("Set configuration port failed\r\n");
      return FALSE;
	}
	return TRUE;
}

HANDLE connect_device(char *port_name,int baud_rate)
{
	DCB	config_comm;
	COMMTIMEOUTS comTimeOut;
	HANDLE hComm=0;


	hComm = CreateFile(port_name,	// Specify port device: default "\\.\COM1"
		GENERIC_READ|GENERIC_WRITE,	// Specify mode that open device.
		0,							// share mode.
		NULL,                       // the object gets a default security.
		OPEN_EXISTING,              // Specify which action to take on file. 
		0,							// overlap
		NULL);

	if (hComm == INVALID_HANDLE_VALUE)	// error opening port; abort
	{
		printf("Could not open port: %s\r\nCOM port may be in use by another program\n",port_name);
		return 0;
	}	
	// Get current configuration of serial communication port.
    if (GetCommState(hComm,&config_comm) == 0)
    {
      printf("Get configuration port failed\r\n");
    }

	config_comm.BaudRate=baud_rate;
	config_comm.ByteSize=8;
	config_comm.Parity=NOPARITY;
	config_comm.StopBits=ONESTOPBIT;
	config_comm.fOutX=0;
	config_comm.fInX=0;

	if (SetCommState(hComm,&config_comm) == 0)
	{
      printf("Set configuration port failed\r\n");
	}

	GetCommTimeouts(hComm,&comTimeOut);
	
    comTimeOut.ReadIntervalTimeout = 20;
    comTimeOut.ReadTotalTimeoutMultiplier = 0;
    comTimeOut.ReadTotalTimeoutConstant = 10;
    comTimeOut.WriteTotalTimeoutMultiplier = 20;
    comTimeOut.WriteTotalTimeoutConstant = 10;
    SetCommTimeouts(hComm,&comTimeOut);
	return hComm;
}

int find_all_ports()
{
	HANDLE hComm;
	char port_name[80];
	char devname[80];
	int i;

	printf("Listing available COM ports:\n");

	for(i=0;i<19;i++)
	{
		sprintf(port_name,"COM%i",i);
		sprintf(devname,"\\\\.\\COM%i",i);
		hComm = CreateFile(devname,  // Specify port device: default "COM1"
			GENERIC_READ | GENERIC_WRITE,       // Specify mode that open device.
			0,                                  // the devide isn't shared.
			NULL,                               // the object gets a default security.
			OPEN_EXISTING,                      // Specify which action to take on file. 
			0,                              
			NULL);

		if (hComm != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hComm);
			printf("%s\n",port_name);
		}

	}
	return TRUE;
}


int get_xy(POINT *p)
{
	HANDLE hcon;
	CONSOLE_SCREEN_BUFFER_INFO conbuf;
	hcon=GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hcon,&conbuf);
	if(p!=0)
	{
		//curs pos is zero based
		p->x=conbuf.dwCursorPosition.X;
		p->y=conbuf.dwCursorPosition.Y;
	}
}

int goto_xy(POINT p)
{
	HANDLE hcon;
	COORD c;
	hcon=GetStdHandle(STD_OUTPUT_HANDLE);
	c.X=p.x;
	c.Y=p.y;
	SetConsoleCursorPosition(hcon,c);
}

int process_ansi_command(char C, int n, int m)
{
	POINT p,t;
	static POINT old_pos;
	int i;
	switch(C)
	{
		case 'A': //cursor up
			get_xy(&p);
			p.y-=n;
			if(p.y<0)
				p.y=0;
			goto_xy(p);
			break;
		case 'B': //cursor down
			get_xy(&p);
			if(n<80)
			{
				p.y+=n;
				goto_xy(p);
			}
			break;
		case 'C': //cursor forward
			get_xy(&p);
			if(n<80)
			{
				p.x+=n;
				goto_xy(p);
			}
			break;
		case 'D': //cursor back
			get_xy(&p);
			if(n<80)
			{
				p.x-=n;
				goto_xy(p);
			}
			break;
		case 'E': //next line
			get_xy(&p);
			p.y+=n;
			p.x=0;
			goto_xy(p);
			break;
		case 'F': //prev line
			get_xy(&p);
			p.y-=n;
			p.x=0;
			goto_xy(p);
			break;
		case 'G': //goto horizontal position
			get_xy(&p);
			p.x=n;
			goto_xy(p);
			break;

		case 'f':
		case 'H': //goto row n col m
			p.y=n; 
			p.x=m;
			goto_xy(p);
			break;
		case 'J': //erase data
			switch(n)
			{
			case 0: //clear to end of screen
				n=n;
				break;
			case 1: //clear to beginning of screen
				n=n;
				break;
			case 2: //clear entire screen
				clrscr();
				break;
			}
			break;
		case 'K': //erase data in line
			switch(n)
			{
			case 0: //clear to end of line
				get_xy(&p);
				t=p;
				for(i=t.x;i<80;i++)
					putchar(' ');
				goto_xy(p);
				break;
			case 1: //clear to beginning of line
				get_xy(&p);
				t=p;
				t.x=0;
				for(i=0;i<t.y;i++)
					putchar(' ');
				goto_xy(p);
				break;
			case 2: //clear entire line
				get_xy(&p);
				t=p;
				t.x=0;
				goto_xy(t);
				for(i=0;i<80;i++)
					putchar(' ');
				goto_xy(p);
				break;
			}
			break;
		case 'S': //scroll up
			p=p;
			break;
		case 'T': //scroll down
			p=p;
			break;
		case 'u': //restore cursor position
			goto_xy(old_pos);
			break;
		case 's': //save cursor position
			get_xy(&old_pos);
			break;




	}
}
int parse_ansi(char *buffer, int length)
{
	int i;
	static int state=0;
	static int parse_len=0;
	static int n=0,m=0;

	for(i=0;i<length;i++)
	{
		switch(state)
		{
			case 0:
				if(buffer[i]==0x1b)
				{
					state=1;
				}
				else
					putchar(buffer[i]);
				break;
			case 1:
				if(buffer[i]=='[')
				{
					n=m=0;
					parse_len=0;
					state=2;
				}
				break;
			case 2:
				if(buffer[i]==';')
					state=3;
				else if((buffer[i]>='A') && (buffer[i]<='z'))
				{
					process_ansi_command(buffer[i],n,m);
					state=0;
				}
				else
				{
					if((buffer[i]>='0') && (buffer[i]<='9'))
					{
					n*=10;
					n+=buffer[i]-'0';
					}
				}
				break;
			case 3:
				if((buffer[i]>='A') && (buffer[i]<='z'))
				{
					process_ansi_command(buffer[i],n,m);
					state=0;
				}
				else
				{
					if((buffer[i]>='0') && (buffer[i]<='9'))
					{
					m*=10;
					m+=buffer[i]-'0';
					}
				}

				break;
		}
		if(state>0)
			parse_len++;
		else
			parse_len=0;

		if(parse_len>20)
			state=0;


	}

}
/*
char log1[]={0x4,0xd,0};
char log2[]="sta\r";
char resp1[]= " rtyryrENTER SELECTIONrtytr";
char resp2[]="rtrtryFMU Idle Comm Timerrtyr";
char resp3[]="rtyrtWAITING_FOR_HOSErtyrt";
char resp4[]="rtyrtyHOSE_INSERTEDrty";
char resp5[]="rtyrtHOSE_ENUMERATEDrtyrt";

int sniffer(HANDLE hComm,char buffer[])
{
	static int index=0;
	int i,length;
	static int state=0;

//	printf("state=%i,index=%i\n",state,index);
	i=getkey2();
	if(i=='1')
	{
		printf("xxx\n");
		WriteFile(hComm,resp2,strlen(resp2),&length,NULL);

	}
	switch(state)
	{
	case 0:
		for(i=0;i<100;i++)
		{
			if(buffer[i]==0)
				break;
			if(buffer[i]==log1[index])
				index++;
			else
				index=0;
			if(log1[index]==0)
			{
				index=0;
				state++;
				printf("sending enter selection\n");
				WriteFile(hComm,resp1,strlen(resp1),&length,NULL);
				break;

			}
		}
		break;
	case 1:
		for(i=0;i<100;i++)
		{
			if(buffer[i]==0)
				break;
			if(buffer[i]==log2[index])
				index++;
			else
				index=0;
			if(buffer[i]==log1[index] && index==0)
			{
				state=0;
				index=0;
			}
			if(log2[index]==0)
			{
				index=0;
				state++;
				printf("sending FMU Idle Comm Time\n");
				WriteFile(hComm,resp2,strlen(resp2),&length,NULL);
				break;

			}
		}
		break;
	case 2:
		printf("***state2***\n");
		state++;
		WriteFile(hComm,resp3,strlen(resp3),&length,NULL);
		break;
	case 3:
		state++;
		WriteFile(hComm,resp4,strlen(resp4),&length,NULL);
		break;
	case 4:
		state++;
		WriteFile(hComm,resp5,strlen(resp5),&length,NULL);
		break;
	case 5:
		state=0;
		break;
	default:
		state=0;
		index=0;
		break;
	}
}
int is_second()
{
	static DWORD tick=0,tick2=0;

	if(tick==0)
		tick=GetTickCount();

	tick2=GetTickCount()-tick;

	if(tick2>400)
	{
		tick=GetTickCount();
		return TRUE;
	}
	return FALSE;

}
*/
void set_console_title(char *port_name,int baud_rate,int echo,int ansi)
{
	char buffer[255];

	sprintf(buffer,"%s %i %s %s(ctrl-c=quit,F1=echo F9=cls F10=ansi F11=reopen F12=baud)",port_name,baud_rate,echo ? "ECHO":"",
		ansi ? "ANSI" : "");
	SetConsoleTitle(buffer);
}
DWORD WINAPI read_thread(HANDLE hComm)
{
	char buffer[0x100];
	int i,length;
	DWORD dwEvents;

//	SetCommMask(hComm,EV_RXCHAR);
	do{
		memset(buffer,0,sizeof(buffer));
		length=0;
//		WaitCommEvent( hComm, &dwEvents, NULL);
//		Sleep(1);
		ReadFile(hComm,buffer,0xFF,&length,NULL);
//		length=0;
		if(length!=0)
		{
			if(ansi_mode!=0)
				parse_ansi(buffer,length);
			else
			{
				for(i=0;i<length;i++)
					printf("%c",buffer[i]);
			}
		}

	}while(TRUE);
}

//////////////////////////////////////////////////////////////////////////////////////
mode_terminal(HANDLE hComm,char *port_name,int baud_rate)
{
	int i,j,length,key;
	char buffer[0x100],RXbuf;

	HANDLE hcon;
	CONSOLE_CURSOR_INFO con_cursor;
	COORD coord;
	SMALL_RECT rect;
	DWORD time1;

	printf("clearing RX buffer\n");
	PurgeComm(hComm,PURGE_RXCLEAR|PURGE_RXABORT);

	hcon = GetStdHandle(STD_OUTPUT_HANDLE);
	set_console_title(port_name,baud_rate,echo,ansi_mode);

	con_cursor.bVisible=TRUE;
	con_cursor.dwSize=100;
	SetConsoleCursorInfo(hcon,&con_cursor);

	hcon = GetStdHandle(STD_OUTPUT_HANDLE);
	coord.X=80;
	coord.Y=30;
	SetConsoleScreenBufferSize(hcon,coord);
	rect.Bottom=30-1;
	rect.Top=0;
	rect.Left=0;
	rect.Right=80-1;
	SetConsoleWindowInfo(hcon,TRUE,&rect);

	j=0;
	clrscr();
		

//	CreateThread(NULL,NULL,read_thread,hComm,0,&i);

	do{

		Sleep(20);
		memset(buffer,0,0x100);
		length=0;
		ReadFile(hComm,buffer,0xFF,&length,NULL);

		if(length!=0)
		{
			if(ansi_mode!=0)
				parse_ansi(buffer,length);
			else
			{
				for(i=0;i<length;i++)
					printf("%c",buffer[i]);
			}
		}

	
		if(kbhit())
		{
		key=getkey();


		if(extended_key)
		{
			switch(key)
			{	
				case ';':
					echo^=TRUE;
					set_console_title(port_name,baud_rate,echo,ansi_mode);
					break;
				case 0x43:
					clrscr();
					break;
				case 0x44: 
					ansi_mode^=TRUE;
					set_console_title(port_name,baud_rate,echo,ansi_mode);
					break;
				case 0x85: //F11
					CloseHandle(hComm);
					hComm=connect_device(port_name,baud_rate);
					if(hComm==0)
					{
						printf("\nCant reopen port %s\n",port_name);
						Sleep(250);
						return;
					}
					printf("\nport reopened\n");
					//RXbuf=VK_ESCAPE;
					//WriteFile(hComm,&RXbuf,1,&length,NULL);

					break;
				case 0x86: //F12
					{
						int baud=14400;
						printf("\nEnter new baud rate:");
						scanf("%i",&baud);
						if(set_baud(hComm,baud))
						{
							baud_rate=baud;
							set_console_title(port_name,baud_rate,echo,ansi_mode);
						}
					}
					break;
				
			}
		}
		else
		{
			if(echo){
				if(key=='\b')
					printf(" \b\b");
				else
					printf("%c",key);
				if(key==0xD) //enter
					printf("\n"); //clrscr();
			}
			RXbuf=key;
			WriteFile(hComm,&RXbuf,1,&length,NULL);
		}




		}
	}while(TRUE);

	con_cursor.bVisible=TRUE;
	con_cursor.dwSize=25;
	SetConsoleCursorInfo(hcon,&con_cursor);


}


int main(int argc, char **argv)
{
	char port_name[255]={0};
	int port,baud_rate=19200;
	HANDLE	hComm;

	SetConsoleTitle("TERM (build: "__DATE__ "  " __TIME__")");
	if(argc>1)
	{
		int i;
		for(i=1;i<argc;i++)
		{
			if(strstr(argv[i],"-baud="))
			{
				sscanf(argv[i],"-baud=%u",&baud_rate);
				printf("baud rate=%i\n",baud_rate);
			}
			else if(strstr(argv[i],"-echo"))
			{
				echo=TRUE;
				printf("echo enabled\n");
			}
			else if(strstr(argv[i],"-ansi"))
			{
				ansi_mode=TRUE;
				printf("ansi mode enabled\n");
			}
			else if(strstr(argv[i],"-port"))
			{
				port_name[0]=0;
				sscanf(argv[i],"-port=%254s",port_name);
				printf("using port name %s\n",port_name);
			}
			else 
			{
				printf("usage: [-port=\\\\.\\COM1] [-echo] [-ansi] [-baud=#]\n");
			}
		}
	}
	if(port_name[0]==0){
retry:
		port_name[0]=0;
		find_all_ports();
		printf("enter port selection:\n");
		scanf("%254s",port_name);
		port_name[sizeof(port_name)-1]=0;
		port=-1;
		if(sscanf(port_name,"%i",&port)!=1){
			char tmp[sizeof(port_name)]={0};
			strncpy(tmp,port_name,sizeof(tmp));
			tmp[sizeof(tmp)-1]=0;
			strlwr(tmp);
			sscanf(tmp,"COM%i",&port);
		}
		if(port!=-1){
			sprintf(port_name,"\\\\.\\COM%i",port);
			printf("port=%i\n",port);
		}
	}
	hComm=connect_device(port_name,baud_rate);
	if(hComm==0)
		goto retry;

	mode_terminal(hComm,port_name,baud_rate);
	CloseHandle(hComm);

}