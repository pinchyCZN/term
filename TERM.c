
#include <windows.h>

#include <fcntl.h> 
#include <io.h> 
#include <stdlib.h> 
#include <stdio.h>




int		hCrt;
char portName[255];
FILE *file=0;
int filelen=0;
char filename[300];
char *fbuffer;
int echo=FALSE;
int ansi_mode=FALSE;
int byte_size=8;



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
int set_dtr(HANDLE hcomm,int flag)
{
	DCB	config_comm;
    if(GetCommState(hcomm,&config_comm) == 0){
		printf("Get configuration port failed\r\n");
		return 0;
    }

	config_comm.fDtrControl=flag;
	if(SetCommState(hcomm,&config_comm) == 0){
		printf("Set configuration port failed\r\n");
	}
	else{
		printf("set dtr=%s\n",flag?"TRUE":"FALSE");
	}
	return 0;
}
HANDLE connect_device(char portName[],int baud_rate,int bits)
{
	DCB	config_comm;
	COMMTIMEOUTS comTimeOut;
	HANDLE hComm=0;
	char devname[80];

	sprintf(devname,"\\\\.\\%s",portName);

	hComm = CreateFile(devname,	// Specify port device: default "\\.\COM1"
		GENERIC_READ|GENERIC_WRITE,	// Specify mode that open device.
		0,							// share mode.
		NULL,                       // the object gets a default security.
		OPEN_EXISTING,              // Specify which action to take on file. 
		0,							// overlap
		NULL);

	if (hComm == INVALID_HANDLE_VALUE)	// error opening port; abort
	{
		printf("Could not open port: %s\r\nCOM port may be in use by another program\n",portName);
		goto exit;
	}	
	// Get current configuration of serial communication port.
    if (GetCommState(hComm,&config_comm) == 0)
    {
      printf("Get configuration port failed\r\n");
      goto exit;
    }

	config_comm.BaudRate=baud_rate;
	config_comm.ByteSize=bits;
	config_comm.Parity=NOPARITY;
	config_comm.StopBits=ONESTOPBIT;
	config_comm.fOutX=0;
	config_comm.fInX=0;

	if (SetCommState(hComm,&config_comm) == 0)
	{
      printf("Set configuration port failed\r\n");
      goto exit;
	}

	GetCommTimeouts(hComm,&comTimeOut);
	
    comTimeOut.ReadIntervalTimeout = 20;
    comTimeOut.ReadTotalTimeoutMultiplier = 0;
    comTimeOut.ReadTotalTimeoutConstant = 10;
    comTimeOut.WriteTotalTimeoutMultiplier = 20;
    comTimeOut.WriteTotalTimeoutConstant = 10;
    SetCommTimeouts(hComm,&comTimeOut);




	return hComm;


exit:	
	CloseHandle(hComm);


	return 0;

}

