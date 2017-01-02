/*
 * twxmms2.c    2016-01-02
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#include <xmmsclient/xmmsclient.h>

#include <Tw/Tw.h>
#include <Tw/Twerrno.h>


static xmmsc_connection_t *conn;
static char * server_name;
static int connected; 

static int titleXPosition, titleScrollSpeed = 1, titleScrollMode, titleWidth;
static int mywidth, volume, trackTime, currentTime, playlistpos, displayElapsedTime;  

static char title[1024];

static tmsgport xmms_MsgPort;
static tmenu    xmms_Menu;
static twindow  xmms_Win;

#define G_BACK   1
#define G_PLAY   2
#define G_PAUSE  3
#define G_STOP   4
#define G_SKIP   5
#define G_EJECT  6
#define G_CLOSE  7
#define G_KILL   8


static void libtw_error(uldat err, uldat err_detail) {
    fprintf(stderr, "twxmms2: libTw error: %s%s\n", TwStrError(err), TwStrErrorDetail(err, err_detail));
}

static void libxmms_error(int err) {
    fprintf(stderr, "twxmms2: libxmmsclient error %d: %s\n", err, conn ? xmmsc_get_last_error(conn) : "xmmsc_init() failed");
}

static int connectXmms(void);

TW_DECL_MAGIC(twxmms2_magic);

static int init_gui(int argc, char **argv) {
    int borders = 0;
    twindow Window;
    
#ifdef HAVE_GETENV
    server_name = getenv("XMMS_PATH");
#endif

    while (--argc && *++argv) {
	if (!strcmp(*argv, "-with-border"))
	    borders = 0;
	else if (!strcmp(*argv, "-no-border"))
	    borders = TW_WINDOWFL_BORDERLESS;
        else
            server_name = *argv;
    }
    
    if (TwCheckMagic(twxmms2_magic) && TwOpen(NULL) &&
	(xmms_MsgPort = TwCreateMsgPort(4, "twxmms2")) &&
	(xmms_Menu = TwCreateMenu
	 (COL(BLACK,WHITE), COL(BLACK,GREEN), COL(HIGH|BLACK,WHITE), COL(HIGH|BLACK,BLACK),
	  COL(RED,WHITE), COL(RED,GREEN), (byte)0)) &&
	(TwInfo4Menu(xmms_Menu, TW_ROW_ACTIVE, 19, " Twin Xmms2 Client ", "ptpppptppppptpppppp"), TRUE) &&
	(Window=TwWin4Menu(xmms_Menu)) &&
	TwItem4Menu(xmms_Menu, Window, TRUE, 6, " File ") &&
	(/* TwRow4Menu(Window, G_EJECT, TW_ROW_ACTIVE,  6, " Open "), */
	 TwRow4Menu(Window, G_CLOSE, TW_ROW_ACTIVE, 14, " Close Client "),
/*	 TwRow4Menu(Window, G_KILL,  TW_ROW_ACTIVE, 12, " Quit Xmms2 "), */ /* quitting xmms gives SEGFAULT... better luck with xmms2 ? */
	 TRUE) &&
	TwItem4MenuCommon(xmms_Menu) &&
	(xmms_Win = TwCreateWindow
	 (7, "twxmms2", NULL,
	  xmms_Menu, COL(HIGH|YELLOW,HIGH|BLACK), TW_NOCURSOR,
	  TW_WINDOW_DRAG|TW_WINDOW_CLOSE|TW_WINDOW_WANT_MOUSE|TW_WINDOW_AUTO_KEYS,
	  TW_WINDOWFL_USEROWS|borders,
	  mywidth = 20, 4, 0)) &&
        /*
	TwCreateButtonGadget
	(xmms_Win, 2, 1, " \x17", 0, G_EJECT,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 16, 2) &&
         */
	TwCreateButtonGadget
	(xmms_Win, 2, 1, "\x10|", 0, G_SKIP,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 15, 2) &&
	TwCreateButtonGadget
	(xmms_Win, 2, 1, "\xDE\xDD", 0, G_STOP,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 11, 2) &&
	TwCreateButtonGadget
	(xmms_Win, 2, 1, "\xFE\xFE", 0, G_PAUSE,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 8, 2) &&
	TwCreateButtonGadget
	(xmms_Win, 2, 1, " \x10", 0, G_PLAY,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 5, 2) &&
	TwCreateButtonGadget
	(xmms_Win, 2, 1, "|\x11", 0, G_BACK,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 1, 2)
	) {
	
	TwMapWindow(xmms_Win, TwFirstScreen());
	TwFlush();

        connectXmms();
        return TRUE;
    }
    return FALSE;
}

/*
 * paint the title
 */
