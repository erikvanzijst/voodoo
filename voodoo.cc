/**********************************************************************************************
** Voodoo Chat
**
** voodoo.cc
** 19/05/1998
** laatste wijzigingen door : Erik van Zijst
**
** Compilen: moc -o voodoo.moc voodoo.cc ; gcc -lqt voodoo.cc -o voodoo
**
************************************************************************************************
** 
** Graphical chat program for the X window system.
** It is best described as a mixture of Winchat.exe for multiple users with IRC commands.
** Easy to use by avoiding the traditional client-server model. It does use TCP/IP, but since
** the program automatically and dynamically elects and maintains one server, there is no need
** for a dedicated server machine that is always reachable (like IRC).
** 
** Requirements:
** 
** Linux with a recent version of Qt (designed on 1.33, but 1.32 will work too) 
** Probably runs on other unices as well. Not tested though. Porting might be necessary.
**
************************************************************************************************
**
** Clientnr kan niet in de constructor gebruikt worden, deze moet dus 
** private gedeclareerd worden ipv als een functie.
**
** De acceptbox moet nog verder geimplementeerd worden
**
** De sendsound functie moet geluiden gaan versturen ipv ze bij zichzelf afspelen
**
***********************************************************************************************/

/* ----- Sound Includes ----- */
#include <limits.h>	/* PATH_MAX */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <malloc.h>
#include <termios.h>
/* ----- Config READ/WRITE Includes ----- */
#include <stdlib.h>
#include <stdio.h>
/* ----- GUI Includes ----- */
#include <qradiobt.h>
#include <qevent.h>
#include <qapp.h>
#include <qpushbt.h>
#include <qlined.h>
#include <qlistbox.h>
#include <string.h>
#include <qlabel.h>
#include <qmlined.h>
#include <qmsgbox.h>
#include <qgrpbox.h>
#include <qfont.h>
#include <qwindefs.h>
#include <qmenubar.h>
#include <qkeycode.h>
#include <qmsgbox.h>
#include <qtooltip.h>
#include <qbttngrp.h>
#include <qcombo.h>
#include <qchkbox.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qwindow.h>
#include <time.h>
/* ----- Network Includes ----- */
#include <sys/time.h>
#include <qsocknot.h>				// Troll's idea of select()
#include <fcntl.h>					// FileDescriptor manipulations
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <netdb.h>
/* ----- Sound Defines ----- */
#define FRAG_SPEC 0x00020007
#define NUM_SAMPLES 19
#define SAMPLE_RATE 11000
#define NUM_CHANNELS 16
/* ----- Defines ----- */
#define SA struct sockaddr
#define SERVER 0						// three
#define CLIENT 1						// operation
#define NONE -1						// modes
#define VOODOOPORT 3666				// need I say more?
#define MAXCONNS 1024				// maximum amount chatters
#define MAXLINE 1024
#define MAXDNSLENGTH 256
#define MAXSOCKADDR 128				// max socket address structure size
#define _UTS_NAMESIZE 16
#define _UTS_NODESIZE 256
#define h_addr h_addr_list[0]
#define ENGLISH 0
#define DUTCH 1
/* ----- Lots of Global Sound stuff ----- */
int chanel;
struct Sample
{
    unsigned char *data;	/* unsigned 8-bit raw samples */
    int len;			/* length of sample in bytes  */
};
typedef struct Sample Sample;
struct Mix
{
    unsigned char *Vclippedbuf;	
    int *Vunclipbuf;
    int Vsize;
};
typedef struct Mix Mix;
struct Channel
{
    unsigned char *Vstart,*Vcurrent;	/* ptr's into a playing sample */
    int	Vlen;				/* length of sample in bytes */
    int	Vleft;				/* bytes left of sample to play */
};
typedef struct Channel Channel;
static int sampleMixerStatus = 0;
static const Sample 	*S_sounds = NULL; /* ptr to array of samples */
static int S_num_sounds = 0;		/* size of 'sounds' array above */
static int S_fd_snddev = -1;		/* file # for sound device once open */
static int S_fd_pipe[2] = { -1, -1 };	/* pipe to talk to child process */
static int S_son_pid = -1;		/* process ID for the forked sound mixer */
static const char *S_snddev = NULL;	/* char string for device, ie "/dev/dsp" */
static int S_num_channels = 6;		/* number of channels to mix */
static int S_playback_freq = 0;		/* playback frequency (in Hz) */
/* ----- Lots of Global stuff ----- */
typedef enum visible {OFF,ON};
int id_connect;          			// id_nr of the menu items of chatwidget
int id_invite;							// id_nr of the menu items of chatwidget
int id_disconnect;					// id_nr of the menu items of chatwidget
int remoteport = VOODOOPORT;		// default VooDooChat TCP port
int clientsock;						// use this socket in clientmode
int listensock;						// bind to 666 and wait for incomings
int chatmode = NONE;					// no server, no client yet
int number_of_users;    
int clientlist[MAXCONNS];			// all socket fd's attached to clients are stored here
int numberofclients = 0;
int font_nr;
int user;								// Used by private widget
int language;
int server_language;
int temp_clientnr;
QString IPnr;							// if you're being invited, this is the IP of the server.
QString NewNick;						// holds the newly requested nick till server confirms its use
QString nick;			// user has a nick, even without voodoo.cfg
QString server_nick; 				// for client only
QString temp_clientnick;
QString remote_ip;						// changed!!!!!!!!!!!!!!!!!
QString invite_nick;					// de inviter geeft deze nick mee
QSocketNotifier *ClientModeNotifier;	// notifier on socket for communication with server
QSocketNotifier *notifylist[MAXCONNS];	// notifiers on sockets for communication with clients
QPopupMenu *connection;  //made global so chatwidget, acceptinvite and connectwidget can use it
QLineEdit *edit_user_nick;
QLineEdit *edit_user_dns;
QLineEdit *connect_line_edit;
QLineEdit *chat_line_edit;
QListBox *connect_list_box;
QListBox *nick_list_box;
QCheckBox *accept_box[MAXCONNS +1];
QCheckBox *splash_screen;
QMultiLineEdit *chat_multi_line_edit;
QLabel *splash;
visible connect_window; 			//only one window can be opened
visible accept_window;
bool sound_on;
bool private_on;
bool motif_on; //motif style or windows
bool debug_on;
bool english_on;
bool window_on[MAXCONNS+1];		// TRUE means, a privatechat window is on
bool accept_sound[MAXCONNS+1];
bool splash_on;
struct sockaddr_in remoteserver;	// info on remote server
struct sockaddr_in localserver;	// info on this machine
struct userInfo 						//only used by address book
{
   char alias[48];
   char ip[128];
} user_info[999] ;
struct userdatabase					// for servermode only
{
   QString nickname;
   int validated;						// without validation, the user cannot speak
   int language;
} User[MAXCONNS];
/* ----- Global Sound funtions ----- */
int Snd_loadRawSample( const char *file, Sample *sample );
int Snd_init( int num_snd, const Sample *sa, int freq, 
          int channels, const char *sound_device );
int Snd_restore();
int Snd_effect( int nr, int channel );
void InitSound();

int Snd_init_dev();
int Snd_restore_dev();
void Chan_reset( Channel *chan );	/* init channel structure */
void Chan_assign( Channel *chan, const Sample *snd );
int  Chan_mixAll( Mix *mix, Channel *ch );
int  Chan_copyIn( Channel *chan, Mix *mix );
int  Chan_mixIn( Channel *chan, Mix *mix );
int  Chan_finalMixIn( Channel *chan, Mix *mix );
void Mix_alloc( Mix *mix, int size );
void Mix_dealloc( Mix *mix );
void play_sound(int sound_nr);
/* ----- Global funtions ----- */
void InitUser(void);
void InitNotifyList(void);
void InitClientList(void);
void setnonblocking(int sockfd);						// socket is now hopefully nonblocking
void send2all(QString output, int listnum);	
void sendlanguage2all (QString e_string, QString d_string, int listnum);
void read_config(void);
void write_config(void);
void put_text(QString message);						// our version of printf()
void nickevent(QString nickname, int listnum);	// user listnum wants nickname as new nick
void chservernick(QString newnick);
void whois(QString nickname, int listnum);		// send user info on nickname to listnum
void invite(void);
int find_user(QString nickname);						// returns listnum
void message2window(QString input,QString nickname); 
void create_private_window(QString nickname);
void senduserlist2all(void);
void update_user_list(QString userlist);
void sound(QString nickname, int listnum, int soundnr);
void receive_sound(QString nickname, int soundnr);

/* ----- Declaration of Classes ----- */
class OptionWidget : public QDialog 
{
   Q_OBJECT
private:
   QLineEdit *option_line_edit;
   bool nick_changed;
   QRadioButton *snd_on;
   QRadioButton *snd_off;
   QRadioButton *english;
   QRadioButton *dutch;
public:
   OptionWidget ( QWidget *parent=0, const char *name=0 );
   void closeEvent(QCloseEvent *e);
private slots:
   void option_close();
   void change_nick();
   void option_nick_changed(const char *new_nick);
};

class LayoutWidget : public QDialog 
{
   Q_OBJECT
private:
   QComboBox *layout_combo;
   QRadioButton *motif;
   QRadioButton *windows;
   QRadioButton *priv_on;
   QRadioButton *priv_off;
public:
   LayoutWidget ( QWidget *parent=0, const char *name=0 );
   void closeEvent(QCloseEvent *e);
private slots:
   void layout_close();
};

class AddWidget : public QDialog
{
   Q_OBJECT
private:
   QLineEdit *add_user_nick;
   QLineEdit *add_user_dns;
 	QPushButton *ok_button;
public:
   AddWidget ( QWidget *parent=0, const char *name=0 );
   void closeEvent(QCloseEvent *e);
public slots:
   void nick_return_pressed();
   void dns_return_pressed();
   void add_close();
   void insert_user();
};

class EditWidget : public QDialog
{
   Q_OBJECT
private:
   QLineEdit *edit_user_nick;
   QLineEdit *edit_user_dns;
 	QPushButton *ok_button;
public:
   EditWidget ( QWidget *parent=0, const char *name=0);
   void closeEvent(QCloseEvent *e);
public slots:
   void nick_return_pressed();
   void dns_return_pressed();
   void edit_close();
   void update_user();
};


class AcceptInvite : public QDialog
{
   Q_OBJECT
public:
   AcceptInvite(QWidget *parent = 0, const char *name = 0);
signals:
   void chat_mode_changed(); 
public slots:
   void accept_invite();
   void reject_invite();
   void change_popup_menu();
};

class ConnectWidget : public QWidget
{	
   Q_OBJECT
private:
	QPushButton *connect_button;
public:
	ConnectWidget ( QWidget *parent=0, const char *name=0 );
   void closeEvent(QCloseEvent *e);
signals:
   void chat_mode_changed(); 
public slots:
   void connect_close();
   void connect2host();
   void update_line_edit();
   void add_user();
   void delete_user();
   void edit_user();
   void receive_data();
   void connect_available(const char *);
   void change_popup_menu();
   void change_remote_ip();
};

ConnectWidget *connectwidget;		// There's only ONE connectwidget. It's global.

class ChatWidget : public QWidget
{
   Q_OBJECT
private:
   QPushButton *options_button;
   QPushButton *help_button;
   QGroupBox *user_names;
public:
   ChatWidget (QWidget *parent=0, const char *name=0);
   void closeEvent(QCloseEvent *e);
   void resizeEvent(QResizeEvent *e);
signals:
   void chat_mode_changed(); 
public slots:
   void parser();
   void make_connection();
   void options();
   void layout();
   void invitewindow();
   void handle_inc();	// accept incoming clients
   void deal_with_data();	// receive data as a server (from clients)
   void disconnect();
   void change_popup_menu();
   void exit_program();
   void help();
   void about();
   void aboutQt();
   void nick_double_clicked(const char *);  //opent een window als er op een nick gedubbel clicked word
   void remove_splash();
};   

ChatWidget *chatwidget;

class PrivateWidget : public QWidget
{
   Q_OBJECT
private:
   QMultiLineEdit *multi_line;
   QLineEdit *line_edit;
   QComboBox *sound_combo;
   QPushButton *send_button;
   int clientnr; 
   QString clientnick;
public:
   PrivateWidget (QWidget *parent=0, const char *name=0);
   void closeEvent(QCloseEvent *e);
   void resizeEvent(QResizeEvent *e);
   void update_multi_line(QString input, QString nickname);
   void put_private_text(QString message);
   void set_client_info(int listnum,QString nickname);
private slots:
   void private_parser();
   void send_sound();
};   

PrivateWidget *privatewidget[MAXCONNS+1]; //last one for the server

/*class SplashWidget : public QLabel
{
private:
   QPixmap pic;
public:
   SplashWidget (QFrame *parent=0, const char *name=0);
   void paintEvent(QPaintEvent *e);   
};
*/
/* ----- End of declaration of classes ----- */

void InitClientList(void)
{  for(int nX=0; nX<MAXCONNS; nX++)
      clientlist[nX] = 0;
}

void InitNotifyList(void)
{  for(int nX=0; nX<MAXCONNS; nX++)
      notifylist[nX] = NULL;
}

void InitUser(void)
{  
   for(int nX=0; nX<MAXCONNS; nX++)
      User[nX].validated = 0;
}

void setnonblocking(int sockfd)	
{  int flags;
   if( (flags = fcntl(sockfd, F_GETFL)) < 0) exit(0);	// I agree, brutal way of error handling
   flags = (flags | O_NONBLOCK);
   if( (fcntl(sockfd, F_SETFL, flags)) < 0) exit(0);
   return;
}

void send2all(QString sendstr, int listnum)
{  
//   put_text("onzin");
   int nX;
   sendstr.append("\r\n");  // maken the client to empty the socket
//   QString sendstr = output;	// is this allowed??
   for(nX=0; nX<MAXCONNS; nX++)
   {  if((clientlist[nX] != 0)&&(nX != listnum)&&(User[nX].validated))	// trigger on connected sockets
      {  write(clientlist[nX], sendstr, sendstr.length());
         put_text("INTERNAL: Incoming text forwarded to a client.");
      }
   }
}

void sendlanguage2all (QString e_string, QString d_string, int listnum)
{  
//   put_text("onzin");
   int nX;
   e_string.append("\r\n");  // maken the client to empty the socket
   d_string.append("\r\n");  // maken the client to empty the socket
//   QString sendstr = output;	// is this allowed??
   for(nX=0; nX<MAXCONNS; nX++)
   {  if((clientlist[nX] != 0)&&(nX != listnum)&&(User[nX].validated))	// trigger on connected sockets
      {  
         if (User[nX].language==ENGLISH) write(clientlist[nX], e_string, e_string.length());
         else write(clientlist[nX], d_string, d_string.length());
//         put_text("INTERNAL: Incoming text forwarded to a client.");
      }
   }
}