int find_all_ports()
{
	HANDLE hComm;
	char portName[80];
	char devname[80];
	int i;

	printf("Listing available COM ports:\n");

	for(i=0;i<19;i++)
	{
		sprintf(portName,"COM%i",i);
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
			printf("%s\n",portName);
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
void set_console_title(char portName[],int baud_rate,int echo,int ansi,int hex)
{
	char buffer[255];

	sprintf(buffer,"%s %i %s%s%s(ctrl-c=quit,F2=echo F8=hex F9=cls F10=ansi F11=reopen F12=baud)",portName,baud_rate,echo ? "ECHO ":"",
		ansi ? "ANSI " : "", hex ? "HEX ":"");
	SetConsoleTitle(buffer);
}
int dump_data(char *buf,int len)
{
	int i;
	for(i=0;i<len;i+=16){
		int j,max;
		max=i+16;
		if(max>len)
			max=len;
		for(j=i;j<max;j++){
			unsigned char c=buf[j];
			printf("%02X ",c);
		}
		if(max!=(i+16)){
			for(j=0;j<(i+16-len);j++)
				printf("   ");
		}
		for(j=i;j<max;j++){
			unsigned char c=buf[j];
			if(c>=' ' && c<=127)
				printf("%c",c);
			else
				printf(".");
		}
		printf("\n");
	}
	return 0;
}

int get_date(char *date,int len)
{
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	return GetDateFormat(LOCALE_USER_DEFAULT,0,&systime,"yyMMdd",date,len);
}
int create_authorization(char *buffer,int len,int approved)
{
	int serial_num=12345678;
	int index=0;
	int trans_number=1234;
	int type=0;
	char *resp_code="";
	char *appr_code="123456";
	char *msg="";
	char date[40]={0};
	char pin[80]={0};
	char data_source='X';
	int tmp_amount=100;
	get_date(date,sizeof(date));
	if(approved){
		resp_code="AA";
		msg="Approved";
	}
	else{
		resp_code="NX";
		msg="Denied";
	}
	//_snprintf(buffer,len,"50.%8.8i%1.1i%4.4i%2.2s%6.6s%6.6s%s",serial_num,index,trans_number,resp_code,appr_code,date,msg);
	type=20; //Credit Sale
	type=21; //credit void
	_snprintf(pin,sizeof(pin),"%8s%16.16i%20.20i"," ",99,98);
	_snprintf(buffer,len,"50.%42s%2.2i%8.8i %4.4i%c%c%s%c%i"," ",type,serial_num,trans_number,28,data_source,pin,28,tmp_amount*100);
	if(len>0)
		buffer[len-1]=0;
	return TRUE;

}
int create_status(char *buffer,int len)
{
	int status=0;
	char *msg="APPROVED";
	status=6; //approved/declined
	msg="DECLINED";
	switch(status){
	default:
	case 6:
		_snprintf(buffer,len,"11.%2.2i%s",status,msg);
		break;
	}
	return TRUE;

}
int process_command(HANDLE hcom,unsigned char *command,int len)
{
	char response[255]={0};
	int wlen,sent;
	int cmd=atoi(command);
	switch(cmd){
	case 7: //unit data
		printf("sending response\n");
		wlen=_snprintf(response,sizeof(response),"%c11.00%c*",2,3); //STX,ETX
		wlen=_snprintf(response,sizeof(response),"%c",6); //6=ACK 21=NAK
		WriteFile(hcom,response,wlen,&sent,NULL);
		break;
	case 13: //amount
		{
			char tmp[255]={0};
			create_authorization(tmp,sizeof(tmp),TRUE);
			wlen=_snprintf(response,sizeof(response),"%c%s%c%c",2,tmp,28,3); //STX,ETX
			printf("sending authorized %s\n",response);
			WriteFile(hcom,response,wlen,&sent,NULL);
		}
		break;
	case 11:
		{
			char tmp[255]={0};
			create_status(tmp,sizeof(tmp));
			wlen=_snprintf(response,sizeof(response),"%c%s%c",2,tmp,3); //STX,ETX
			printf("sending status authorized %s\n",response);
			WriteFile(hcom,response,wlen,&sent,NULL);
		}
		break;
	case 10: //reset
	default:
		printf("sending default response\n");
		wlen=_snprintf(response,sizeof(response),"%c",6); //6=ACK 21=NAK
		WriteFile(hcom,response,wlen,&sent,NULL);
	}
	return TRUE;
}

int handle_pinpad(HANDLE hcom,unsigned char *buf,int len)
{
	static char command[255]={0};
	static char entire[255]={0};
	static int state=0;
	static int c_index=0;
	static int e_index=0;
	int i;
	unsigned char a;
	for(i=0;i<len;i++){
		a=buf[i];
		switch(state){
		default:
		case 0:
			if(a==2){
				printf("found start\n");
				memset(command,0,sizeof(command));
				c_index=0;
				e_index=0;
				state=1;
			}
			break;
		case 1:
			if(a==3){
				printf("found end\ncommand=%s\n",command);
				process_command(hcom,command,c_index);
				state=0;
			}
			else{
				if(c_index<sizeof(command))
					command[c_index++]=a;
			}
			break;
		}
		if(e_index<sizeof(entire))
			entire[e_index++]=a;
		if(a=='*' && FALSE){
			if(e_index<sizeof(entire))
				entire[e_index++]=0;
			printf("entire=%s\n",entire);
			memset(entire,0,sizeof(entire));
			e_index=0;
		}

	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////
mode_terminal(HANDLE hcom,char portName[],int baud_rate)
{
	int i,j,length,key;
	char buffer[0x100],RXbuf;
	int hex=FALSE;
	int dtr_flag=TRUE;

	HANDLE hcon;
	CONSOLE_CURSOR_INFO con_cursor;
	COORD coord;
	SMALL_RECT rect;
	DWORD time1;

	printf("clearing RX buffer\n");
	PurgeComm(hcom,PURGE_RXCLEAR|PURGE_RXABORT);

	hcon = GetStdHandle(STD_OUTPUT_HANDLE);
	set_console_title(portName,baud_rate,echo,ansi_mode,hex);

	con_cursor.bVisible=TRUE;
	con_cursor.dwSize=100;
	SetConsoleCursorInfo(hcon,&con_cursor);

	hcon = GetStdHandle(STD_OUTPUT_HANDLE);
	coord.X=100;
	coord.Y=300;
	SetConsoleScreenBufferSize(hcon,coord);
	rect.Bottom=50-1;
	rect.Top=0;
	rect.Left=0;
	rect.Right=100-1;
	SetConsoleWindowInfo(hcon,TRUE,&rect);

	j=0;
	clrscr();
		

	do{

		Sleep(20);
		memset(buffer,0,0x100);
		length=0;
		ReadFile(hcom,buffer,0xFF,&length,NULL);

		if(length!=0)
		{
			if(ansi_mode!=0)
				parse_ansi(buffer,length);
			else
			{
				if(hex)
					dump_data(buffer,length);
				else
					printf("%.*s",length,buffer);
				//handle_pinpad(hcom,buffer,length);
				//for(i=0;i<length;i++)
				//	printf("%c",buffer[i]);
			}
		}

	
		if(kbhit())
		{
		key=getkey();


		if(extended_key)
		{
			//printf("key=%08X\n",key);
			switch(key)
			{	
				case 0x3B: //F1
					break;
				case 0x3C: //F2
					echo^=TRUE;
					set_console_title(portName,baud_rate,echo,ansi_mode,hex);
					break;
				case 0x3D: //F3
					dtr_flag=!dtr_flag;
					set_dtr(hcom,dtr_flag);
					break;
				case 0x3F: //f5
					printf("enter byte size (current=%i)\n",byte_size);
					scanf("%i",&byte_size);
					printf("byte size %i\n",byte_size);
					printf("setting com port\n");
					goto REOPEN;
					break;
				case 0x42: //f8
					hex=!hex;
					set_console_title(portName,baud_rate,echo,ansi_mode,hex);
					break;
				case 0x43: //f9
					clrscr();
					break;
				case 0x44: //f10
					ansi_mode^=TRUE;
					set_console_title(portName,baud_rate,echo,ansi_mode,hex);
					break;
				case 0x85: //F11
REOPEN:
					CloseHandle(hcom);
					hcom=connect_device(portName,baud_rate,byte_size);
					if(hcom==0)
					{
						printf("\nCant reopen port %s\n",portName);
						Sleep(250);
						return;
					}
					printf("\nport reopened\n");
					//RXbuf=VK_ESCAPE;
					//WriteFile(hcom,&RXbuf,1,&length,NULL);

					break;
				case 0x86: //F12
					{
						int baud=14400;
						printf("\nEnter new baud rate:");
						scanf("%i",&baud);
						if(set_baud(hcom,baud))
						{
							baud_rate=baud;
							set_console_title(portName,baud_rate,echo,ansi_mode,hex);
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
			WriteFile(hcom,&RXbuf,1,&length,NULL);
		}




		}
	}while(TRUE);

	con_cursor.bVisible=TRUE;
	con_cursor.dwSize=25;
	SetConsoleCursorInfo(hcon,&con_cursor);


}


int main(int argc, char **argv)
{
	char portName[255];
	int port=1,baud_rate=115200;
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
			else if(strstr(argv[i],"echo"))
			{
				echo=TRUE;
				printf("echo enabled\n");
			}
			else if(strstr(argv[i],"ansi"))
			{
				ansi_mode=TRUE;
				printf("ansi mode enabled\n");
			}
			else if(strstr(argv[i],"-bits="))
			{
				sscanf(argv[i],"-bits=%u",&byte_size);
				printf("byte size=%i\n",byte_size);
			}
			else 
			{
				printf("usage: [echo] [ansi] [-baud=#] [-bits=#]\n");
				if(strstr(argv[i],"-?")){
					exit(0);
				}
			}
		}
	}
retry:
	portName[0]=0;
	find_all_ports();
	printf("enter port selection:\n");
	fgets(portName,sizeof(portName),stdin);
	portName[sizeof(portName)-1]=0;
	if(sscanf(portName,"%i",&port)==1){
		sprintf(portName,"COM%i",port);
	}
	printf("opening port=%s\n",portName);
	hComm=connect_device(portName,baud_rate,byte_size);
	if(hComm==0)
	{
		goto retry;
	}

	mode_terminal(hComm,portName,baud_rate);
	CloseHandle(hComm);

}