static void paint_title(void) {
    byte buf[20];
    int len;
    
    TwGotoXYWindow(xmms_Win, 0, 0);

    memset(buf, ' ', 20);

    if (connected && *title) {
	if (titleXPosition > 0) {
	    if ((len = 20 - titleXPosition) > 0) {
		if (len > titleWidth)
		    len = titleWidth;
		memcpy(buf + titleXPosition, title, len);
	    }
	} else {
	    if ((len = titleWidth + titleXPosition) > 20)
		len = 20;
	    if (len > 0)
		memcpy(buf, title - titleXPosition, len);
	}
    }
    TwSetColTextWindow(xmms_Win, COL(HIGH|YELLOW,HIGH|BLACK));
    TwWriteAsciiWindow(xmms_Win, 20, buf);
}
    
/*
 * paint the title, track time, volume.
 */
static void paint(void) {
    hwcol col;
    byte buf[24];
    
    paint_title();

    TwGotoXYWindow(xmms_Win, 0, 1);
    if (connected) {
	int cTime = currentTime / 1000;
	int tTime = trackTime / 1000;

	sprintf(buf, "\x11%02d:%02d/%02d:%02d\x10 \x1F%03d%%\x1E",
		cTime/60, cTime%60, tTime/60, tTime%60, volume);
    } else {
	memset(buf, ' ', 20);
    }
    TwSetColTextWindow(xmms_Win, COL(HIGH|WHITE,HIGH|BLACK));
    TwWriteAsciiWindow(xmms_Win, 14, buf);
    if (volume < 50)
	col = COL(HIGH|CYAN,HIGH|BLACK);
    else if (volume < 75)
	col = COL(HIGH|GREEN,HIGH|BLACK);
    else if (volume < 90)
	col = COL(HIGH|YELLOW,HIGH|BLACK);
    else
	col = COL(HIGH|RED,HIGH|BLACK);
    TwSetColTextWindow(xmms_Win, col);
    TwWriteAsciiWindow(xmms_Win, 6, buf+14);
}

/*
 * scroll the title
 */
static void scroll(void) {
    
    if (titleWidth < mywidth) {
	/*
	 * don't scroll if title too small
	 */
	titleXPosition = (int) (mywidth / 2 - titleWidth / 2);
    } else {
	titleXPosition -= titleScrollSpeed;
    
	if (titleScrollMode == 0) {
	    /*
	     * normal scrolling
	     */
	    if (titleScrollSpeed > 0) {
		if (titleXPosition < -titleWidth)
		    titleXPosition = mywidth;
	    } else {
		if (titleXPosition > mywidth)
		    titleXPosition = -titleWidth;
	    }
	} else if (titleScrollMode == 1) {
	    /*
	     * ping pong scrolling
	     */
	    if (titleScrollSpeed > 0) {
		if (titleXPosition < -titleWidth + mywidth - 15)
		    titleScrollSpeed = -titleScrollSpeed;
	    } else {
		if (titleXPosition > 15) 
		    titleScrollSpeed = -titleScrollSpeed;
	    }
	}
    }
}


static int32_t to_int(xmmsc_result_t * res) {
    int32_t n = 0;
    if (res != NULL) {
        xmmsc_result_wait(res);
        if (xmmsc_result_get_class(res) == XMMSC_RESULT_CLASS_DEFAULT) {
            xmmsv_t * val = xmmsc_result_get_value(res);
            if (val != NULL) {
                xmmsv_type_t type = xmmsv_get_type(val);
                switch (type) {
                case XMMSV_TYPE_INT32:
                    {
                        xmmsv_get_int(val, &n);
                        break;
                    }
                case XMMSV_TYPE_ERROR:
                    {
                        const char * err_txt = NULL; 
                        xmmsv_get_error(val, & err_txt);
                        if (err_txt != NULL)
                            ; /* fprintf(stderr, "twxmms2: libxmmsclient error: %s\n", err_txt); */
                    }
                    break;
                }
            }
        }
        xmmsc_result_unref(res);
    }
    return n;
}
    