void read_config(void)
{
   FILE *config_file;
   int index,options,temp1;
   char temp,temp_nick[12],temp_options[4],temp_font[4],temp_splash[2];

   QString config_data = getenv("HOME");
   config_data.append("/.voodoo.cfg");
  
   config_file=fopen(config_data,"r");
   if( (config_file = fopen(config_data, "r")) == NULL) return;
   
   index=0;
   while ( (temp=fgetc(config_file)) != '\n' )
      temp_nick[index++]=temp;
   temp_nick[index]='\0';
   nick=temp_nick;
   number_of_users=0;
   index=0;

   while ( (temp=fgetc(config_file)) != '\n' )
      temp_options[index++]=temp;
   temp_options[index]='\0';
   options=atoi(temp_options);
   index=0;

   while ( (temp=fgetc(config_file)) != '\n' )
      temp_font[index++]=temp;
   temp_font[index]='\0';
   font_nr=atoi(temp_font);
   index=0;

   while ( (temp=fgetc(config_file)) != '\n' )
      temp1=temp;
   index=0;

   while (!feof(config_file))
   {
      temp=fgetc(config_file);
      
      if ( temp == ',' )
      {
        	user_info[number_of_users].alias[index]='\0';
      	index=0;
         while ( (temp=fgetc(config_file)) != '\n' )
         	user_info[number_of_users].ip[index++]=temp;
        	user_info[number_of_users].ip[index]='\0';
         number_of_users++;
      	index=0;
      }  
      else
        	user_info[number_of_users].alias[index++]=temp;
   }


   sound_on=FALSE;
   english_on=FALSE;
   language=DUTCH;
   private_on=FALSE;
   motif_on=FALSE;
   splash_on=FALSE;
   if (options & 1) sound_on=TRUE;
   if (options & 2)
   { 
     language=ENGLISH ; 
     english_on=TRUE;
   }
   if (options & 4)
   {
      motif_on=TRUE;
      QApplication::setStyle(GUIStyle(MotifStyle));
   }
   if (options & 8) private_on=TRUE;

   QFont voodoofont;
   switch (font_nr)
   {
      case 0: voodoofont = QFont("helvetica"); break;
      case 1: voodoofont = QFont("times"); break;
      case 2: voodoofont = QFont("courier"); break;
      case 3: voodoofont = QFont("lucida"); break;
   }   
   QApplication::setFont(voodoofont );
   if (temp1=='1') splash_on=TRUE;
   
   fclose(config_file);   
}

void write_config(void)
{
   FILE *config_file;
   int index1,index2,options;
   char temp,temp_nick[12],temp_options[4],temp_font[4];
   
   QString config_data = getenv("HOME");
   config_data.append("/.voodoo.cfg");
  
   config_file=fopen(config_data,"w");

   strcpy(temp_nick,nick);   

   
   for (index1=0;index1 < strlen(temp_nick); ++index1)
      fputc(temp_nick[index1],config_file);
   fputc('\n',config_file);

   options=0;
   if (sound_on==TRUE) options=options+1;
   if (english_on==TRUE) options=options+2;
   if (motif_on==TRUE) options=options+4;
   if (private_on==TRUE) options=options+8;

   if (options>=10)
      temp_options[0]='1';   
   else
      temp_options[0]='0';   
   
   if (options==0) temp_options[1]='0';
   if (options==1) temp_options[1]='1';
   if (options==2) temp_options[1]='2';
   if (options==3) temp_options[1]='3';
   if (options==4) temp_options[1]='4';
   if (options==5) temp_options[1]='5';
   if (options==6) temp_options[1]='6';
   if (options==7) temp_options[1]='7';
   if (options==8) temp_options[1]='8';
   if (options==9) temp_options[1]='9';
   if (options==10) temp_options[1]='0';
   if (options==11) temp_options[1]='1';
   if (options==12) temp_options[1]='2';
   if (options==13) temp_options[1]='3';
   if (options==14) temp_options[1]='4';
   if (options==15) temp_options[1]='5';
   temp_options[2]='\0';   

   temp_font[0]='0';
   switch (font_nr)
   {
      case 0:   temp_font[0]='0'; break;   
      case 1:   temp_font[0]='1'; break;   
      case 2:   temp_font[0]='2'; break;   
      case 3:   temp_font[0]='3'; break;   
      case 4:   temp_font[0]='4'; break;   
      case 5:   temp_font[0]='5'; break;   
      case 6:   temp_font[0]='6'; break;   
      case 7:   temp_font[0]='7'; break;   
      case 8:   temp_font[0]='8'; break;   
      case 9:   temp_font[0]='9'; break;   
   }
   temp_font[1]='\0';
   
   for (index1=0;index1 < strlen(temp_options); ++index1)
      fputc(temp_options[index1],config_file);
   fputc('\n',config_file);

   for (index1=0;index1 < strlen(temp_font); ++index1)
      fputc(temp_font[index1],config_file);
   fputc('\n',config_file);

   if (splash_on==TRUE) fputc('1',config_file);
   else fputc('0',config_file);
   fputc('\n',config_file);

   for (index2=0; index2 < number_of_users ; ++index2)
   {
      for (index1=0; index1 < strlen(user_info[index2].alias) ; ++index1)
         fputc(user_info[index2].alias[index1],config_file);
      fputc(',',config_file);
      for (index1=0; index1 < strlen(user_info[index2].ip) ; ++index1)
         fputc(user_info[index2].ip[index1],config_file);
      fputc('\n',config_file);
   }
   fputc('\n',config_file);

   fclose(config_file);   
}

void put_text(QString message)
{  int doorgaan = 1;
   char buffer[MAXLINE+1];
   if (!(message.find("INTERNAL",0,TRUE)==0 && debug_on==FALSE ))
   {   strcpy(buffer, message);
       while(doorgaan == 1)
       {
         if((buffer[strlen(buffer)-1] == '\r')||(buffer[strlen(buffer)-1] == '\n'))
         {  put_text("INTERNAL: Last character is cr or nl. Deleted...");
            buffer[strlen(buffer)-1] = '\0';
         }
         else doorgaan=0;
       }
       chat_multi_line_edit->append(buffer);
       chat_multi_line_edit->setCursorPosition(chat_multi_line_edit->numLines(),0,FALSE);
   }
}

void chservernick(QString newnick)	// change your nick in servermode
{
   newnick = newnick.simplifyWhiteSpace();
   newnick = newnick.stripWhiteSpace();
   put_text("INTERNAL: Changing your nickname.");
   for(int nX=0; nX<MAXCONNS; nX++)
   {
      if(clientlist[nX] != 0)
      {  put_text("INTERNAL: Checking a connected client.");
      	if(User[nX].nickname.lower() == newnick.lower() || nick.lower()==newnick.lower())
        {
        	put_text("-=] Nickname already in use. [=-");
            put_text("INTERNAL: Requested nick not available.");
            return;		// don't even bother, return immediately
        }
      }
   }
   put_text("INTERNAL: New nickname is available.");
   QString e_output = "-=] " +nick.copy();
   QString d_output = "-=] " +nick.copy();
   e_output.append(" is now known as ");
   d_output.append(" heet nu ");
   e_output.append(newnick + ". [=-");
   d_output.append(newnick + ". [=-");
   sendlanguage2all(e_output,d_output, -1);
   put_text("INTERNAL: All clients are notified of nickchange.");
   QString output;
   if (language==ENGLISH) output = "-=] You are now known as ";
   if (language==DUTCH) output = "-=] Je alias is verandert in ";
   output.append(newnick+". [=-");
   put_text(output);
   nick = newnick.copy();	// eventually change your nick
   senduserlist2all();
}

void nickevent(QString nickname2, int listnum)
{
   put_text("INTERNAL: Is the new nick available?");
   for(int nX=0; nX<MAXCONNS; nX++)
   {  if(clientlist[nX] != 0)	// check connected sockets only
      {  if((nickname2.lower() == User[nX].nickname.lower()) || (nickname2.lower() == nick.lower())) //.lower()          
         {  QString msg;
            if (User[listnum].language==ENGLISH) msg="-=] Nickname already in use. [=- ";
            if (User[listnum].language==DUTCH) msg="-=] Alias bestaat al. [=- ";
            msg.append("\r\n");
            write(clientlist[listnum], msg,msg.length());
            put_text("INTERNAL: User requested a nick that is already in use.");
            return;
         }
      }
   }
   put_text("INTERNAL: Nickchange allowed.");
   if(User[listnum].validated == 1)
   {  
      QString e_output = "-=] " + User[listnum].nickname.copy();
      QString d_output = "-=] " + User[listnum].nickname.copy();
      e_output.append(" is now known as ");
      d_output.append(" heet nu ");
      e_output.append(nickname2+ ". [=-");
      d_output.append(nickname2+ ". [=-");
      if (language==ENGLISH) put_text(e_output);
      if (language==DUTCH) put_text(d_output);
      sendlanguage2all(e_output,d_output, listnum);
      User[listnum].nickname = nickname2;
      senduserlist2all();
      QString output;
      output = "/nick ";
      output.append("\r\n");
      write(clientlist[listnum], output, sizeof(output));
   }
   else
   {  User[listnum].nickname = nickname2.copy();
//      QString output = User[listnum].nickname;
//   	senduserlist2all();
      QString output="/nick ";
      output.append("\r\n");
      write(clientlist[listnum], output, sizeof(output));	// only send "/nick \r\n".
      User[listnum].validated = 1;
      QString e_output = "-=] " + User[listnum].nickname.copy();
      QString d_output = "-=] " + User[listnum].nickname.copy();
      e_output.append(" has joined the partyline. [=-");
      d_output.append(" is er bij gekomen. [=-");
      if (language==ENGLISH) put_text(e_output);				// show this to the server-user
      else put_text(d_output);				// show this to the server-user
      sendlanguage2all(e_output,d_output, listnum);	// and all third-party chatters
      senduserlist2all();
   }
}

void whois(QString nickname, int listnum)
{
   struct sockaddr_in peer;
   struct sockaddr_in sin;
   struct hostent *lhinfo;
   struct utsname myname;
   int len = sizeof(peer);
   QString output;
   int nX, found = 0;
   char *hostname;

   put_text("INTERNAL: whois-procedure called.");

   if(nick.lower() == nickname.lower())	// if info on server is requested
   {
      if(uname(&myname) < 0) return;
      if( (lhinfo = gethostbyname(myname.nodename)) == NULL) return;
      bzero((char *)&sin, sizeof(sin));
      bcopy(lhinfo->h_addr, (char *)&sin.sin_addr, lhinfo->h_length);

   		//      if (gethostname(hostname, size) == -1) retu
 
	if (listnum==-1 && language==ENGLISH)
	{
      	output = " ** whois output\n ";
        output.append(nickname);
        output.append(" is connected from:\n IP:  ");
        output.append( inet_ntoa(sin.sin_addr) );	// insert the IP number
        output.append("\n DNS: ");
        lhinfo = gethostbyaddr((char *)&sin.sin_addr, 4, AF_INET);
        if(lhinfo == NULL) return;
        output.append(lhinfo->h_name);
        output.append("\n ** End of whois\r\n");
      }
	if (listnum==-1 && language==DUTCH)
	{
      	output = " ** whois gegevens\n ";
        output.append(nickname);
        output.append(" is verbonden vanaf:\n IP:  ");
        output.append( inet_ntoa(sin.sin_addr) );	// insert the IP number
        output.append("\n DNS: ");
        lhinfo = gethostbyaddr((char *)&sin.sin_addr, 4, AF_INET);
        if(lhinfo == NULL) return;
        output.append(lhinfo->h_name);
        output.append("\n ** Einde van whois\r\n");
      }

     if( (listnum > -1 && User[listnum].language==ENGLISH)) // English client or server
      {
      	output = " ** whois output\n ";
        output.append(nickname);
        output.append(" is connected from:\n IP:  ");
        output.append( inet_ntoa(sin.sin_addr) );	// insert the IP number
        output.append("\n DNS: ");
        lhinfo = gethostbyaddr((char *)&sin.sin_addr, 4, AF_INET);
        if(lhinfo == NULL) return;
        output.append(lhinfo->h_name);
        output.append("\n ** End of whois\r\n");
      }
      if( (listnum > -1 && User[listnum].language==DUTCH) ) // Dutch client or server
      {
      	output = " ** whois gegevens\n ";
        output.append(nickname);
        output.append(" is verbonden vanaf:\n IP:  ");
        output.append( inet_ntoa(sin.sin_addr) );	// insert the IP number
        output.append("\n DNS: ");
        lhinfo = gethostbyaddr((char *)&sin.sin_addr, 4, AF_INET);
        if(lhinfo == NULL) return;
        output.append(lhinfo->h_name);
        output.append("\n ** Einde van whois\r\n");
      }
      if(listnum > -1)		// if the request came from the outside
         write(clientlist[listnum], output, strlen(output));
      else	// if the request came from the inside (local user)
         put_text(output);
      return;
   }

   for(nX=0; nX<MAXCONNS; nX++)
   {
      if((clientlist[nX] != 0)&&(User[nX].nickname.lower() == nickname.lower())) // if an fd is connected      
      {
      	found = 1;
        put_text("INTERNAL: Specified user found on an fd.");
        break;
      }
   }
   if(!found)
   {
      put_text("INTERNAL: Specified user doesn't exist.");
      
	  if (listnum==-1 && language==ENGLISH)
	  {
      	output = " ** whois output\n ";
        output.append(nickname);
        output.append(": no such nickname.\n ** End of whois\r\n");
      }
	  if (listnum==-1 && language==DUTCH)
	  {
      	output = " ** whois gegevens\n ";
        output.append(nickname);
        output.append(": gebruiker bestaat niet.\n ** End of whois\r\n");
      }


      if( (listnum > -1 && User[listnum].language==ENGLISH)) // Dutch client or server
      {
      	output = " ** whois output\n ";
        output.append(nickname);
        output.append(": no such nickname.\n ** End of whois\r\n");
      }
      if( (listnum > -1 && User[listnum].language==DUTCH) ) // Dutch client or server
      {
      	output = " ** whois gegevens\n ";
        output.append(nickname);
        output.append(": geen geldige alias.\n ** Einde van whois\r\n");
      }
      if(listnum > -1)		// if the request came from the outside
       write(clientlist[listnum], output, strlen(output));
      else	// if the request came from the inside (local user)
         put_text(output);
      return;
   }

   getpeername(clientlist[nX], (SA *)&peer, &len);
   len = sizeof(*lhinfo);
   //lhinfo = gethostbyname(inet_ntoa(peer.sin_addr));
	if (listnum==-1 && language==ENGLISH)  //for tha server
	{
      output = " ** whois output\n ";
      output.append(nickname);
      output.append(" is connected from:\n IP:  ");
      output.append(inet_ntoa(peer.sin_addr));	// insert the IP number
      output.append("\n DNS: ");
      lhinfo = gethostbyaddr((char *)&peer.sin_addr, 4, AF_INET);
      if(lhinfo == NULL) output.append("does not exist." );//return;
      else output.append(lhinfo->h_name);
      output.append("\n ** End of whois\r\n");
      }
	if (listnum==-1 && language==DUTCH)
	{
      output = " ** whois gegevens\n ";
      output.append(nickname);
      output.append(" is verbonden vanaf:\n IP:  ");
      output.append(inet_ntoa(peer.sin_addr));	// insert the IP number
      output.append("\n DNS: ");
      lhinfo = gethostbyaddr((char *)&peer.sin_addr, 4, AF_INET);
      if(lhinfo == NULL) output.append("bestaat niet." );//return;
      else output.append(lhinfo->h_name);
      output.append("\n ** Eind van whois\r\n");
      }

   if( (listnum > -1 && User[listnum].language==ENGLISH)) // English client 
   {
      output = " ** whois output\n ";
      output.append(nickname);
      output.append(" is connected from:\n IP:  ");
      output.append(inet_ntoa(peer.sin_addr));	// insert the IP number
      output.append("\n DNS: ");
      lhinfo = gethostbyaddr((char *)&peer.sin_addr, 4, AF_INET);
      if(lhinfo == NULL) output.append("does not exist." );//return;
      else output.append(lhinfo->h_name);
      output.append("\n ** End of whois\r\n");
   }
   if( (listnum > -1 && User[listnum].language==DUTCH)) // English client or server
   {

      output = " ** whois gegevens\n ";
      output.append(nickname);
      output.append(" is verbonden vanaf:\n IP:  ");
      output.append(inet_ntoa(peer.sin_addr));	// insert the IP number
      output.append("\n DNS: ");
      lhinfo = gethostbyaddr((char *)&peer.sin_addr, 4, AF_INET);
      if(lhinfo == NULL) output.append("bestaat niet." );//return;
      else output.append(lhinfo->h_name);
      output.append("\n ** Eind van whois\r\n");
   }
   if(listnum > -1)		// if the request came from the outside
      write(clientlist[listnum], output, strlen(output));
   else	// if the request came from the inside (local user)
      put_text(output);
   
   output = "INTERNAL: Specified user comes from: ";
   output.append(inet_ntoa(peer.sin_addr));
   put_text(output);
}