void load_track_info(void)
{
    const char * track_title = NULL, * track_artist = NULL;
    size_t track_title_len = 0, track_artist_len = 0;
    int32_t id = 0, duration = 0;

    xmmsc_result_t * res = xmmsc_medialib_get_info(conn, playlistpos);

    if (res != NULL) {
        xmmsc_result_wait(res);
        if (xmmsc_result_get_class(res) == XMMSC_RESULT_CLASS_DEFAULT) {
            xmmsv_t * val = xmmsc_result_get_value(res);
            if (val != NULL) {
                val = xmmsv_propdict_to_dict(val, NULL); // must call xmmsv_unref() later

                xmmsv_dict_entry_get_string(val, "artist", &track_artist);
                xmmsv_dict_entry_get_string(val, "title", &track_title);
                xmmsv_dict_entry_get_int(val, "duration", &duration);
                xmmsv_dict_entry_get_int(val, "id", &id);
                
                track_artist_len = track_artist ? strlen(track_artist) : 0;
                track_title_len = track_title ? strlen(track_title) : 0;
                
                if (track_artist_len) {
                    if (track_artist_len >= sizeof(title)/2)
                        track_artist_len = sizeof(title)/2 - 1;
                    
                    if (track_artist_len)
                        memcpy(title, track_artist, track_artist_len);
                }
                if (track_artist_len && track_title_len) {
                    memcpy(title + track_artist_len, " - ", 3);
                    track_artist_len += 3;
                }
                if (track_title_len) {
                    if (track_title_len >= sizeof(title) - track_artist_len)
                        track_title_len = sizeof(title) - track_artist_len - 1;
                    
                    if (track_title_len)
                        memcpy(title + track_artist_len, track_title, track_title_len);
                }
                if (track_artist_len || track_title_len) {
                    title[track_artist_len + track_title_len] = '\0';
                    titleWidth = track_artist_len + track_title_len;
                }
                xmmsv_unref(val);
            }
        }
        xmmsc_result_unref(res);
    }
    trackTime = duration; /* duration/1000=track length in seconds */
    if (!track_artist_len && !track_title_len)
        memcpy(title, "<<unknown>>", (titleWidth = 11) + 1 /* for final '\0' */);
}

/* get values from XMMS2 */
static void receive(void) {
    if (connected) {
	playlistpos = to_int(xmmsc_playback_current_id(conn));
        volume      = to_int(xmmsc_playback_volume_get(conn));
	currentTime = to_int(xmmsc_playback_playtime(conn));
        load_track_info();
    }
}


/* start XMMS2 */
static int startXmms(void) {
    pid_t pid = fork();
    int wstatus = -1;
    switch (pid) {
    
    case 0:
	execlp("xmms2-launcher", "xmms2-launcher", NULL);
	exit(1);
	break;
    
    case -1:
	break;

    default:
#ifdef HAVE_WAITPID
	if (waitpid(pid, &wstatus, 0) != pid)
            wstatus = -1;
#else
# if defined(HAVE_NANOSLEEP)
        {
            struct timespec delay = { 0, 100000000 };
            nanosleep(&delay, NULL);
        }
# elif defined(HAVE_SLEEP)
        sleep(1);
# endif
        wstatus = 0;
#endif
	break;
    }
    return wstatus;
}

/* connect to XMMS2, starting it if needed */
static int connectXmms(void) {
    int err = -1;
    
    if (conn || (conn = xmmsc_init("twxmms2"))) {
        if (connected) {
            err = 0;
        } else {
            err = !xmmsc_connect(conn, server_name);  
            if (err != 0) {
                /* start XMMS2 and try again... */
                if ((err = startXmms()) == 0)
                    err = !xmmsc_connect(conn, server_name);
            }
        }
    }
    connected = err == 0;
    if (connected)
        receive();
    else
        libxmms_error(err);
    return err;
}

static void playlist_jump(int delta) {
    xmmsc_result_t *res = xmmsc_playlist_set_next_rel(conn, delta);
    if (res != NULL) {
        xmmsc_result_wait(res);
        if (xmmsc_result_get_class(res) == XMMSC_RESULT_CLASS_DEFAULT) {
            xmmsv_t * val = xmmsc_result_get_value(res);
            const char * err_txt = NULL;
            if (xmmsv_get_error(val, &err_txt)) {
		fprintf(stderr, "Failed to advance in playlist: %s\n", err_txt ? err_txt : "unknown error");
            } else {
		xmmsc_result_t *res2 = xmmsc_playback_tickle(conn);
                xmmsc_result_wait(res2);
                xmmsc_result_unref(res2);
            }
        }
        xmmsc_result_unref(res);
    }
}

static void event_menu(tevent_menu event) {
    switch (event->Code) {
      case G_EJECT:
	break;
      case G_CLOSE:
	exit(0);
	break;
      case G_KILL:
	if (connected) {
	    xmmsc_quit(conn);
	}
	break;
      default:
	break;
    }
}

/*
 * mouse handling
 *
 */
static void event_gadget(tevent_gadget event) {
    xmmsc_result_t *res = NULL;
    
    if (!connected && connectXmms() != 0)
        return;
    
    switch (event->Code) {
      case G_BACK:
        playlist_jump(-1);
	break;
      case G_PLAY:
        res = xmmsc_playback_start(conn);
	break;
      case G_PAUSE:
	res = xmmsc_playback_pause(conn);
	break;
      case G_STOP:
        res = xmmsc_playback_stop(conn);
	break;
      case G_SKIP:
        playlist_jump(1);
	break;
      case G_EJECT:
	break;
      default:
	break;
    }
    if (res != NULL) {
        xmmsc_result_wait(res);
        xmmsc_result_unref(res);
    }
}