void invite(void)
{
   int invitesock;
   struct sockaddr_in remoteserver2;	// info on machine to invite
   struct sockaddr_in server;
   int len = sizeof(server);
   struct hostent *hptr;
   struct in_addr saddr;
   struct sockaddr_in sin;

//          if( (hptr = gethostbyname(remote_ip)) == NULL) return;
 //         if( inet_aton( inet_ntoa(sin.sin_addr), &remoteserver.sin_addr) == 0) return;


   if( (invitesock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
   bzero(&remoteserver2, sizeof(remoteserver2));
   remoteserver2.sin_family = AF_INET;
   remoteserver2.sin_port = htons(remoteport);
   
   if( inet_aton(remote_ip, &remoteserver2.sin_addr) == 0)
   {
      if ( (hptr = gethostbyname(remote_ip)) == NULL) return;
      bzero((char *)&sin, sizeof(sin));
      bcopy(hptr->h_addr, (char *)&sin.sin_addr, hptr->h_length);
      if( (inet_aton( inet_ntoa(sin.sin_addr), &remoteserver2.sin_addr)) == 0) return;
   }

//   if( inet_aton( (user_info[connect_list_box->currentItem()].ip), &remoteserver2.sin_addr) <= 0)
   if( (::connect(invitesock, (SA *)&remoteserver2, sizeof(remoteserver2))) < 0)
   {  put_text("INTERNAL: Error in ::connect() call."); return; }
   setnonblocking(invitesock);
   QString invitestr = "/invite ";
   
   if(chatmode == SERVER)		// send your own IP
   {  struct hostent *lhinfo;
      struct sockaddr_in sin;
      struct utsname myname;
      if(uname(&myname) < 0) return;
      if( (lhinfo = gethostbyname(myname.nodename)) == NULL) return;
      bzero((char *)&sin, sizeof(sin));
      bcopy(lhinfo->h_addr, (char *)&sin.sin_addr, lhinfo->h_length);
      invitestr.append(inet_ntoa(sin.sin_addr));	// insert my own IP
      QString crap = "INTERNAL: Sending the server IP: ";
      crap.append(inet_ntoa(sin.sin_addr));
      put_text(crap);
      invitestr.append(" " +nick+"\r\n");
      write(invitesock, invitestr, strlen(invitestr));
   }
   if(chatmode == CLIENT)
   {
      getpeername(clientsock, (SA *)&server, &len);
      invitestr.append(inet_ntoa(server.sin_addr));
      QString crap = "INTERNAL: Sending the server IP: ";
      crap.append(inet_ntoa(server.sin_addr));
      put_text(crap);
      invitestr.append(" " + nick + "\r\n");
      write(invitesock, invitestr, strlen(invitestr));
   }
   if (language==ENGLISH) put_text("-=] User has been invited. [=-");
   if (language==DUTCH) put_text("-=] Gebruiker is uitgenodigd. [=-");
   ::close(invitesock);
   return;
}

void senduserlist2all(void)
{  
/*
   Alleen uitgevoerd door de server
   De vorm is /userlist:serverbick,aantalclients,nick,listnum,validated,language
*/
   int nX;
   QString temp;
   put_text("INTERNAL: Sending userlist to all validated clients.");
   QString sendstr="/userlist:";
   sendstr.append(nick);
   sendstr.append(",");
   temp.setNum(numberofclients);
   sendstr.append(temp);
   sendstr.append(",");
   for(nX=0; nX<MAXCONNS; nX++)
   {  
      if(clientlist[nX] != 0)	// trigger on connected sockets
      {
         sendstr.append(User[nX].nickname);
         sendstr.append(",");
         temp.setNum(nX);
         sendstr.append(temp);
         sendstr.append(",");
         temp.setNum(User[nX].validated);
         sendstr.append(temp);
         sendstr.append(",");
         temp.setNum(User[nX].language);
         sendstr.append(temp);
         sendstr.append(",");
      }
   }
   
   sendstr=sendstr.left( sendstr.length()-1 );   // haalt die laatse komma weg
   sendstr.append("\r\n");  // maken the client to empty the socket
//   send2all(sendstr,-1);

 
//   QString sendstr = output;	// is this allowed??
   for(nX=0; nX<MAXCONNS; nX++)
   {  if(clientlist[nX] != 0&&User[nX].validated)	// trigger on connected sockets
      {  write(clientlist[nX], sendstr, sendstr.length());
         put_text("INTERNAL: Incoming text forwarded to a client.");
      }
   }
   update_user_list(sendstr);   
}

void update_user_list(QString userlist)
{
   // aangeroepen als de server de userlist verzonden heeft
   // de server runt deze functie ook

   int temp,count,index,temp_listnum,temp_validated,temp_language;
   bool ok;
   
//   put_text(userlist);
   QString list_nick,user_nr,temp_string;
   userlist=userlist.right(userlist.length()-10);
/* lijst wissen */
   count=nick_list_box->count();
   for (index=0;index<count;++index)
      nick_list_box->removeItem(0);
/* server toevoegen aan de lijst en aantal clients inlezen */
   temp = userlist.find(",",0,FALSE);
   list_nick = userlist.left(temp);
   userlist=userlist.right(userlist.length()-(temp+1) );
   server_nick=list_nick.copy();
   temp = userlist.find(",",0,FALSE);
   temp_string = userlist.left(temp);
   userlist=userlist.right(userlist.length()-(temp+1) );
   numberofclients=temp_string.toInt(&ok);

//   put_text(userlist);
/* de volgende regels vorkomt dat de server toch zijn nick in zijn lijst krijgt
   als hij zijn nick verandert
*/
   if (chatmode==SERVER)
   {
      for (index=0;index<MAXCONNS;++index)
         if (User[index].validated==1)
         {
         	nick_list_box->insertItem(list_nick,-1);   
            break;
         }
   }
   else
      nick_list_box->insertItem(list_nick,-1);   
/* users in de database en listbox zetten */
   if(chatmode == CLIENT)
      for (index=0; index<MAXCONNS; ++index) User[index].validated=0; //userdbase leegmaken
   
   for (index=0;index<numberofclients;++index)
   {
      temp = userlist.find(",",0,FALSE);			// nick bepalen
      list_nick = userlist.left(temp);
      userlist=userlist.right(userlist.length()-(temp+1) );

//      put_text(list_nick);
      temp = userlist.find(",",0,FALSE);			// listnum bepalen
      user_nr = userlist.left(temp);
      userlist=userlist.right(userlist.length()-(temp+1) );
      temp_listnum=user_nr.toInt(&ok);
      
      temp = userlist.find(",",0,FALSE);			// validated bepalen
      temp_string = userlist.left(temp);
      userlist=userlist.right(userlist.length()-(temp+1) );
      temp_validated=temp_string.toInt(&ok);
      if (temp_validated==1)
         nick_list_box->insertItem(list_nick,-1);
//      else
//         nick_list_box->insertItem("[Connecting]",-1);
         
      temp = userlist.find(",",0,FALSE);			// language bepalen
      temp_string = userlist.left(temp);
      userlist=userlist.right(userlist.length()-(temp+1) );
      temp_language=temp_string.toInt(&ok);

      if(chatmode == CLIENT && list_nick!=nick)
      {
      	 User[temp_listnum].nickname=list_nick;
         User[temp_listnum].validated=temp_validated;
         User[temp_listnum].validated=temp_language;
//         put_text(user_nr);
      }
   
   }
/* In het geval dat er nicks veranderd zijn waar jij mee zat te prv_chatten, zal met
   de onderstaande regels de caption naar de goede nick veranderd worden
*/

   for (index=0;index<MAXCONNS;++index)
      if (window_on[index]==TRUE)	//client windows
      {   
          if (language==ENGLISH) privatewidget[index]->setCaption("Private [" + User[index].nickname + "]");
          if (language==DUTCH) privatewidget[index]->setCaption("Persoonlijk [" + User[index].nickname + "]");
          privatewidget[index]->set_client_info(index,User[index].nickname);
      }

   if (window_on[MAXCONNS]==TRUE)	//server window
   {
      if (language==ENGLISH) privatewidget[MAXCONNS]->setCaption("Private [" + server_nick + "]");
      if (language==DUTCH) privatewidget[MAXCONNS]->setCaption("Persoonlijk [" + server_nick + "]");
      privatewidget[MAXCONNS]->set_client_info(MAXCONNS,server_nick);
   }
   
   
}

int find_user(QString nickname)
{
   int index;
   bool found=FALSE;

   if (chatmode==SERVER)
   {
      for(index=0; index<MAXCONNS; ++index)
      {
      	if((clientlist[index] != 0)&&(User[index].nickname.lower() == nickname.lower()))	
         {
         	found = TRUE;
            put_text("INTERNAL: Specified user found on an fd.");
            break;
         }
      }
   }
   if (chatmode==CLIENT)
   {
      for(index=0; index<MAXCONNS; ++index)
      {
      	if(User[index].nickname.lower() == nickname.lower())
         {
         	found = TRUE;
            put_text("INTERNAL: Specified user found on an fd.");
            break;
         }
      }
      if (server_nick.lower() == nickname.lower()) return MAXCONNS;
   }
   
   if (found==TRUE)	return index;
   else return -1;
}

void message2window(QString input, QString nickname) 
{
   int listnum;
   listnum=find_user(nickname);
   if (listnum!= -1)
   {
      if (window_on[listnum]==FALSE)	// window bestaat al en alleen de text moet in de window 
      {
         create_private_window(nickname);
      }
      if (input.length()>0) privatewidget[listnum]->update_multi_line(input, nickname);
   }
   
/*   put_text("*\n");
   put_text(input);
   put_text(nickname);
   put_text("*\n");
*/   
}

void create_private_window(QString nickname)
{
   put_text("INTERNAL: create_private_window()");
   int listnum;
   listnum=find_user(nickname);
   if (listnum!= -1&& window_on[listnum]==FALSE)
   {  
      temp_clientnr=listnum;
      temp_clientnick=nickname.copy();
      privatewidget[listnum] = new PrivateWidget(0, "Private Dialog");
      if (language==ENGLISH) privatewidget[listnum]->setCaption("Private [" + nickname + "]");
      if (language==DUTCH) privatewidget[listnum]->setCaption("Persoonlijk [" + nickname + "]");
      privatewidget[listnum]->show();
      window_on[listnum]=TRUE;
   }
}

void sound(QString nickname, int listnum, int soundnr) // alleen door de server aangeroepen
{
   int user_nr;
   if (nickname!=nick)
   {
      if ( (user_nr=find_user(nickname))!=-1)
      {      
             QString sendstr;
             sendstr="/sound " + User[listnum].nickname +" ";
             QString temp;
             temp.setNum(soundnr);
             sendstr.append(temp+"\r\n");
             write(clientlist[user_nr], sendstr, sendstr.length());
      }
      else put_text("INTERNAL: No such user.");
   }
   else
   {
      receive_sound(User[listnum].nickname,soundnr);
   }
}

void receive_sound(QString nickname, int soundnr)
{
   int user;
   user=find_user(nickname);
   QString temp;
   temp.setNum(soundnr);
   put_text("INTERNAL: reveive_sound() activated "+nickname);
   if (window_on[user]==TRUE)
   {
      put_text("INTERNAL: window is on");
      if (accept_box[user]->isChecked()==TRUE)
      {         
                play_sound(soundnr);
                if (language==ENGLISH) 
                put_text("-=] " + nickname + " send you a sound. [=-");// # " + temp +". [=-");
                if (language==DUTCH) 
                put_text("-=] " + nickname + " verstuurde een geluid. [=-");// # " + temp +". [=-");
      }
   }
   else
   {
      if (accept_sound[user]==TRUE)
      {      	 
         put_text("INTERNAL: accept_sound = TRUE");
         play_sound(soundnr);
         if (language==ENGLISH) 
         put_text("-=] " + nickname + " send you a sound. [=-");// # " + temp +". [=-");
         if (language==DUTCH) 
         put_text("-=] " + nickname + " verstuurde een geluid. [=-");// # " + temp +". [=-");
      }
   }
}

OptionWidget::OptionWidget ( QWidget *parent, const char *name ) : QDialog (parent,name,TRUE)
{
   resize(300,340);
   setMaximumSize(300,340);
   setMinimumSize(300,340);
//   move(10,10);
   nick_changed=FALSE;

   QGroupBox *group_box = new QGroupBox(this, "options");
   if (language==ENGLISH) group_box->setTitle("Options");
   else group_box->setTitle("Opties");
   group_box->setGeometry(10,10,280,290);
   
 	QLabel *optiontext = new QLabel(this, "optiontext");
   if (language==ENGLISH) optiontext->setText("Nick :");
   else optiontext->setText("Alias :");
   optiontext->setGeometry(30,40,60,20);

   option_line_edit = new QLineEdit(this, "Nick");
   option_line_edit->setMaxLength(_UTS_NAMESIZE);
   option_line_edit->setGeometry(130,40,140,22);
   option_line_edit->setText(nick);
   option_line_edit->selectAll();
   option_line_edit->setFocus();
   connect(option_line_edit, SIGNAL(textChanged(const char *)), SLOT(option_nick_changed(const char *)) );

   QButtonGroup *sound = new QButtonGroup (this);
   sound->resize(240,90);
//   connect (sound, SIGNAL(clicked(int)),SLOT(updateIt(int)));
   if (language==ENGLISH) sound->setTitle("Sound");
   else sound->setTitle("Geluid");
   sound->move(30,85);
   if (language==ENGLISH) snd_on = new QRadioButton ("On",sound);
   else snd_on = new QRadioButton ("Aan",sound);
   snd_on->setGeometry (30,20,80,30);
   if (language==ENGLISH) snd_off = new QRadioButton ("Off", sound);
   else snd_off = new QRadioButton ("Uit", sound);
   snd_off->setGeometry (30,50,80,30);
   if (sound_on==TRUE)
   {
      snd_on->setChecked(TRUE);
      snd_off->setChecked(FALSE);
   }
   else
   {
      snd_on->setChecked(FALSE);
      snd_off->setChecked(TRUE);
   }

   QButtonGroup *slang = new QButtonGroup (this);
   slang->resize(240,90);
   if (language==ENGLISH) slang->setTitle("Language");
   else slang->setTitle("Taal");
   slang->move(30,190);
   if (language==ENGLISH) english = new QRadioButton ("English",slang);
   else english = new QRadioButton ("Engels",slang);
   english->setGeometry (30,20,100,30);
   if (language==ENGLISH) dutch = new QRadioButton ("Dutch", slang);
   else dutch = new QRadioButton ("Nederlands", slang);
   dutch->setGeometry (30,50,100,30);
   if (english_on==TRUE)
   {
   english->setChecked(TRUE);
   dutch->setChecked(FALSE);
   }
   else
   {
      english->setChecked(FALSE);
      dutch->setChecked(TRUE);
   }
   
   QPushButton *ok_button = new QPushButton (  this, "Ok" );
   ok_button->setText ("Ok");
   ok_button->setGeometry (210,310,80,25);
   ok_button->setDefault(TRUE);
   connect(ok_button, SIGNAL(clicked()), SLOT(option_close() ));

}

void OptionWidget::closeEvent(QCloseEvent *e)
{
   e->accept();
   this->~OptionWidget();
}

void OptionWidget::option_close()
{
   change_nick();
   
//   debug_on=FALSE;
   sound_on=FALSE;
   english_on=FALSE;
   if (snd_on->isChecked()==TRUE)sound_on=TRUE;
   if (english->isChecked()==TRUE) english_on=TRUE;

/*   if (snd_on->isChecked()==TRUE)
   {
      QString errormsg = "Sound not yet implemented";
      QMessageBox::critical(this, "Error", errormsg);
      sound_on=TRUE;
   }
*/
   
   write_config();
   this->~OptionWidget();
}

void OptionWidget::option_nick_changed(const char *new_nick)
{
   if (strcmp(new_nick,nick))
      nick_changed=TRUE;
   else
      nick_changed=FALSE;
}

void OptionWidget::change_nick()
{
   if (nick_changed==FALSE) return;
   QString output;
   if(chatmode == NONE)
   {  nick = (nick,option_line_edit->text());
      if (language==ENGLISH) output = "-=] You are now known as ";
      if (language==DUTCH) output = "-=] Je alias is veranderd in ";
      output.append(nick+". [=-");
      put_text(output);
   }
   if(chatmode == CLIENT)
   {
      output = "/nick ";
      output.append(option_line_edit->text());
      NewNick = option_line_edit->text();	// store the nick for the time being
      output.append("\r\n");						// necessary for transmission
      write(clientsock, output, strlen(output));	// send /nick crap to server
      put_text("INTERNAL: Requesting new nick.");	// debug info
   }
   if(chatmode == SERVER)
   {
      put_text("INTERNAL: You want to change your nick huh...");
      output = (option_line_edit->text());
      chservernick(output);
   }
   option_line_edit->selectAll();
}

LayoutWidget::LayoutWidget ( QWidget *parent, const char *name ) : QDialog (parent,name,TRUE)
{
   resize(300,340);
   setMaximumSize(300,375);
   setMinimumSize(300,375); 
//   move(10,10);

   QGroupBox *group_box = new QGroupBox(this, "layout");
   if (language==ENGLISH) group_box->setTitle("Layout");
   else group_box->setTitle("Opmaak");
   group_box->setGeometry(10,10,280,325);
      
 	QLabel *optiontext = new QLabel(this, "optiontext");
   if (language==ENGLISH) optiontext->setText("Font :");
   else optiontext->setText("Lettertype :");
   optiontext->setGeometry(30,40,100,20);

   layout_combo = new QComboBox(FALSE,this,"Font");
   layout_combo->setGeometry(130,40,140,22);
   layout_combo->setFocus();
   layout_combo->insertItem("helvetica",-1);
   layout_combo->insertItem("times",-1);
   layout_combo->insertItem("courier",-1);
   layout_combo->insertItem("lucida",-1);
   layout_combo->setCurrentItem(font_nr); 
   
   QButtonGroup *style = new QButtonGroup (this);
   style->resize(240,90);
   if (language==ENGLISH) style->setTitle("Style");
   else style->setTitle("Stijl");
   style->move(30,85);
   motif = new QRadioButton ("Motif",style);
   motif->setGeometry (30,20,80,30);
   windows = new QRadioButton ("Windows 95", style);
   windows->setGeometry (30,50,120,30);
   if (motif_on==TRUE)
   {
      motif->setChecked(TRUE);
      windows->setChecked(FALSE);
   }
   else
   {
      motif->setChecked(FALSE);
      windows->setChecked(TRUE);
   }

   QButtonGroup *userdisplay = new QButtonGroup (this);
   userdisplay->resize(240,90);
   if (language==ENGLISH) userdisplay->setTitle("Private Windows");
   else userdisplay->setTitle("Persoonlijke Windows");
   userdisplay->move(30,190);
   if (language==ENGLISH) priv_on = new QRadioButton ("On",userdisplay);
   else priv_on = new QRadioButton ("Aan",userdisplay);   
   priv_on->setGeometry (30,20,80,30);
   if (language==ENGLISH) priv_off = new QRadioButton ("Off", userdisplay);
   else priv_off = new QRadioButton ("Uit", userdisplay);
   priv_off->setGeometry (30,50,80,30);
   if (private_on==TRUE)
   {
      priv_on->setChecked(TRUE);
      priv_off->setChecked(FALSE);
   }
   else
   {
      priv_on->setChecked(FALSE);
      priv_off->setChecked(TRUE);
   }

   if (language==ENGLISH) splash_screen=new QCheckBox("Show splashscreen",this,"check it");
   if (language==DUTCH)	splash_screen=new QCheckBox("Toon splashscreen",this,"check it");
   if (splash_on==TRUE) splash_screen->setChecked(TRUE) ;
   else splash_screen->setChecked(FALSE);
   splash_screen->setGeometry(30,290,200,25);
   QPushButton *ok_button = new QPushButton (  this, "Ok" );
   ok_button->setText ("Ok");
   ok_button->setGeometry (210,345,80,25);
   ok_button->setDefault(TRUE);
   connect(ok_button, SIGNAL(clicked()), SLOT(layout_close() ));
}


void LayoutWidget::closeEvent(QCloseEvent *e)
{
   e->accept();
   this->~LayoutWidget();
}

void LayoutWidget::layout_close()
{
   motif_on=FALSE;
   private_on=FALSE;
   splash_on=FALSE;    
   if (motif->isChecked()==TRUE) motif_on=TRUE;
   if (priv_on->isChecked()==TRUE) private_on=TRUE;
   if (splash_screen->isChecked()==TRUE) splash_on=TRUE;
   font_nr=layout_combo->currentItem();

   QString infomsg ;
//   QMessageBox::information(this, "Info", infomsg);

   if ((QApplication::style()==WindowsStyle&&motif_on==TRUE)
      	||(QApplication::style()==MotifStyle && motif_on==FALSE))
   {
      if (language==ENGLISH) infomsg = "Changes will only be applied after restart.";
    if (language==DUTCH) infomsg = "Veranderingen worden geldig na herstart van VooDoo Chat.";
     QMessageBox::information(this, "Info", infomsg);
   }
/*
   if (priv_on->isChecked()==TRUE)
   {
      QString errormsg = "Image not yet implemented";
      QMessageBox::critical(this, "Error", errormsg);
      private_on=TRUE;
   }
*/   
   write_config();
   this->~LayoutWidget();
}


AddWidget::AddWidget ( QWidget *parent, const char *name ) : QDialog (parent, name,TRUE)
{
   int index;
//   add_user_window=ON;

   setMinimumSize (420,85);
   setMaximumSize (420,85);

   ok_button = new QPushButton (  this, "Ok" );
   ok_button->setText ("Ok");
   ok_button->setGeometry (320,10,80,25);
   connect(ok_button, SIGNAL(clicked()), SLOT(insert_user()  ));

   QPushButton *cancel_button = new QPushButton (  this, "Cancel" );
   if (language==ENGLISH) cancel_button->setText ("Cancel");
   else cancel_button->setText ("Annuleren");
   cancel_button->setGeometry (320,45,80,25);
   connect(cancel_button, SIGNAL(clicked()), SLOT(add_close()  ));

 	QLabel *nicktext = new QLabel(this, "nicktext");
   if (language==ENGLISH) nicktext->setText("Nick :");
   else nicktext->setText("Alias :");
   nicktext->setGeometry(10,10,60,20);
   
  	QLabel *dnstext = new QLabel(this, "dnstext");
   if (language==ENGLISH) dnstext->setText("DNS or IP :");
   else dnstext->setText("DNS of IP :");
   dnstext->setGeometry(10,45,80,20);

   add_user_nick = new QLineEdit( this, "name" );
 	add_user_nick->setMaxLength(48);
 	add_user_nick->setGeometry (100,10,190,22);
   add_user_nick->setFocus();
   connect(add_user_nick, SIGNAL(returnPressed()), SLOT(nick_return_pressed()) );

   add_user_dns = new QLineEdit( this, "dns" );
 	add_user_dns->setMaxLength(MAXDNSLENGTH);
  	add_user_dns->setGeometry (100,45,190,22);
   connect(add_user_dns, SIGNAL(returnPressed()), SLOT(dns_return_pressed()) );
}

void AddWidget::closeEvent(QCloseEvent *e)
{
   e->accept();
   this->~AddWidget();
}

void AddWidget::nick_return_pressed()
{
//   ok_button->setDefault(TRUE);
   add_user_dns->setFocus();
}

void AddWidget::dns_return_pressed()
{  
//   ok_button->setFocus();
   ok_button->setDefault(TRUE);
}

void AddWidget::add_close()
{
   this->~AddWidget();
}

void AddWidget::insert_user()
{
   if (strlen(add_user_nick->text()) > 0 && strlen(add_user_dns->text()) > 0 )
   {
       strcpy(user_info[number_of_users].alias,add_user_nick->text() );
       strcpy(user_info[number_of_users].ip,add_user_dns->text() );
       connect_list_box->insertItem(add_user_nick->text());
       number_of_users++;
    }
    this->~AddWidget();
}

EditWidget::EditWidget ( QWidget *parent, const char *name ) : QDialog (parent, name,TRUE)
{
   int index;

   setMinimumSize (420,85);
   setMaximumSize (420,85);

 	ok_button = new QPushButton (  this, "Ok" );
   ok_button->setText ("Ok");
   ok_button->setGeometry (320,10,80,25);
   connect(ok_button, SIGNAL(clicked()), SLOT(update_user()  ));

   QPushButton *cancel_button = new QPushButton (  this, "Cancel" );
   if (language==ENGLISH) cancel_button->setText ("Cancel");
   else cancel_button->setText ("Annuleren");
   cancel_button->setGeometry (320,45,80,25);
   connect(cancel_button, SIGNAL(clicked()), SLOT(edit_close()  ));

 	QLabel *nicktext = new QLabel(this, "nicktext");
   if (language==ENGLISH) nicktext->setText("Nick :");
   else nicktext->setText("Alias :");
   nicktext->setGeometry(10,10,60,20);
    
 	QLabel *dnstext = new QLabel(this, "dnstext");
   if (language==ENGLISH) dnstext->setText("DNS or IP:");
   else dnstext->setText("DNS of IP:");
   dnstext->setGeometry(10,45,80,20);

   edit_user_nick = new QLineEdit( this, "name" );
 	edit_user_nick->setMaxLength(48);
 	edit_user_nick->setGeometry (100,10,190,22);
   edit_user_nick->setFocus();
   edit_user_nick->setText(user_info[connect_list_box->currentItem()].alias);
   connect(edit_user_nick, SIGNAL(returnPressed()), SLOT(nick_return_pressed()) );

   edit_user_dns = new QLineEdit( this, "dns" );
 	edit_user_dns->setMaxLength(MAXDNSLENGTH);
 	edit_user_dns->setGeometry (100,45,190,22);
   edit_user_dns->setText(user_info[connect_list_box->currentItem()].ip);
   connect(edit_user_dns, SIGNAL(returnPressed()), SLOT(dns_return_pressed()) );

}

void EditWidget::closeEvent(QCloseEvent *e)
{
   e->accept();
   this->~EditWidget();
}

void EditWidget::nick_return_pressed()
{
//   ok_button->setDefault(TRUE);
   edit_user_dns->setFocus();
}

void EditWidget::dns_return_pressed()
{  
//   ok_button->setFocus();
   ok_button->setDefault(TRUE);
}

void EditWidget::edit_close()
{
   this->~EditWidget();
}

void EditWidget::update_user()
{
   strcpy (user_info[connect_list_box->currentItem()].alias,edit_user_nick->text());
   strcpy (user_info[connect_list_box->currentItem()].ip,edit_user_dns->text());
   connect_list_box->changeItem(edit_user_nick->text(),connect_list_box->currentItem());
   connect_line_edit->setText(user_info[connect_list_box->currentItem()].ip);

   this->~EditWidget();
}

AcceptInvite::AcceptInvite(QWidget *parent, const char *name) : QDialog(parent,name,TRUE)
{
   connect(this,SIGNAL(chat_mode_changed()),this, SLOT(change_popup_menu() ));

   accept_window = ON;
   this->setMinimumSize(300,135);
   this->setMaximumSize(300,135);
   
   QLabel *invite_text = new QLabel(this, "invite_text");
   if (language==ENGLISH)invite_text->setText(invite_nick + " invites you to join the discussion." 
      "\n\nDo you wish to join?");   if (language==DUTCH)invite_text->setText(invite_nick + " nodigt je uit om" +
      " te chatten." 
      "\n\nWilt je chatten?");   
   invite_text->setGeometry(20, 0, 260, 100);
   if (language==ENGLISH) this->setCaption("Accept Invitation?");
   if (language==DUTCH) this->setCaption("Accepteer uitnodiging?");
   QPushButton *yes_button = new QPushButton(this, "Yes");
   if (language==ENGLISH) yes_button->setText("Yes");
   if (language==DUTCH) yes_button->setText("Ja");
   yes_button->setFocus();
   yes_button->setGeometry(80, 100, 50, 25);
   connect(yes_button, SIGNAL(clicked()), SLOT(accept_invite()) );
   QPushButton *no_button = new QPushButton(this, "No");
   if (language==ENGLISH) no_button->setText("No");
   if (language==DUTCH) no_button->setText("Nee");
   no_button->setGeometry(170, 100, 50, 25);
   connect(no_button, SIGNAL(clicked()), SLOT(reject_invite()) );
}

void AcceptInvite::accept_invite()
{
   put_text("INTERNAL: accept_invite()");
   if(chatmode != NONE)
   {  
      this->hide();
      accept_window = OFF;
   }
   else
   {
      put_text("INTERNAL: Joining the discussion.");
      if( (clientsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;	// obtain fd
      bzero(&remoteserver, sizeof(remoteserver));
      remoteserver.sin_family = AF_INET;
      remoteserver.sin_port = htons(remoteport);
      if(inet_aton( IPnr, &remoteserver.sin_addr) <= 0) return;
      if( (::connect(clientsock, (SA *)&remoteserver, sizeof(remoteserver)) ) < 0) return;
      setnonblocking(clientsock);
      chatmode = CLIENT;
      emit chat_mode_changed();
      QString sendnick = "/connect ";
      sendnick.append(nick);
      sendnick.append("\r\n");
      NewNick = nick.copy();
      write(clientsock, sendnick, strlen(sendnick));
      ClientModeNotifier = new QSocketNotifier(clientsock, QSocketNotifier::Read);
      connect(ClientModeNotifier, SIGNAL(activated(int)), connectwidget, SLOT(receive_data()));
      put_text("INTERNAL: Connection established.");
      this->hide();
      accept_window = OFF;
   }
}

void AcceptInvite::reject_invite()
{
   this->hide();
   accept_window = OFF;
}

void AcceptInvite::change_popup_menu()
{   
    if (chatmode==NONE)
    {
       connection->setItemEnabled(id_connect,TRUE);
       connection->setItemEnabled(id_invite,FALSE);
       connection->setItemEnabled(id_disconnect,FALSE);
    }
    else if (chatmode==SERVER || chatmode==CLIENT)
    {
       connection->setItemEnabled(id_connect,FALSE);
       connection->setItemEnabled(id_invite,TRUE);
       connection->setItemEnabled(id_disconnect,TRUE);
    }
}

ConnectWidget::ConnectWidget ( QWidget *parent, const char *name ) : QWidget (parent,name)
{
   int index;
   connect_window=ON;
   setMinimumSize (420,200);
 	setMaximumSize (420,200);

   connect(this,SIGNAL(chat_mode_changed()),this, SLOT(change_popup_menu() ));

   QPushButton *add_button = new QPushButton (  this, "Add" );
   if (language==ENGLISH) add_button->setText ("&Add");
   else add_button->setText ("&Toevoegen");
 	add_button->setGeometry (320,10,80,25);
   if (language==ENGLISH)   QToolTip::add(add_button, "Add a user to the addressbook.");
   else QToolTip::add(add_button, "Voeg een gebruiker toe aan het adresboek.");
   connect(add_button, SIGNAL(clicked()), SLOT(add_user()  ));

   QPushButton *delete_button = new QPushButton (  this, "Delete" );
   if (language==ENGLISH)   delete_button->setText ("&Delete");
   else delete_button->setText ("Verwij&der");
   delete_button->setGeometry (320,45,80,25);
   if (language==ENGLISH)   QToolTip::add(delete_button, "Delete the selected user from the addressbook.");
   else QToolTip::add(delete_button, "Verwijder de geselecteerde gebruiker uit het adresboek.");
   connect(delete_button, SIGNAL(clicked()), SLOT(delete_user()  ));

   QPushButton *edit_button = new QPushButton (  this, "Edit" );
   if (language==ENGLISH) edit_button->setText ("&Edit");
   else edit_button->setText ("&Bewerken");
   edit_button->setGeometry (320,80,80,25);
   if (language==ENGLISH)   QToolTip::add(edit_button, "Edit the selected user.");
   else QToolTip::add(edit_button, "Bewerk de geselecteerde gebruiker.");
   connect(edit_button, SIGNAL(clicked()), SLOT(edit_user()  ));
 
    QPushButton *cancel_button = new QPushButton (  this, "Cancel" );
    if (language==ENGLISH)   cancel_button->setText ("&Cancel");
    else cancel_button->setText ("A&nnuleren");
 	cancel_button->setGeometry (320,115,80,25);
  	connect(cancel_button, SIGNAL(clicked()),SLOT(connect_close() ));

   connect_button = new QPushButton (  this, "Connect" );
   connect_button->setGeometry (320,160,80,25);
  	connect(connect_button, SIGNAL(clicked()), SLOT(change_remote_ip()  ));
   if (chatmode==NONE)
      if (language==ENGLISH) connect_button->setText ("C&onnect");
      else connect_button->setText ("&Verbinden");
   else if (chatmode==SERVER || chatmode==CLIENT)
   if (language==ENGLISH)      connect_button->setText ("&Invite");
   else connect_button->setText ("&Uitnodigen");
   connect_button->setEnabled(FALSE);
   if (language==ENGLISH) QToolTip::add(connect_button, "Connect to the selected ip.");
   else QToolTip::add(connect_button, "Maak een verbinding met het geselecteerde ip.");

   connect_line_edit= new QLineEdit( this, "IP" );
 	 connect_line_edit->setMaxLength(MAXDNSLENGTH);
 	 connect_line_edit->setGeometry (10,160,280,22);
   connect_line_edit->setFocus();
   connect(connect_line_edit,SIGNAL(returnPressed()),SLOT(change_remote_ip() ));   
   connect(connect_line_edit,SIGNAL(textChanged(const char *)),SLOT(connect_available(const char *) ));   

   connect_list_box = new QListBox ( this, "ListBox" );
 	 connect_list_box->setGeometry (10,10,280,140);      
   connect( connect_list_box, SIGNAL(highlighted(int)), SLOT( update_line_edit()));
   connect( connect_list_box, SIGNAL(selected(int)), SLOT( change_remote_ip()));  //changed
//fill the listbox
   for (index=0; index < number_of_users; ++index)
   {
      QString str; //,tmp;
      str.sprintf (user_info[index].alias);
      connect_list_box->insertItem(str);
   }	
}

void ConnectWidget::closeEvent(QCloseEvent *e)
{
   write_config();
   e->accept();
   connect_window=OFF;
}

void ConnectWidget::connect_close()
{
   this->hide();
   connect_window=OFF;
   write_config();
}

void ConnectWidget::connect2host()
   {	
	struct hostent *hptr;
	struct in_addr saddr;
  struct sockaddr_in sin;

	if(chatmode != NONE)
      {
      	invite();
        this->hide();		// Connect window is being hided... not destroyed
        connect_window=OFF;
        write_config();
      }
      else
      {	if( (clientsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
        bzero(&remoteserver, sizeof(remoteserver));
        remoteserver.sin_family = AF_INET;
        remoteserver.sin_port = htons(remoteport);

      	if( inet_aton(remote_ip, &remoteserver.sin_addr) == 0)
        {
          if( (hptr = gethostbyname(remote_ip)) == NULL) return;
          bzero((char *)&sin, sizeof(sin));
          bcopy(hptr->h_addr, (char *)&sin.sin_addr, hptr->h_length);
          if( inet_aton( inet_ntoa(sin.sin_addr), &remoteserver.sin_addr) == 0) return;
        }

//        if(inet_aton( (char *)&sin.sin_addr, &remoteserver.sin_addr) <= 0 ) return;
        if( (::connect(clientsock, (SA *) &remoteserver, sizeof(remoteserver)) ) < 0) return;
        setnonblocking(clientsock);       
        chatmode = CLIENT;
        emit chat_mode_changed();
        
        QString sendnick = "/connect ";
        QString temp;
        temp.setNum(language);
        sendnick.append(nick+","+temp);
        sendnick.append("\r\n");
        NewNick = nick.copy();
        write(clientsock, sendnick, strlen(sendnick));	// first to send is your nickname
        ClientModeNotifier = new QSocketNotifier(clientsock, QSocketNotifier::Read);
        connect(ClientModeNotifier, SIGNAL(activated(int)), SLOT(receive_data()) );
        if (language==ENGLISH) put_text("-=] Connection with server established. [=-");
        else put_text("-=] Verbinding met server tot stand gebracht. [=-");
//        connect_button->setText("Invite");
//      	emit connected();
//      	connected_users->request_user_list();

        this->hide();		// Connect window is being hided... not destroyed
        connect_window=OFF;
        write_config();
      }
   }

void ConnectWidget::update_line_edit()
{
   connect_line_edit->setText(user_info[connect_list_box->currentItem()].ip);
}

void ConnectWidget::add_user()
{
   AddWidget *add_user = new AddWidget(this,"Add widget");
   add_user->setCaption("Add user");
   add_user->exec();
}

void ConnectWidget::delete_user()
{
   if (connect_list_box->currentItem()>=0)
   {
       int index1,index2;
       index2=0;
       for (index1=0; index1 < number_of_users; ++index1)
       {
           if (index1 != connect_list_box->currentItem())
           {
              strcpy(user_info[index2].alias,user_info[index1].alias);
              strcpy(user_info[index2++].ip,user_info[index1].ip);		
           }
       }
       connect_list_box->removeItem(connect_list_box->currentItem());
       number_of_users--;
   }
}

void ConnectWidget::edit_user()
{
   if (connect_list_box->currentItem()>=0 )
   {
       EditWidget *edit_user = new EditWidget(this,"Edit widget");
       edit_user->setCaption("Edit user");
       edit_user->exec();
//       edit_user_dns->setText(user_info[connect_list_box->currentItem()].ip);
//       edit_user_nick->setText(user_info[connect_list_box->currentItem()].alias);
   }

}

void ConnectWidget::receive_data()
{  
   bool again=FALSE;
   bool ok;
   char buffer[MAXLINE+1];
   int lengte,temp,count,index;
   QString original;
   if( (lengte = read(clientsock, buffer, MAXLINE)) != -1)	// if socket contains data
   {
      if(lengte == 0)	// server closed conn
      {  
         if (language==ENGLISH) put_text("-=] The server threw you out. [=-");
         if (language==DUTCH) put_text("-=] De server is gestopt. [=-");
         count=nick_list_box->count();		//wissen van de nick list
         for (index=0;index<count;++index)
            nick_list_box->removeItem(0);	//
         ClientModeNotifier->~QSocketNotifier();
         ::close(clientsock);
         chatmode = NONE;
         emit chat_mode_changed();		// switch buttons
         return;
      }
      else	// data from server
      {  
         do {
         buffer[lengte-2] = '\0';	// trash those \n\r things on the end
         QString output= buffer;
         original=buffer;
         put_text("INTERNAL: Data received from the server.");
//         put_text(output);
//         put_text("output");
      	if ((output.find("\r",0,FALSE)) > -1 )
         {
            again=TRUE;
            put_text("INTERNAL: Double command received.");
            temp=original.find("\n",0,FALSE);
            output=original.left(temp);  // ouput krijgt wat output zou moeten zijn
            original=original.right(original.length()-(original.find("\n",0,FALSE)+1));
            strcpy(buffer,original);
         }
         else
            again=FALSE;
         if( (output.find("/nick ",0,FALSE)) > -1 )	// server changes your nick
         {
            nick = NewNick.copy();	// server says OK to this nick
            put_text("INTERNAL: server changed your nick into:");
            if (language==ENGLISH) output = "-=] You are now known as ";
            if (language==DUTCH) output = "-=] Je alias is verandert in ";
            output.append(nick+". [=-");
            put_text(output);
 //           connected_users->request_user_list(); // deze functie zal maar 1 keer werken
         }
         else if( (output.find("/server ",0,FALSE))>-1)
         {
//	numberofclients=0;
         		put_text("INTERNAL: This machine is not the server!");
            QString IPnr = output.right( output.length()-8);
            QString crap = "INTERNAL: Server IP is: ";
            crap.append(IPnr);
            put_text(crap);
            ClientModeNotifier->~QSocketNotifier();	// destroy
            ::close(clientsock);
            put_text("INTERNAL: I closed the conn. Now connecting to server.");
            if( (clientsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {  put_text("INTERNAL: Error on socket() call.");
               chatmode = NONE;
               emit chat_mode_changed();
               return;
            }
            bzero(&remoteserver, sizeof(remoteserver));
            remoteserver.sin_family = AF_INET;
            remoteserver.sin_port = htons(remoteport);
            if( inet_aton(IPnr, &remoteserver.sin_addr) <= 0)
            {  put_text("INTERNAL: Error on inet_aton() call.");
               chatmode = NONE;
               emit chat_mode_changed();
               return;
            }
            if( (::connect(clientsock, (SA *)&remoteserver, sizeof(remoteserver)) ) < 0)
            {  put_text("INTERNAL: Error on ::connect() call.");
               chatmode = NONE;
               emit chat_mode_changed();
               return;
            }
            setnonblocking(clientsock);
            ClientModeNotifier = new QSocketNotifier(clientsock, QSocketNotifier::Read);
            connect(ClientModeNotifier, SIGNAL(activated(int)), SLOT(receive_data()));
            put_text("INTERNAL: Connection with server established.");
            crap = "/nick ";	// re-use this QString. making new one is too expensive (mem, cpu)
            crap.append(nick);
            crap.append("\r\n");	// needed for transmission
            NewNick = nick.copy();
            write(clientsock, crap, strlen(crap));	// announce your nick
            return;
         }
         else if( (output.find("/private ",0,FALSE)) > -1 )	
         {
            put_text("INTERNAL: Private message received.");
//          put_text(output);
            QString nickname,showstring;
            temp=output.find("/",0,FALSE);
            output=output.right(output.length()-(temp+9)); 
            temp=output.find(" ",0,FALSE);
            nickname=output.left(temp);
            output=output.right(output.length()-temp);
            showstring="[";
            showstring.append(nickname);
            showstring.append("]=>");
            showstring.append(output);
            if (nickname!=nick && find_user(nickname)!=-1) 
            {
            	if (private_on==TRUE) // gooi de zooi in een main window
                  message2window(showstring,nickname);
               else
                  put_text(showstring);
            }
         }
         else if( (output.find("/sound",0,FALSE)) > -1 )	
         {
        	put_text("INTERNAL: Server sends /sound.");
            QString temp_nick;
            temp=output.find(" ",0,FALSE);
            output=output.right( output.length()-(temp+1)  );
            temp=output.find(" ",0,FALSE);
            temp_nick=output.left(temp);                    
            output=output.right( output.length()-(temp+1)  );
            temp=output.toInt(&ok);                                        
                                        
            receive_sound(temp_nick, temp);
         }
         else if( (output.find("/userlist",0,FALSE)) > -1 )	// server changes your nick
         {
//		put_text(output);
//            connected_users->refresh_list(output);
            put_text("INTERNAL: Server send userlist.");
//            connect_list_box->clearList(); //eerst de listbox wissen
            update_user_list(output);
         }
         else if( (output.find("/becomeserver",0,FALSE)) > -1 )	// ScArrY!!!!!!!!!!!
         {
        	numberofclients=0;    
		put_text("INTERNAL: Server is quiting you're becoming server.");
            chatmode=NONE;
            for (index=0;index<MAXCONNS;++index)
            {
            	clientlist[index] = 0;
                User[index].nickname= "";
            	User[index].validated=0;;
      }

         }
         else		// some user spoke
         {  
            put_text(output);
         }
      } while (again==TRUE);
      }
   }
}

void ConnectWidget::connect_available(const char * current)
{
   if(!strlen(current))
      connect_button->setEnabled(FALSE);
   else
      connect_button->setEnabled(TRUE);
}

void ConnectWidget::change_popup_menu()
{   
    if (chatmode==NONE)
    {
       connection->setItemEnabled(id_connect,TRUE);
       connection->setItemEnabled(id_invite,FALSE);
       connection->setItemEnabled(id_disconnect,FALSE);
    }
    else if (chatmode==SERVER || chatmode==CLIENT)
    {
       connection->setItemEnabled(id_connect,FALSE);
       connection->setItemEnabled(id_invite,TRUE);
       connection->setItemEnabled(id_disconnect,TRUE);
//       connected_users->request_user_list();
    }
}

void ConnectWidget::change_remote_ip()
{
   remote_ip=connect_line_edit->text();
   connect2host();
}

ChatWidget::ChatWidget (QWidget *parent, const char *name) : QWidget (parent,name)
{  
   connect(this,SIGNAL(chat_mode_changed()), this, SLOT(change_popup_menu() ));
   QTimer *timer =new QTimer (this);
   connect (timer,SIGNAL(timeout()),this,SLOT(remove_splash())  );
  
   timer->start(5000);	
   setMinimumSize (420,200);

   connectwidget = new ConnectWidget(0, "Connect Widget");
   if (language==ENGLISH) connectwidget->setCaption("Connect");
   else connectwidget->setCaption("Verbinden");
   connectwidget->hide();
   connect_window = OFF;

   QMenuBar *menubar = new QMenuBar(this);
   menubar->setSeparator(QMenuBar::InWindowsStyle);

   connection = new QPopupMenu;
   if (language==ENGLISH) menubar->insertItem("&Connect", connection);
   else menubar->insertItem("&Verbinding", connection);
   
   if (language==ENGLISH) id_connect = connection->insertItem("&Connect", this, SLOT(make_connection()), CTRL+Key_C );
   else id_connect = connection->insertItem("&Verbinden", this, SLOT(make_connection()), CTRL+Key_V );
   if (language==ENGLISH) id_invite = connection->insertItem("&Invite", this, SLOT(make_connection()), CTRL+Key_I );
   else id_invite = connection->insertItem("&Uitnodigen", this, SLOT(make_connection()), CTRL+Key_U );
   connection->setItemEnabled(id_invite,FALSE);
   if (language==ENGLISH) id_disconnect = connection->insertItem("&Disconnect", this, SLOT(disconnect()), CTRL+Key_D );
   else id_disconnect = connection->insertItem("Ver&breken", this, SLOT(disconnect()), CTRL+Key_B );
   connection->setItemEnabled(id_disconnect,FALSE);
   connection->insertSeparator();
   if (language==ENGLISH) connection->insertItem("E&xit", this, SLOT(exit_program()), CTRL+Key_X );
   else connection->insertItem("Af&sluiten", this, SLOT(exit_program()), CTRL+Key_S );

   QPopupMenu* edit = new QPopupMenu;
   if (language==ENGLISH) menubar->insertItem("&Edit", edit);
   else  menubar->insertItem("&Bewerken", edit);
   if (language==ENGLISH) edit->insertItem("Voodoo &Options", this, SLOT(options()), CTRL+Key_O);
   else edit->insertItem("Voodoo &Opties", this, SLOT(options()), CTRL+Key_O);
   if (language==ENGLISH) edit->insertItem("Voodoo &Layout", this, SLOT(layout()), CTRL+Key_L);
   else edit->insertItem("Voodoo Op&maak", this, SLOT(layout()), CTRL+Key_M);

   QPopupMenu* help = new QPopupMenu;
   menubar->insertItem("&Help", help);
   help->insertItem("Voodoo &Help", this, SLOT(help()), CTRL+Key_H);
   help->insertSeparator();
   if (language==ENGLISH) help->insertItem("&About", this, SLOT(about()), CTRL+Key_A);
   else help->insertItem("&Info", this, SLOT(about()), CTRL+Key_I);
   if (language==ENGLISH) help->insertItem("About &Qt", this, SLOT(aboutQt()), CTRL+Key_Q);
   else help->insertItem("&Qt Info", this, SLOT(aboutQt()), CTRL+Key_Q);

   chat_multi_line_edit = new QMultiLineEdit(this, "ML");
   chat_multi_line_edit->setReadOnly(TRUE);

   chat_line_edit = new QLineEdit(this, "chatline");
   chat_line_edit->setMaxLength(MAXLINE);
   chat_line_edit->setFocus();
   connect(chat_line_edit, SIGNAL(returnPressed()), SLOT(parser()) );

   if( (listensock = socket(AF_INET, SOCK_STREAM, 0)) < 0) exit(0);
   int reuse = 1;
   setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));	// no more TIME_WAIT
   setnonblocking(listensock);
   bzero(&localserver, sizeof(localserver));
   localserver.sin_family = AF_INET;
   localserver.sin_addr.s_addr = htonl(INADDR_ANY);
   localserver.sin_port = htons(VOODOOPORT);
   bind(listensock, (SA *) &localserver, sizeof(localserver));
   listen(listensock, MAXCONNS);
   QSocketNotifier *notifyinc;
   notifyinc = new QSocketNotifier(listensock, QSocketNotifier::Read);
   connect(notifyinc, SIGNAL(activated(int)), SLOT(handle_inc()));

   user_names = new QGroupBox(this, "user names");
   if (language==ENGLISH) user_names->setTitle("Users");
   else user_names->setTitle("Gebruikers");

   nick_list_box = new QListBox ( user_names, "ListBox" );
   connect( nick_list_box, SIGNAL(selected(const char *)), SLOT( nick_double_clicked(const char *)));  

}

void ChatWidget::closeEvent(QCloseEvent *e)
{
   write_config();
   disconnect();
/*   if(chatmode == CLIENT)											//toegevoegd
   {  write(clientsock, "/quit\r\n", strlen("/quit\r\n"));	//toegevoegd
      ClientModeNotifier->~QSocketNotifier();					//toegevoegd
      ::close(clientsock);												//toegevoegd
   }*/
   e->accept();
}

void ChatWidget::resizeEvent(QResizeEvent *e)
{
   chat_multi_line_edit->setGeometry(10,40,width()-200,height()-75);
   chat_line_edit->setGeometry(10,height()-29,width()-200,22);
//   options_button->setGeometry(width()-200,10,80,25);
//   help_button->setGeometry(width()-100,10,80,25);
   user_names->setGeometry( width()-180,35,170,height()-41);
 	nick_list_box->setGeometry (15,20,140,height()-75);      
}

void ChatWidget::parser()
{  
      int temp,listnum;
      QString input,output;
      char buffer[MAXLINE+1];

      input = chat_line_edit->text();	// read the typed text
      if(strlen(input) == 0) return;

      if (input[0]!='/')
      {
         if (chatmode == CLIENT)
         {  output=nick.copy();
            output.append("> ");
            output.append(input);         
            put_text(output);
            output.append("\r\n");
            write(clientsock, output, strlen(output));
         }
         if(chatmode == SERVER)
         {  output=nick.copy();
            output.append("> ");
            output.append(input);
            put_text(output);
            send2all(output, -1);
         }
         if(chatmode == NONE)
         {  
            if (language==ENGLISH)put_text("-=] Not connected. Click \"Connect\" or wait for an incoming chat. [=-");
            if (language==DUTCH)put_text("-=] Niet verbonden. Druk op \"Verbinden\" of wacht op"
            " een binnen komend gesprek. [=-");
         }
      }
      else if (input.find("/nick ",0,FALSE)>-1)
      {
         if(chatmode == NONE)
         {  
            QString temp;
            nick = input.right( input.length()-6);
            if (language==ENGLISH) temp = "-=] You are now known as ";
            if (language==DUTCH) temp = "-=] Je alias is verandert in ";
            temp.append(nick+". [=-");
            put_text(temp);
         }
         if(chatmode == CLIENT)
         {
            NewNick = input.right(strlen(input) - 6);	// store the nick for the time being
      		input.append("\r\n");						// necessary for transmission
            write(clientsock, input, strlen(input));	// send /nick crap to server
            put_text("INTERNAL: Requesting new nick.");	// debug info
         }
         if(chatmode == SERVER)
         {
         	put_text("INTERNAL: /nick detected in servermode.");
            chservernick( input.right(strlen(input) - 6) );
         }
      }
      else if (input.find("/options",0,FALSE)>-1)
      {
         put_text("INTERNAL: Displaying options menu.");
         chat_line_edit->setText("");
         options();
      }
      else if (input.find("/layout",0,FALSE)>-1)
      {
         put_text("INTERNAL: Displaying layout menu.");
         chat_line_edit->setText("");
         layout();
      }
      else if(input.find("/whois ",0,FALSE)>-1)
      {  if(chatmode == CLIENT)
         {  input.append("\r\n");
            write(clientsock, input, strlen(input));
            put_text("INTERNAL: Requesting whois information.");
         }
         if(chatmode == SERVER)
         {
      		put_text("INTERNAL: Serveruser said /whois.");
            whois(input.right( input.length()-7), -1);
         }
      }
      else if(input.find("/debug",0,FALSE)>-1)
      {  if(debug_on==FALSE)
         {  debug_on = TRUE;
            put_text("-=] Internal debug messages turned on. Type /debug again to turn off. [=-");
         }
         else
         {  debug_on = FALSE;
            put_text("-=] Internal debug messages turned off. Type /debug again to turn on. [=-");
         }
      }
      else if(input.find("/msg ",0,FALSE)>-1)
      {  if(chatmode == CLIENT)
         {  
            QString showstring,nickname;
            input.append(" \r\n");
            write(clientsock, input, strlen(input));
            put_text("INTERNAL: Sending private message.");
          	input=input.right(input.length()-5);  //haal '/msg ' eraf
            temp=input.find(" ",0,FALSE);
            nickname=input.left(temp);
            input=input.right(input.length()-temp);  //haal NICK eraf
          	showstring="=>[";
            showstring.append(nickname);
            showstring.append("]");
            showstring.append(input);
            if (nickname!=nick && find_user(nickname)!=-1) 
            {
            	if (private_on==TRUE)
                  message2window(showstring,nickname);
               else
                  put_text(showstring); //don't private chat with yourself or not xsisting user
            }
         }
         if(chatmode == SERVER)
         {
         	QString nickname,showstring,sendstring,original;

         	original=input.copy();
         	input=input.right(input.length()-5);  //haal '/msg ' eraf
            temp=input.find(" ",0,FALSE);
            nickname=input.left(temp);
            listnum=find_user(nickname);
         	input=input.right(input.length()-temp);  //haal NICK eraf
            sendstring.setNum(MAXCONNS); //server number
            sendstring.append("/private ");
            sendstring.append(nick);
            sendstring.append(input);            
            if (listnum==-1)
               put_text("INTERNAL: No such user.");
            else
            {
               sendstring.append(" \r\n");
               write(clientlist[listnum], sendstring, sendstring.length());
               put_text("INTERNAL: Sending private message.");
            }
           	showstring="=>[";
            showstring.append(nickname);
            showstring.append("]");
            showstring.append(input);
            if (nickname!=nick && find_user(nickname)!=-1) 
            {
               if (private_on==TRUE)
                  message2window(showstring,nickname);
               else
                  put_text(showstring); //don't private chat with yourself
            }
         }
      }
      else if(input.find("/connect ",0,FALSE)>-1)
      {  if(chatmode == NONE)
         {  
           	put_text("INTERNAL: Trying to connect.");
            remote_ip=input.right( input.length()-9);
            connectwidget->connect2host();
         }
      }
      else if(input.find("/invite",0,FALSE)>-1)
      {  if(chatmode != NONE)
         {  
           	put_text("INTERNAL: Trying to invite.");
            remote_ip=input.right( input.length()-8);
            invite();
         }
      }
      else if(input.find("/quit",0,FALSE)>-1)
      {  
         chatwidget->exit_program();
      }
      else if(input.find("/disconnect",0,FALSE)>-1)
      {  
         chatwidget->disconnect();
      }
      else if(input.find("/sound",0,FALSE)>-1)
      {  
           	put_text("INTERNAL: Switching sound.");
            if (sound_on==TRUE)
            {
               sound_on=FALSE;
               put_text("-=] Sound is disabled. [=-");
            }
            else
            {
               sound_on=TRUE;
               put_text("-=] Sound is enabled. [=-");
            }
      }
      else if(input.find("/debug",0,FALSE)>-1)
      {  if(debug_on==FALSE)
         {  debug_on = TRUE;
            put_text("-=] Internal debug messages turned on. Type /debug again to turn off. [=-");
         }
         else
         {  debug_on = FALSE;
            put_text("-=] Internal debug messages turned off. Type /debug again to turn on. [=-");
         }
      }
      chat_line_edit->setText("");
}

void ChatWidget::make_connection()
{   
   if (connect_window==OFF)
   {
//      connectwidget = new ConnectWidget(0, "Connect Widget");
//       connectwidget->setCaption("Connect");
       connectwidget->show();
   }
}

void ChatWidget::options()
{   
    OptionWidget *optionwidget = new OptionWidget(this, "Option Widget");
    if (language==ENGLISH) optionwidget->setCaption("Voodoo Options");
    else optionwidget->setCaption("Voodoo Opties");
    optionwidget->exec();
}

void ChatWidget::layout()
{   
    LayoutWidget *layoutwidget = new LayoutWidget(this, "Layout Widget");
    if (language==ENGLISH) layoutwidget->setCaption("Voodoo Layout");
    else layoutwidget->setCaption("Voodoo Opmaak");
    layoutwidget->exec();
}

void ChatWidget::invitewindow()
{
   if (accept_window==OFF)
   {
      AcceptInvite *acceptinvite = new AcceptInvite(this, "Accept Invite");
      acceptinvite->show();
   }
}

void ChatWidget::handle_inc()	// handle new incoming connections here, watch and shiver!
{  int connectionfd, listnum;
   struct sockaddr_in peer;
   int len = sizeof(peer);
   QString output;
   
   put_text("INTERNAL: Incoming connection detected.");
   
   if((chatmode == NONE)||(chatmode == SERVER))
   {  chatmode = SERVER;
      emit chat_mode_changed();
      put_text("INTERNAL: Switching to server mode if not done allready.");
      if( (connectionfd = accept(listensock, NULL, NULL)) < 0)
      {  chatmode = NONE;
         emit chat_mode_changed();
         put_text("INTERNAL: Accept error! Too bad for client.");
         return;
      }
      setnonblocking(connectionfd);
      put_text("INTERNAL: Checking for room.");
      // try to find a free entry in the clientlist
      for(listnum = 0; (listnum<MAXCONNS)&&(connectionfd!= -1); listnum++)
      {  if(clientlist[listnum] == 0)	// not full yet, conn accepted!
         {  clientlist[listnum] = connectionfd;
            put_text("INTERNAL: Enough space, registering new user.");
            connectionfd = -1;
            notifylist[listnum] = new QSocketNotifier(clientlist[listnum], QSocketNotifier::Read);
            connect(notifylist[listnum], SIGNAL(activated(int)), SLOT(deal_with_data()) );
            // QObject::connect(), ::connect(), connect()... damn I love this object oriented crap! NOT ;)
            ++numberofclients;
            put_text("INTERNAL: User inserted, now monitoring its clientsock.");
            User[listnum].validated = 0;
            User[listnum].nickname = "new_user"; 
//            senduserlist2all();
         }
      }
      if(connectionfd != -1)	// server = full
      {
         put_text("INTERNAL: Maximum connections reached, closing connection.");
         write(connectionfd, "/serverfull\r\n", sizeof("/serverfull\r\n"));
         ::close(connectionfd);
      }
   }
   else	
   {	// too bad.. we're dedicated client. Sending the IP of the chatserver
      put_text("INTERNAL: Not in servermode, kindly sending rejection.");
      if( (connectionfd = accept(listensock, NULL, NULL)) < 0)
      {
         put_text("INTERNAL: Accept error! Too bad for client.");
         return;
      }
      setnonblocking(connectionfd);
      getpeername(clientsock, (SA *)&peer, &len);	// get the server's IP
      output = "INTERNAL: Forwarding server IP (";
      output.append(inet_ntoa(peer.sin_addr));
      output.append(") to client.");
      put_text(output);
      output = "/server ";
      output.append(inet_ntoa(peer.sin_addr));
      output.append("\r\n");
      write(connectionfd, output, strlen(output));	// and send it over
      ::close(connectionfd);
      put_text("INTERNAL: Connection with new client closed.");
   }
}

void ChatWidget::deal_with_data()
{  
   bool ok;
   char buffer[MAXLINE+1];
   int lengte,temp,to_user,count,index;
   put_text("INTERNAL: Incoming data detected.");
   // first figure out which socket(s) contain(s) data
   for(int listnum = 0; listnum<MAXCONNS; listnum++)	// cycle through whole list
   {  if(clientlist[listnum] != 0)	// only trigger on connected sockets
      {  if( (lengte = read(clientlist[listnum], buffer, MAXLINE)) != -1)	// if socket contains data
         {  
            if(lengte == 0)	// conn was closed
            {
               notifylist[listnum]->~QSocketNotifier();
               notifylist[listnum] = NULL;	// if so, the entry has now been reset
               ::close(clientlist[listnum]);	// and, just in case, I close my end of the conn
               clientlist[listnum] = 0;	// reset old socket
               put_text("INTERNAL: Client has quit VooDooChat.");
               --numberofclients; // one soul less
               if (User[listnum].validated==1)
               {
                	QString e_quitstring="-=] ";//houdt de rest op dce hoogte
                	QString d_quitstring="-=] ";//houdt de rest op dce hoogte
                    e_quitstring.append(User[listnum].nickname.copy());
                    d_quitstring.append(User[listnum].nickname.copy());
                    e_quitstring.append(" has quit VooDooChat. [=-");
                    d_quitstring.append(" heeft VooDooChat afgesloten. [=-");
                    if (language==ENGLISH) put_text(e_quitstring);
                    if (language==DUTCH) put_text(d_quitstring);
                    sendlanguage2all(e_quitstring,d_quitstring, -1);
               }
               
               senduserlist2all();
               if(numberofclients == 0)
               {  chatmode = NONE;
                  emit chat_mode_changed();
                  if (language==ENGLISH) put_text("-=] You're the only one remaining... disconnected. [=-");
                  if (language==DUTCH) put_text("-=] Je bent de enige... verbinding verbroken. [=-");
                  count=nick_list_box->count();		//wissen van de nicklist
                  for (index=0;index<count;++index)
                     nick_list_box->removeItem(0);	//

                  put_text("INTERNAL: Everyone left, no more server functions.");
                  return;
               }
            }
            else	// client typed something
            {  buffer[lengte-2] = '\0';	// trash those \n\r things on the end
               QString incoming = buffer;
               if(incoming.find("/nick ",0,FALSE)>-1)	// user changed nick
               {
                  put_text("INTERNAL: Client sends /nick:");

//                  put_text(incoming.right( incoming.length()-6));          
                  nickevent(incoming.right( incoming.length()-6), listnum);
//                  senduserlist2all();
               }
               else if(incoming.find("/connect ",0,FALSE)>-1)
               {
               	  chatwidget->show();
                  QString temp_string;
                  put_text("INTERNAL: Client sends /connect:");
                  temp = incoming.find(",",0,FALSE);
                  temp_string = incoming.right(incoming.length()-temp-1);
                  incoming=incoming.left(temp);
//                  put_text(incoming);
                  User[listnum].language=temp_string.toInt(&ok);
                  //put_text(temp_string);
//                  put_text(incoming.right( incoming.length()-9));          
                  nickevent(incoming.right( incoming.length()-9), listnum);
//                  senduserlist2all();
               }
               else if((incoming.find("/invite ",0,FALSE) > -1)&&(numberofclients == 1))
               {
                  chatwidget->show(); // gooit de chatwidget open
                  QString crap = "INTERNAL: You have been invited to ";
                  incoming=incoming.right(incoming.length()-8);
                  invite_nick=incoming.right( incoming.length() - incoming.find(" ",0,FALSE) -1 );
                  incoming=incoming.left(incoming.find(" ",0,FALSE));
                  //IPnr = incoming.right(incoming.length()-8);
                  IPnr = incoming;
                  crap.append(IPnr);
                  put_text(crap);
                  notifylist[listnum]->~QSocketNotifier();
                  notifylist[listnum] = NULL;	// the entry has now been reset
                  ::close(clientlist[listnum]);	// and, just in case, I close my end of the conn
                  clientlist[listnum] = 0;	// reset old socket
                  --numberofclients; // one soul less
                  chatmode = NONE;
                  emit chat_mode_changed();
                  invitewindow();
                  // display "accept invitation"-window now.
                  return;
               }
               else if(incoming.find("/whois ",0,FALSE)>-1)
               {
               		put_text("INTERNAL: Client sends /whois.");
                  whois(incoming.right( incoming.length()-7), listnum);
               }
               else if(incoming.find("/msg ",0,FALSE)>-1)
               {
                  QString nickname,sendstring,showstring,original;
                  original=incoming.copy();
                  incoming=incoming.right(incoming.length()-5);  //haal '/msg ' eraf
                  temp=incoming.find(" ",0,FALSE);
                  nickname=incoming.left(temp);
                  incoming=incoming.right(incoming.length()-temp);  //haal NICK eraf
                  
                  if (nick.lower()==nickname.lower()) //msg dus voor de server besoelt
                  {
                  	put_text("INTERNAL: Private message for the server.");
                     showstring="[";
                     showstring.append(User[listnum].nickname);
                     showstring.append("]=>");
                     showstring.append(incoming);
                     if(User[listnum].validated==1)
                     {
                        if (private_on==TRUE)
                           message2window(showstring,User[listnum].nickname);
                        else
                           put_text(showstring);
                     }
                  }
                  else
                  {                 
                     to_user=find_user(nickname);
//                     incoming.prepend("/msg");
                     if (to_user==-1)
                        put_text("INTERNAL: No such user.");
                     else  //als de message van een client naar een andere client moet
                     {
                        sendstring.setNum(listnum); //client number from sender
                        sendstring="/private ";
                        sendstring.append(User[listnum].nickname);
                        sendstring.append(incoming);
                     	sendstring.append("\r\n");
                        write(clientlist[to_user], sendstring, sendstring.length());
                        put_text("INTERNAL: Sending private message.");
                     }
                  }
               }
               else if(incoming.find("/sound",0,FALSE)>-1)
               {
               		put_text("INTERNAL: Client sends /sound.");
                    QString temp_nick;
                    temp=incoming.find(" ",0,FALSE);
                    incoming=incoming.right( incoming.length()-(temp+1)  );
                    temp=incoming.find(" ",0,FALSE);
                    temp_nick=incoming.left(temp);                    
                    incoming=incoming.right( incoming.length()-(temp+1)  );
                    temp=incoming.toInt(&ok);                                        
                                        
                    sound(temp_nick, listnum, temp);
               }


//               else if(incoming.find("/requestuserlist",0,FALSE)>-1)
//               {
//               	put_text("INTERNAL: Client requests userlist.");
//                  senduserlist2all()
//               }
               else if(User[listnum].validated == 1)
               {  
                  put_text(buffer);
                  send2all(buffer, listnum);	// send string to all EXCEPT listnum
               }
            }
         }
      }
   }   
} 

void ChatWidget::disconnect()
{   
   struct sockaddr_in peer;
   int len = sizeof(peer);
   QString output;
   int count,index;
   write_config();
   if(chatmode == CLIENT)
   { 
//      write(clientsock, "/quit\r\n", strlen("/quit\r\n"));
      ClientModeNotifier->~QSocketNotifier();
      ::close(clientsock);
      chatmode = NONE;
      emit chat_mode_changed();
      put_text("-=] Disconnected from server. [=-");
      count=nick_list_box->count();		//wissen van de nick list
      for (index=0;index<count;++index)
      {
         nick_list_box->removeItem(0);	//
         User[index].nickname=0;
         User[index].validated=0;
         User[index].validated=0;
      }
      numberofclients=0;
   }
   if (chatmode == SERVER)
   {
      //de server zal een string naar de eerstvolgende client versturen zodat deze
      //door heeft dat hij als server zal draaien
      for (index=0;index<MAXCONNS;++index)
      {
      	if (User[index].validated==1)
        break;
      }
      sendlanguage2all("-=] Server quit, connecting to new server. [=-" ,
      "-=] Server is gestopt, nieuwe verbinding wordt gemaakt. [=-" ,index);
      QString sendstr="/becomeserver\r\n";
      write(clientlist[index],sendstr , sendstr.length());  
      //verstuurt het /server commando naar alle andere cients
      getpeername(clientlist[index], (SA *)&peer, &len);	// get the server's IP
      output = "INTERNAL: Forwarding server IP (";
      output.append(inet_ntoa(peer.sin_addr));
      output.append(") to client.");
      put_text(output);
      output = "/server ";
      output.append(inet_ntoa(peer.sin_addr));
//      output.append("\r\n");
	send2all(output,index);

     for (index=0;index<MAXCONNS;++index)
     {
       	if (clientlist[index] != 0)
	{	::close(clientlist[index]);
		notifylist[index]->~QSocketNotifier();
	}
     }
/* lijst wissen */
   count=nick_list_box->count();
   for (index=0;index<count;++index)
      nick_list_box->removeItem(0);
     numberofclients=0;
     chatmode=NONE;
     emit chat_mode_changed();
   }
}

void ChatWidget::change_popup_menu()
{   
    if (chatmode==NONE)
    {
       connection->setItemEnabled(id_connect,TRUE);
       connection->setItemEnabled(id_invite,FALSE);
       connection->setItemEnabled(id_disconnect,FALSE);
    }
    else if (chatmode==SERVER || chatmode==CLIENT)
    {
       connection->setItemEnabled(id_connect,FALSE);
       connection->setItemEnabled(id_invite,TRUE);
       connection->setItemEnabled(id_disconnect,TRUE);
    }
}

void ChatWidget::exit_program()
{   
   write_config();
   int index,count;

   if(chatmode == CLIENT)
   {  
//      write(clientsock, "/quit\r\n", strlen("/quit\r\n"));
      ClientModeNotifier->~QSocketNotifier();
      ::close(clientsock);
      chatmode = NONE;
      emit chat_mode_changed();
      put_text("-=] Disconnected from server. [=-");
      count=nick_list_box->count();		//wissen van de nick list
      for (index=0;index<count;++index)
         nick_list_box->removeItem(0);	//
      //alle andere windows sluiten
      connectwidget->hide();
      for (index=0;index<=MAXCONNS;++index)
      	if (window_on[index]==TRUE)
         {
            window_on[index]=FALSE;
            privatewidget[index]->~PrivateWidget();
         }
   }
   this->iconify();
}
 
void ChatWidget::help()
{
   pid_t pid;
   if( (pid = fork()) == 0)
   {  
      if(language == ENGLISH) system("xpdf voodoohelp.pdf");
      if(language == DUTCH)   system("xpdf voodoohelp-nl.pdf");
      exit(0);
   }
}

void ChatWidget::about()
{
   if (language==ENGLISH) QMessageBox::about(this, "About",
                    	"VooDooChat was developed as a school project for\n"
                        "Hoge School Rotterdam & Omstreken (HR&O)\n\n"
                        "Developers :\n\nErik van Zijst\nLeon van Zantvoort\nJan Graumans\n"
                        "Ids Achterhof\nErwin Springelkamp\nIngo Pak\nYan Yuen\n"
                     );
   if (language==DUTCH) QMessageBox::about(this, "Info",
                    	"VooDooChat is ontwikkeld in opdracht van :\n\n"
                        "Hoge School Rotterdam & Omstreken (HR&O)\n\n"
                        "Ontwerpers :\n\nErik van Zijst\nLeon van Zantvoort\nJan Graumans\n"
                        "Ids Achterhof\nErwin Springelkamp\nIngo Pak\nYan Yuen\n"
                     );
}

void ChatWidget::aboutQt()
{
   if (language==ENGLISH) QMessageBox::aboutQt(this, "About Qt");
   else QMessageBox::aboutQt(this, "Qt Info");
}

void ChatWidget::nick_double_clicked(const char * nickname)
{
   if (private_on==TRUE) 
      create_private_window(nickname);
}

void ChatWidget::remove_splash()
{
   splash->hide();
}

PrivateWidget::PrivateWidget (QWidget *parent, const char *name) :QWidget (parent,name)
{  
   setMinimumSize (445,200);
   clientnr=temp_clientnr;
   clientnick=temp_clientnick.copy();
   
   multi_line = new QMultiLineEdit(this, "ML");
   multi_line ->setReadOnly(TRUE);
   line_edit = new QLineEdit(this, "chatline");
   line_edit->setMaxLength(MAXLINE);
   line_edit->setFocus();
   connect(line_edit, SIGNAL(returnPressed()), SLOT(private_parser()) );

   sound_combo = new QComboBox(FALSE,this,"Sound");
   sound_combo->insertItem("Don't push it",-1);
   sound_combo->insertItem("...I'm coming back for you",-1);
   sound_combo->insertItem("Make it quick",-1);
   sound_combo->insertItem("What do you want",-1);
   sound_combo->insertItem("Don't force me to...",-1);
   sound_combo->insertItem("Are you listening",-1);
   sound_combo->insertItem("Pig",-1);
   sound_combo->insertItem("Fart",-1);
   sound_combo->insertItem("What is it",-1);
   sound_combo->insertItem("Whaaatt!",-1);
   sound_combo->insertItem("I would not do such...",-1);
   sound_combo->insertItem("Burp...",-1);
   sound_combo->insertItem("huh?",-1);
   sound_combo->insertItem("I'm not listening",-1);
   sound_combo->insertItem("Uh ooh",-1);

   send_button = new QPushButton (  this, "Send");
   if (language==ENGLISH) send_button->setText ("&Send");
   else send_button->setText ("&Versturen");
   if (language==ENGLISH) QToolTip::add(send_button, "Send user a sound.");
   else QToolTip::add(send_button, "Stuur een geluidje naar een gebruiker.");
   connect(send_button, SIGNAL(clicked()), SLOT(send_sound()  ));
   
//   accepttext = new QLabel(this, "accepttext");
//   if (language==ENGLISH) accepttext->setText("Accept sound :");
//   if (language==DUTCH) accepttext->setText("Ontvang geluid :");
//   QString temp;
//   temp.setNum(clientnr);
   if (language == ENGLISH) accept_box[clientnr]=new QCheckBox("Accept sound",this,"check");
   else accept_box[clientnr]=new QCheckBox("Accepteer geluiden",this,"check");

   if (accept_sound[clientnr]==TRUE) 
      accept_box[clientnr]->setChecked(TRUE);
   else 
      accept_box[clientnr]->setChecked(FALSE);
}


void PrivateWidget::closeEvent(QCloseEvent *e)
{
   window_on[clientnr]=FALSE;
   if (accept_box[clientnr]->isChecked()==TRUE) 
      accept_sound[clientnr]=TRUE;
   else accept_sound[clientnr]=FALSE;
      
   this->~PrivateWidget();
}

void PrivateWidget::resizeEvent(QResizeEvent *e)
{
//   multi_line->setGeometry(3,55,width()-7,height()-59);
   multi_line->setGeometry(10,45,width()-21,height()-80);
   line_edit->setGeometry(10,height()-29,width()-21,22);
   sound_combo->setGeometry(10,10,170,22);
   send_button->setGeometry (195,10,80,25);
   accept_box[clientnr]->setGeometry (290,10,145,25);
// private_group_box->setGeometry(250,10,150,60);
}

void PrivateWidget::update_multi_line(QString input, QString nickname)
{
   QString line,showstring;
   put_text("INTERNAL: Update multi line.");

   if ( input[0]=='=')	// you send message away
   {
      line=input.right(input.length()-(nickname.length()+4));
      showstring=nick.copy();
      showstring.append(">");
      showstring.append(line);
      put_private_text(showstring);
   }
   if ( input[0]=='[')	// you receive message
   {
      line=input.right(input.length()-(nickname.length()+4));
      showstring=nickname.copy();
      showstring.append(">");
      showstring.append(line);
      put_private_text(showstring);
//      if(this->isActiveWindow()==FALSE)play_sound(1);
   }
}

void PrivateWidget::put_private_text(QString message)
{  int doorgaan = 1;
   char buffer[MAXLINE+1];
   if (!(message.find("INTERNAL",0,TRUE)==0 && debug_on==FALSE ))
   {   strcpy(buffer, message);
       while(doorgaan == 1)
       {
         if((buffer[strlen(buffer)-1] == '\r')||(buffer[strlen(buffer)-1] == '\n'))
         {  //put_text("INTERNAL: Last character is cr or nl. Deleted...");
            buffer[strlen(buffer)-1] = '\0';
         }
         else doorgaan=0;
       }
       multi_line->append(buffer);
       multi_line->setCursorPosition(multi_line->numLines(),0,FALSE);
   }
}

void PrivateWidget::set_client_info(int listnum,QString nickname)
{
   clientnr=listnum;
   clientnick=nickname.copy();   
}


void PrivateWidget::private_parser()
{  
      int temp,listnum;
      char buffer[MAXLINE+1];

      QString input;
      input = line_edit->text();	// read the typed text
      if(strlen(input) == 0) return;
      if (input[0]!='/')
      {
         if (chatmode == CLIENT)
         {  
            QString showstring,nickname,sendit;
            sendit="/msg ";
            sendit.append(clientnick);
            sendit.append(" ");             
            sendit.append(input);
            sendit.append(" \r\n");
            write(clientsock, sendit,sendit.length());
            put_text("INTERNAL: Sending private message.");
          	
            nickname=clientnick.copy();
          	showstring="=>[";
            showstring.append(nickname);
            showstring.append("] ");
            showstring.append(input);
            if (nickname!=nick && find_user(nickname)!=-1) 
            {
            	if (private_on==TRUE)
                  message2window(showstring,nickname);
               else
                  put_text(showstring); //don't private chat with yourself or not xsisting user
            }
         }
         if(chatmode == SERVER)
         { 
         	QString nickname,showstring,sendstring;

            nickname=clientnick.copy();
            listnum=find_user(nickname);
            sendstring.setNum(MAXCONNS); //server number
            sendstring.append("/private ");
            sendstring.append(nick);
            sendstring.append(" ");
            sendstring.append(input);            
            if (listnum==-1)
               put_text("INTERNAL: No such user.");
            else
            {
               sendstring.append(" \r\n");
               write(clientlist[listnum], sendstring, sendstring.length());
               put_text("INTERNAL: Sending private message.");
            }
           	showstring="=>[";
            showstring.append(nickname);
            showstring.append("] ");
            showstring.append(input);
            if (nickname!=nick && find_user(nickname)!=-1) 
            {
               if (private_on==TRUE)
                  message2window(showstring,nickname);
               else
                  put_text(showstring); //don't private chat with yourself
            }
         }
      }
      else if (input.find("/sound",0,FALSE)>-1)
      {
//      	if (accept_sound[clientnr]==TRUE)
      	
        if (accept_box[clientnr]->isChecked()==TRUE)
        {
           accept_sound[clientnr]=FALSE;
           if (language==ENGLISH) multi_line->append("-=] Sound from " + clientnick + 
              " will no longer be played. [=-");
           if (language==DUTCH) multi_line->append("-=] Geluid van " + clientnick + 
              " wordt niet meer afgespeeld. [=-");
           multi_line->setCursorPosition(multi_line->numLines(),0,FALSE);
        }
        else
        {
           accept_sound[clientnr]=TRUE;
           if (language==ENGLISH) multi_line->append("-=] Sound from " + clientnick + " will now be played. [=-");
           if (language==DUTCH) multi_line->append("-=] Geluid van " + clientnick + " wordt weer afgespeeld. [=-");
           //multi_line->append(buffer);
           multi_line->setCursorPosition(multi_line->numLines(),0,FALSE);        }
           //accept_box[clientnr]->setChecked(TRUE);
      }
      accept_box[clientnr]->setChecked(FALSE);
      if (accept_sound[clientnr]==TRUE)
         accept_box[clientnr]->setChecked(TRUE);

      line_edit->setText("");
}


void PrivateWidget::send_sound()
{
//   play_sound(sound_combo->currentItem());
   QString sendstr,nr;
   if (chatmode==CLIENT)
   {
      sendstr="/sound " +clientnick + " ";
      nr.setNum(sound_combo->currentItem()+4);
      sendstr.append(nr+"\r\n");
      write(clientsock, sendstr, sendstr.length());
      put_text("INTERNAL: Sound send to user.");
   }
   if (chatmode==SERVER)
   {
      put_text("INTERNAL: sending sound directly to client.");
      nr.setNum(sound_combo->currentItem()+4);
      sendstr="/sound " + nick +" "+ nr;
      sendstr.append("\r\n");
      write(clientlist[clientnr], sendstr, sendstr.length());
   }
}

/*SplashWidget::SplashWidget (QWidget *parent, const char *name) 
: QLabel (parent,name,TRUE,WStyle_Customize | WStyle_NoBorder | WStyle_Tool)
{  
//   QPaintEvent plaatje;
   //show_pixmap(&plaatje);  
/*   QPicture pic;
   pic.load("/usr/local/qt/examples/picture/car.pic");
   QPainter splash;
   splash.begin(this);
   splash.drawPicture(pic);
   splash.end();
  */
  
/*  bool ok;
  ok= pic.load("kmix.xpm");
 if (ok) repaint();
}

void SplashWidget::paintEvent(QPaintEvent *e)
{
   bitBlt (this,0,0,&pic);
}
*/
/*
void PrivateWidget::change_accept_button()
{
   if (accept_sound[clientnr]==TRUE)
   {
      accept_sound[clientnr]=FALSE;
      if (language==ENGLISH) accept_button->setText ("No");
      if (language==DUTCH) accept_button->setText ("Nee");
   }
   else
   {
      accept_sound[clientnr]=TRUE;
      if (language==ENGLISH) accept_button->setText ("Yes");
      if (language==DUTCH) accept_button->setText ("Ja");
   }
}
*/
/* ----- Sound functions ----- */
void dump_snd_list()
	{
	int i=0;
	
	for(i=0; i<S_num_sounds; i++)
		{
		printf("snd %d: len = %d \n", i, S_sounds[i].len );
		}
	}
	
/*==========================================================================*/
int Snd_init( int num_snd, const Sample *sa, int frequency, 
              int channels, const char *dev )
	{
	int result;

	S_num_sounds     = num_snd;
	S_sounds         = sa;	/* array of sound samples*/
	S_playback_freq  = frequency;
	S_num_channels   = channels; 
	S_snddev= dev;	/* sound device, eg /dev/dsp*/
	
	if (S_sounds==NULL)
		return EXIT_FAILURE;
	
	result=Snd_init_dev();

	if (result==EXIT_SUCCESS)
		{
		sampleMixerStatus=1;
		}
	else
		{
		sampleMixerStatus=0;
		}

	return result;
	}

/*==========================================================================*/
int Snd_restore()
	{
	int result;

	if (!sampleMixerStatus)
		return EXIT_FAILURE;
	
	result=Snd_restore_dev();

	if (result==EXIT_SUCCESS)
		{
		sampleMixerStatus=0;
		}
	else
		{
		sampleMixerStatus=0;
		}

	return result;
	}

/*==========================================================================*/
/* volume control not implemented yet.*/
int Snd_effect( int sound_num, int channel )
	{
	if(! sampleMixerStatus || sound_on==FALSE)
		return EXIT_FAILURE;

//	if(S_sounds[sound_num].data != NULL)
//		{	
		write(S_fd_pipe[1], &sound_num, sizeof(sound_num));
		write(S_fd_pipe[1], &channel, sizeof(channel));
//		}
//	else
//		fprintf(stderr,"Referencing NULL sound entry\n");
	
	return EXIT_SUCCESS;
	}

/*============================================================================*/
int Snd_init_dev()
	{
	int whoami;
	S_fd_snddev = -1;

	S_son_pid = 0;


	if(access(S_snddev,W_OK) != 0)
		{	
		perror("No access to sound device");
		return EXIT_FAILURE;
		}

	S_fd_snddev = open(S_snddev,O_WRONLY);

	if(S_fd_snddev < 0)
		{	
		fprintf(stderr,"int_snddev: Cannot open sound device \n");
		return EXIT_FAILURE;
		}
		
	close(S_fd_snddev);

	if(pipe(S_fd_pipe) < 0)
		{	
		fprintf(stderr,"Cannot create pipe for sound control \n");
		return EXIT_FAILURE;
		}

	/* now setup 2nd process for writing the data... */
	if((whoami = fork()) < 0)
		{	
		fprintf(stderr,"Cannot fork sound driver\n");
		return EXIT_FAILURE;
		}
		
	if(whoami != 0)	/* successfully created son */
		{	
		close(S_fd_pipe[0]);	/* close end for reading */
		S_son_pid = whoami;
		return EXIT_SUCCESS;
		}
		
		/* Here is the code for the son... */
		{
		int sound_num,ch,i;
		struct timeval tval = {0L,0L};
		fd_set readfds,dsp;

		Mix mix;
		
		int frag, fragsize;

		Channel *chan = (Channel*)malloc( sizeof(Channel)*S_num_channels );

		for (i=0; i<S_num_channels; i++)
			Chan_reset( chan+i );
			
		S_fd_snddev = open(S_snddev,O_WRONLY );
		if(S_fd_snddev < 0)
			{	
			perror("Cannot open sound device: ");
//			exit(1);
			}

		frag = FRAG_SPEC; /*defined in soundIt.h */
		
 		ioctl(S_fd_snddev, SNDCTL_DSP_SETFRAGMENT, &frag);
 
		if ( ioctl(S_fd_snddev,SNDCTL_DSP_SPEED, &S_playback_freq)==-1 )
			perror("Sound driver ioctl ");

		fragsize=0;
		if ( ioctl(S_fd_snddev,SNDCTL_DSP_GETBLKSIZE, &fragsize)==-1 ) 
			perror("Sound driver ioctl ");
			
		/* printf("after: block size: %d \n",fragsize); */

		/* init mixer object*/
		Mix_alloc( &mix, fragsize );

		close(S_fd_pipe[1]);	/* close end for writing */

		FD_ZERO(&dsp); 
		FD_SET(S_fd_snddev, &dsp);
		
		FD_ZERO(&readfds); 
		FD_SET(S_fd_pipe[0], &readfds);
		
		printf("Sound driver initialized.\n");
		
		for(;;)
			{
			FD_SET(S_fd_pipe[0], &readfds);
			tval.tv_sec=0L;
			tval.tv_usec=0L;
			select(S_fd_pipe[0]+1, &readfds,NULL,NULL,&tval);

			if (FD_ISSET(S_fd_pipe[0], &readfds))
				{
				if (read(S_fd_pipe[0], &sound_num, sizeof(int))==0)
					break;

				read(S_fd_pipe[0], &ch, sizeof(int));

				/* printf("chan=%d snd=%d len=%d \n", ch, sound_num, S_sounds[sound_num].len ); */
				Chan_assign( &(chan[ch]), &(S_sounds[sound_num]) );
				}
			
			Chan_mixAll(&mix,chan);
			write(S_fd_snddev, mix.Vclippedbuf, fragsize );
			}

		Mix_dealloc( &mix );			
		printf("Sound process exiting..\n\n");
		close(S_fd_pipe[0]);
		close(S_fd_pipe[1]);
		exit (0);
		} /*end of child process */
	}


/*==========================================================================*/
int Snd_restore_dev()
	{
	close(S_fd_pipe[0]);
	close(S_fd_pipe[1]);
	
	/* wait for child process to die*/
	wait(NULL);
	return EXIT_SUCCESS;
	}

/*==========================================================================*/
/*   CHANNEL MIXING FUNCTIONS						    */
/*==========================================================================*/
void Chan_reset( Channel *chan )
    {
    chan->Vstart=NULL;
    chan->Vcurrent=NULL;
    chan->Vlen=0;
    chan->Vleft=0;
    }

/*==========================================================================*/
void Chan_assign( Channel *chan, const Sample *snd )
    {
    chan->Vstart  = snd->data;
    chan->Vcurrent= chan->Vstart;
    chan->Vlen    = snd->len;
    chan->Vleft   = snd->len;
    } 

/*==========================================================================*/
int Chan_copyIn( Channel *chan, Mix *mix )
    {
    int    i,*p = mix->Vunclipbuf, result, min;

    result = (chan->Vleft>0) ? 1 : 0;
    min = (chan->Vleft < mix->Vsize) ? chan->Vleft : mix->Vsize;

    for(i=0; i<min; i++)
        {
        *p++ = (int) *chan->Vcurrent++;
        }
    chan->Vleft -= i;

    /* fill the remaining (if any) part of the mix buffer with silence */
    while (i<mix->Vsize) 
            { 
            *p++ = 128; 
            i++; 
            }
    return result;            
    }

/*==========================================================================*/
int Chan_mixIn( Channel *chan, Mix *mix ) 
    {
    int    i,*p = mix->Vunclipbuf, result, min;

    result = (chan->Vleft>0) ? 1 : 0;
    min = (chan->Vleft < mix->Vsize) ? chan->Vleft : mix->Vsize;
    
    for(i=0; i<min; i++)
        {
        *p++ += (int) (*chan->Vcurrent++) - 128;
        }

    chan->Vleft -= i;
    return result;
    }

/*========================================================================*/
/* clip an int to a value between 0 and 255 */
static inline 
unsigned char clip(int i)
    {
    return (i<0) ? 0 : ( (i>255) ? 255 : i );
    }
    
/*==========================================================================*/
int Chan_finalMixIn( Channel *chan, Mix *mix )
    {
    register int    i;
    int   *p = mix->Vunclipbuf, result, min;
    unsigned char *final = mix->Vclippedbuf;

    result = (chan->Vleft>0) ? 1 : 0;
    min = (chan->Vleft < mix->Vsize) ? chan->Vleft : mix->Vsize;
    
    for(i=0; i<min; i++)
        {
        *p += (int) (*chan->Vcurrent++) - 128;
        *final++ = clip(*p++);
        }
    chan->Vleft -= i;

    /* copy rest of Vunclipbuf over to Vclippedbuf */
    while (i<mix->Vsize) 
            {
            *final++ = clip(*p++);
            i++;
            }
            
    return result;
    }


/*==========================================================================*/
void Mix_alloc(Mix *mix, int size)
    {
    mix->Vclippedbuf = (unsigned char *)calloc( sizeof(char), size);
    mix->Vunclipbuf = (int *)calloc( sizeof(int), size);
    mix->Vsize  = size;
    
    if ((mix->Vclippedbuf==NULL)||(mix->Vunclipbuf==NULL))
    	{
   	fprintf(stderr,"Unable to allocate memory for mixer buffer\n");
//    	exit(-1);
    	}
    }

/*==========================================================================*/
void Mix_dealloc( Mix *mix)
    { 
    if (mix->Vclippedbuf) free(mix->Vclippedbuf); 
    if (mix->Vunclipbuf) free(mix->Vunclipbuf); 
    }

/*==========================================================================*/
/* Mixes together the channels into one sound.
   Returns # of channels currently playing *any* sound
   Therefore, return 0 means to channels have a sample, therefore no
   sound is playing
*/ 
int Chan_mixAll( Mix *mix, Channel *chan )
    {
    int result  = 0,i=0;
    
    result  = Chan_copyIn( chan,  mix);

    /* we want to loop for S_num_channels-2 */
    for(i=2;i<S_num_channels;i++)
    	result += Chan_mixIn( ++chan, mix);
    	
    result += Chan_finalMixIn( ++chan, mix);
    
    return result;
    }

/*==========================================================================*/
/* given the name of a .raw sound file, load it into the Sample struct */ 
/* pointed to by 'sample'                                              */
/* Returns -1 couldn't open/read file				       */
/*         -2 couldn't alloc memory)                                   */
int
Snd_loadRawSample( const char *file, Sample *sample )
   {
   FILE *fp;

   sample->data = NULL;
   sample->len  = 0;
   
   fp = fopen(file,"r");   
   
   if (fp==NULL) return -1;
   
   /* get length of the file */
   sample->len = lseek( fileno(fp), 0, SEEK_END );
   
   /* go back to beginning of file */
   lseek( fileno(fp), 0, SEEK_SET );

   /* alloc memory for sample */
   sample->data = (unsigned char *)malloc( sample->len );
   
   if (sample->data==NULL)
   	{
   	fclose(fp);
   	return -2;
   	}
   
   fread( sample->data, 1, sample->len, fp ); 	
  
   fclose(fp);
   
   return 0;
   }


void InitSound()
{
   chanel=0;
   Sample	snd[NUM_SAMPLES];
   char	ch;
      
   Snd_loadRawSample( "error.raw", &snd[0] );
   Snd_loadRawSample( "hcapture.raw", &snd[1] );
   Snd_loadRawSample( "holyvisn.raw", &snd[2] );
   Snd_loadRawSample( "seal.raw", &snd[3] );
   Snd_loadRawSample( "dkpissd2.raw", &snd[4] );
   Snd_loadRawSample( "dkpissd3.raw", &snd[5] );
   Snd_loadRawSample( "dkwhat2.raw", &snd[6] );
   Snd_loadRawSample( "dwhat1.raw", &snd[7] );
   Snd_loadRawSample( "gnpissd3.raw", &snd[9] );
   Snd_loadRawSample( "knpissd3.raw", &snd[8] );
   Snd_loadRawSample( "ofarm.raw", &snd[10] );
   Snd_loadRawSample( "ogpissd5.raw", &snd[11] );
   Snd_loadRawSample( "omwhat4.raw", &snd[12] );
   Snd_loadRawSample( "opissed1.raw", &snd[13] );
   Snd_loadRawSample( "opissed5.raw", &snd[14] );
   Snd_loadRawSample( "opissed7.raw", &snd[15] );
   Snd_loadRawSample( "owhat4.raw", &snd[16] );
   Snd_loadRawSample( "pspissd7.raw", &snd[17] );
   Snd_loadRawSample( "message.raw", &snd[18] );
  
  
   Snd_init(NUM_SAMPLES, snd, SAMPLE_RATE, NUM_CHANNELS, "/dev/dsp" );
//   {
//      printf("Can't init soundIt library. yech..\n");
//      exit(0);
//   }

}

void play_sound(int sound_nr)
{
   int temp;
   if (chanel>NUM_CHANNELS) chanel=0;
   temp=Snd_effect( sound_nr, chanel++ );
   if (temp==EXIT_FAILURE)
      put_text("INTERNAL: Sound not played.");
   if (temp==EXIT_SUCCESS)
      put_text("INTERNAL: Sound played.");

}
/* ----- End of Sound functions ----- */
#include "voodoo.moc"	// damn... this include _has_ to be down here...

int main (int argc, char **argv)
{
   int index;
   bool ok;
   private_on=TRUE;
   language=ENGLISH;
   english_on=TRUE;
   sound_on=TRUE;
   splash_on=TRUE;
   nick = "VDC_user";
   InitNotifyList(); InitClientList(); InitUser(); InitSound();

   for (index=0;index<=MAXCONNS;++index)
      window_on[index]=FALSE;
   for (index=0;index<=MAXCONNS;++index)
      accept_sound[index]=TRUE;

   QApplication chat(argc, argv);
   QApplication::setFont(QFont("helvetica") );
   QApplication::setStyle(GUIStyle(WindowsStyle));
   read_config();
   chatwidget = new ChatWidget(0, "Chat Widget");
   chat.setMainWidget(chatwidget);
   if (language==ENGLISH) chatwidget->setCaption("VooDooChat version 1.02 [EN]");
   else chatwidget->setCaption("VooDooChat versie 1.02 [NL]");
   connectwidget->setCaption("Connect");
 //  SplashWidget splash(0, "Splash");
    splash=new QLabel(0,"mooi",WStyle_Customize | WStyle_NoBorder | WStyle_Tool); 
   splash->setPixmap("voodoo.xpm");
   splash->setAutoResize(&ok);
   if (splash_on==TRUE) splash->show();
   chatwidget->show();
   chatwidget->iconify();
   
   return chat.exec();
}