static void event_mouse(tevent_mouse event) {
    xmmsc_result_t *res = NULL;
    
    if (!connected && connectXmms() != 0)
	return;

    if (event->Code == PRESS_LEFT && event->W == xmms_Win && event->Y == 1) {
	/*
	 * left mouse button: 
	 *   - set volume
	 *   - jump to track time
	 */
	if (event->X <= 12) {
	    if (event->X <= 5) {
		if ((currentTime -= trackTime / 20) < 0)
		    currentTime = 0;
	    } else {
		if ((currentTime += trackTime / 20) > trackTime)
		    currentTime = trackTime;
	    }
            res = xmmsc_playback_seek_ms(conn, currentTime, XMMS_PLAYBACK_SEEK_SET);
            xmmsc_result_unref(res);

	} else if (event->X >= 14) {
	    if (event->X <= 16) {
		volume -= 2;
	    } else {
		volume += 2;
	    }
            res = xmmsc_playback_volume_set(conn, "left", volume);
            xmmsc_result_unref(res);
            res = xmmsc_playback_volume_set(conn, "right", volume);
            xmmsc_result_unref(res);
	}
    } else if (event->Code == PRESS_RIGHT && event->W == xmms_Win) {
	/*
	 * right mouse button: 
	 *   - switch between displaying elapsed time / displaying time to play
	 *   - switch between title scrolltype
	 */
	if (event->Y == 1 && event->X <= 12)
	    displayElapsedTime ^= 1;
	else if (event->Y == 0)
	    titleScrollMode ^= 1;
    } else if (event->Code == PRESS_MIDDLE && event->W == xmms_Win) {
	/*
	 * middle mouse button:
	 *   - open fileselector
	 */
	// xmms_remote_eject(0);
    }
}

static void inc_timeval(struct timeval *t1, struct timeval *t2) {
    t1->tv_usec += t2->tv_usec;
    while (t1->tv_usec >= 1000000) {
	t1->tv_usec -= 1000000;
	t1->tv_sec++;
    }
    t1->tv_sec += t2->tv_sec;
}

static void dec_timeval(struct timeval *t1, struct timeval *t2) {
    t1->tv_usec -= t2->tv_usec;
    while (t1->tv_usec < 0) {
	t1->tv_usec += 1000000;
	t1->tv_sec--;
    }
    t1->tv_sec -= t2->tv_sec;
}

static int cmp_timeval(struct timeval *t1, struct timeval *t2) {
    int s = t1->tv_sec - t2->tv_sec;
    return s ? s : t1->tv_usec - t2->tv_usec;
}

static void add_timeval(struct timeval *t1, struct timeval *t2, struct timeval *t3) {
    *t1 = *t2;
    inc_timeval(t1, t3);
}

static void sub_timeval(struct timeval *t1, struct timeval *t2, struct timeval *t3) {
    *t1 = *t2;
    dec_timeval(t1, t3);
}

struct timeval now, t_receive;

static void main_loop(void) {
    tmsg Msg;
    uldat err;
    int fd = TwConnectionFd();
    struct timeval t, delta = { 0, 200000 };
    fd_set fset;
    
    FD_ZERO(&fset);
    
    gettimeofday(&now, NULL);
    add_timeval(&t_receive, &now, &delta);
    receive(); scroll(); paint();

    while (!TwInPanic()) {
	while ((Msg = TwReadMsg(FALSE))) {
	
	    if (Msg->Type == TW_MSG_WIDGET_GADGET && Msg->Event.EventGadget.Code == 0)
		return;
	    
	    switch (Msg->Type) {
	      case TW_MSG_WIDGET_MOUSE:
		event_mouse(&Msg->Event.EventMouse);
		break;
	      case TW_MSG_WIDGET_GADGET:
		event_gadget(&Msg->Event.EventGadget);
		break;
	      case TW_MSG_MENU_ROW:
		event_menu(&Msg->Event.EventMenu);
		break;
	      default:
		break;
	    }
	}
	gettimeofday(&now, NULL);
	while (cmp_timeval(&now, &t_receive) >= 0) {
	    inc_timeval(&t_receive, &delta);
	    receive(); scroll(); paint();
	}
	sub_timeval(&t, &t_receive, &now);
	TwFlush();
	
	FD_SET(fd, &fset);
	select(fd+1, &fset, NULL, NULL, &t);
    }

    if ((err = TwErrno))
	libtw_error(err, TwErrnoDetail);
}

int main(int argc, char **argv) {
    if (init_gui(argc, argv))
	main_loop();
    
    return 0;
}
