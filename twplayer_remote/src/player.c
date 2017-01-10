/*
 * player.c    2016-01-06
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

#include <Tw/Tw.h>
#include <Tw/Twerrno.h>

#include "remote_driver.h"

static int trackXPosition, trackScrollSpeed = 1, trackScrollMode, trackLen;
static int myLen, displayElapsedTime;  
static long volume, trackTime, currentTime;

static char track[1024];

static tmsgport mport;
static tmenu    menu;
static twindow  win, win_remote;

static int connect_remote(unsigned index);
static void disconnect_remote(void);
static void scan_remote(void);

#define G_BACK   1
#define G_PLAY   2
#define G_PAUSE  3
#define G_STOP   4
#define G_NEXT   5
#define G_EJECT  6
#define G_CLOSE  7
#define G_KILL   8
#define G_RESCAN 9
#define G_REMOTE_FIRST 10

static void libtw_error(uldat err, uldat err_detail) {
    fprintf(stderr, "twplayer_remote: libTw error: %s%s\n", TwStrError(err), TwStrErrorDetail(err, err_detail));
}

static pl_driver pl = NULL;
static pl_factories factories = NULL;

TW_DECL_MAGIC(player_magic);

static int init_gui(int argc, char **argv) {
    int borders = 0;
    twindow Window;
    
    while (--argc && *++argv) {
	if (!strcmp(*argv, "-with-border"))
	    borders = 0;
	else if (!strcmp(*argv, "-no-border"))
	    borders = TW_WINDOWFL_BORDERLESS;
    }
    
    if (TwCheckMagic(player_magic) && TwOpen(NULL) &&
	(mport = TwCreateMsgPort(15, "twplayer_remote")) &&
	
        (menu = TwCreateMenu
	 (COL(BLACK,WHITE), COL(BLACK,GREEN), COL(HIGH|BLACK,WHITE), COL(HIGH|BLACK,BLACK),
	  COL(RED,WHITE), COL(RED,GREEN), (byte)0)) &&
	(TwInfo4Menu(menu, TW_ROW_ACTIVE, 20, " Twin Remote Player ", "ptpppptpppppptpppppp"), TRUE) &&

	(Window=TwWin4Menu(menu)) &&
	TwItem4Menu(menu, Window, TRUE, 6, " File ") &&
	(TwRow4Menu(Window, G_EJECT, TW_ROW_ACTIVE,  6, " Open "),
	 TwRow4Menu(Window, G_CLOSE, TW_ROW_ACTIVE, 13, " Quit Client "),
/*	 TwRow4Menu(Window, G_KILL,  TW_ROW_ACTIVE, 13, " Quit Player "), */ /* quitting xmms gives SEGFAULT... better luck with xmms2/mpris ? */
	 TRUE) &&
	
        (win_remote = Window = TwWin4Menu(menu)) &&
	TwItem4Menu(menu, Window, TRUE, 15, " Media Players ") &&
	(TwRow4Menu(Window, G_RESCAN, TW_ROW_ACTIVE,  8, " Rescan "),
	 TwRow4Menu(Window, (udat)0,  TW_ROW_IGNORE, 14, "\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4"),
	 TRUE) &&
        
	TwItem4MenuCommon(menu) &&
        
	(win = TwCreateWindow
	 (15, "twplayer-remote", NULL,
	  menu, COL(HIGH|YELLOW,HIGH|BLACK), TW_NOCURSOR,
	  TW_WINDOW_DRAG|TW_WINDOW_CLOSE|TW_WINDOW_WANT_MOUSE|TW_WINDOW_AUTO_KEYS,
	  TW_WINDOWFL_USEROWS|borders,
	  myLen = 20, 4, 0)) &&
	TwCreateButtonGadget
	(win, 2, 1, "\x0E!", 0, G_RESCAN,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 16, 2) &&
	TwCreateButtonGadget
	(win, 2, 1, "\x10|", 0, G_NEXT,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 13, 2) &&
	TwCreateButtonGadget
	(win, 2, 1, "\xDB\xDB", 0, G_STOP,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 10, 2) &&
	TwCreateButtonGadget
	(win, 2, 1, "||", 0, G_PAUSE,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 7, 2) &&
	TwCreateButtonGadget
	(win, 2, 1, " \x10", 0, G_PLAY,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 4, 2) &&
	TwCreateButtonGadget
	(win, 2, 1, "|\x11", 0, G_BACK,
	 COL(CYAN,HIGH|BLACK), COL(HIGH|WHITE,GREEN), COL(HIGH|BLACK,GREEN),
	 1, 2)
	) {
	
	TwMapWindow(win, TwFirstScreen());
	TwFlush();

        return 0;
    }
    return -1;
}

static void update_window_title(void) {
    byte title[20] = "twplayer-remote";
    size_t len = 15;
    if (pl && pl->label) {
        len = min2(strlen(pl->label), sizeof(title) - 9);
        if (len)
            memcpy(title + 9, pl->label, len);
        len += 9;
    }
    TwSetTitleWindow(win, (udat)len, title);
}

static void clear_win_remote(void) {
    tmenuitem menuitem;
    unsigned i, n;
    if (!factories)
        return;
    for (i = 0, n = factories->size; i < n; i++) {
        menuitem = TwFindRowByCodeWindow(win_remote, G_REMOTE_FIRST + i);
        if (menuitem != TW_NOID)
            TwDeleteMenuItem(menuitem);
    }
    TwFlush();
}

static void fill_win_remote(void) {
    char label[40];
    pl_factory factory;
    tmenuitem menuitem;
    unsigned i, n;
    size_t len;
    
    if (!factories)
        return;
    for (i = 0, n = factories->size; i < n; i++) {
        factory = factories->data[i];
        if (factory->label != NULL) {
            len = min2(strlen(factory->label), sizeof(label) - 2);
            memcpy(label + 1, factory->label, len);
            label[0] = label[len+1] = ' ';
            
            TwRow4Menu(win_remote, G_REMOTE_FIRST + i,
                       TW_ROW_ACTIVE, len + 2, label);
        }
    }
}

/*
 * paint the track name
 */
static void paint_track(void) {
    byte buf[20];
    int len;
    
    TwGotoXYWindow(win, 0, 0);

    memset(buf, ' ', 20);

    if (pl && *track) {
	if (trackXPosition > 0) {
	    if ((len = 20 - trackXPosition) > 0) {
		if (len > trackLen)
		    len = trackLen;
		memcpy(buf + trackXPosition, track, len);
	    }
	} else {
	    if ((len = trackLen + trackXPosition) > 20)
		len = 20;
	    if (len > 0)
		memcpy(buf, track - trackXPosition, len);
	}
    }
    TwSetColTextWindow(win, COL(HIGH|YELLOW,HIGH|BLACK));
    TwWriteAsciiWindow(win, 20, buf);
}
    
/*
 * paint the track name, track time, volume.
 */
static void paint(void) {
    hwcol col;
    byte buf[24];
    
    paint_track();

    TwGotoXYWindow(win, 0, 1);
    if (pl) {
	int cTime = currentTime / 1000;
	int tTime = trackTime / 1000;

	sprintf(buf, "\x11%02d:%02d/%02d:%02d\x10 \x1F%03ld%%\x1E",
		cTime/60, cTime%60, tTime/60, tTime%60, volume);
    } else {
	memset(buf, ' ', 20);
    }
    TwSetColTextWindow(win, COL(HIGH|WHITE,HIGH|BLACK));
    TwWriteAsciiWindow(win, 14, buf);
    if (volume < 50)
	col = COL(HIGH|CYAN,HIGH|BLACK);
    else if (volume < 75)
	col = COL(HIGH|GREEN,HIGH|BLACK);
    else if (volume < 90)
	col = COL(HIGH|YELLOW,HIGH|BLACK);
    else
	col = COL(HIGH|RED,HIGH|BLACK);
    TwSetColTextWindow(win, col);
    TwWriteAsciiWindow(win, 6, buf+14);
}

/*
 * scroll the track
 */
static void scroll(void) {
    
    if (trackLen < myLen) {
	/*
	 * don't scroll if track too small
	 */
	trackXPosition = (int) (myLen / 2 - trackLen / 2);
    } else {
	trackXPosition -= trackScrollSpeed;
    
	if (trackScrollMode == 0) {
	    /*
	     * normal scrolling
	     */
	    if (trackScrollSpeed > 0) {
		if (trackXPosition < -trackLen)
		    trackXPosition = myLen;
	    } else {
		if (trackXPosition > myLen)
		    trackXPosition = -trackLen;
	    }
	} else if (trackScrollMode == 1) {
	    /*
	     * ping pong scrolling
	     */
	    if (trackScrollSpeed > 0) {
		if (trackXPosition < -trackLen + myLen - 15)
		    trackScrollSpeed = -trackScrollSpeed;
	    } else {
		if (trackXPosition > 15) 
		    trackScrollSpeed = -trackScrollSpeed;
	    }
	}
    }
}


void load_track_info(void)
{
#if 1
    struct pl_trackinfo_s info = { 0, 0, 0, "\0", "\0" };
    size_t len, len2;
    if (pl)
        pl->get_trackinfo(pl, &info);

    len = info.artist_len;
    len2 = info.title_len;
    
    if (len) {
        if (len >= sizeof(track)/2)
            len = sizeof(track)/2 - 1;
                    
        if (len)
            memcpy(track, info.artist, len);
    }
    if (len && len2) {
        memcpy(track + len, " - ", 3);
        len += 3;
    }
    if (len2) {
        if (len2 >= sizeof(track) - len)
            len2 = sizeof(track) - len - 1;
                    
        if (len2) {
            memcpy(track + len, info.title, len2);
            len += len2;
        }
    }
    if (!len)
        memcpy(track, "<<unknown>>", (len = 11));

    track[len] = '\0';
    trackLen = len;
    trackTime = info.duration;
#else    
    const char * info_title = NULL, * info_artist = NULL;
    size_t info_title_len = 0, info_artist_len = 0;
    int32_t id = 0, duration = 0;

    long current_info_id = to_int(xmmsc_playback_current_id(conn));
    
    xmmsc_result_t * res = xmmsc_medialib_get_info(conn, current_info_id);

    if (res != NULL) {
        xmmsc_result_wait(res);
        if (xmmsc_result_get_class(res) == XMMSC_RESULT_CLASS_DEFAULT) {
            xmmsv_t * val = xmmsc_result_get_value(res);
            if (val != NULL) {
                val = xmmsv_propdict_to_dict(val, NULL); // must call xmmsv_unref() later

                xmmsv_dict_entry_get_string(val, "artist", &info_artist);
                xmmsv_dict_entry_get_string(val, "title", &info_title);
                xmmsv_dict_entry_get_int(val, "duration", &duration);
                xmmsv_dict_entry_get_int(val, "id", &id);
                
                info_artist_len = info_artist ? strlen(info_artist) : 0;
                info_title_len = info_title ? strlen(info_title) : 0;
                
                if (info_artist_len) {
                    if (info_artist_len >= sizeof(title)/2)
                        info_artist_len = sizeof(title)/2 - 1;
                    
                    if (info_artist_len)
                        memcpy(title, info_artist, info_artist_len);
                }
                if (info_artist_len && info_title_len) {
                    memcpy(title + info_artist_len, " - ", 3);
                    info_artist_len += 3;
                }
                if (info_title_len) {
                    if (info_title_len >= sizeof(title) - info_artist_len)
                        info_title_len = sizeof(title) - info_artist_len - 1;
                    
                    if (info_title_len)
                        memcpy(title + info_artist_len, info_title, info_title_len);
                }
                if (info_artist_len || info_title_len) {
                    title[info_artist_len + info_title_len] = '\0';
                    trackLen = info_artist_len + info_title_len;
                }
                xmmsv_unref(val);
            }
        }
        xmmsc_result_unref(res);
    }
    trackTime = duration; /* duration/1000=track length in seconds */
    if (!info_artist_len && !info_title_len)
        memcpy(title, "<<unknown>>", (trackLen = 11) + 1 /* for final '\0' */);
#endif
}

/* get values from XMMS2 */
static void receive(void) {
    if (pl) {
#if 1
        volume      = pl->get_volume(pl);
	currentTime = pl->get_position(pl);
#else
        volume      = to_int(xmmsc_playback_volume_get(conn));
	currentTime = to_int(xmmsc_playback_playtime(conn));
#endif
        load_track_info();
        
        if (pl->fatal_error) {
            scan_remote();
        }
    }
}

static int connect_remote(unsigned index) {
    if (!pl && factories && index < factories->size) {
        pl = call_factory(factories->data[index]);
        update_window_title();
    }

    if (pl)
        receive();
    
    return pl ? 0 : -1;
}

static void disconnect_remote(void) {
    if (pl) {
        pl->del(pl);
        pl = NULL;
        update_window_title();
    }
}

static void scan_remote(void) {
    disconnect_remote();
    if (factories || (factories = new_factories())) {
        clear_win_remote();
        clear_factories(factories);
        mpris2_list_factories(factories);
        fill_win_remote();
    }
    connect_remote(0);
}

static void event_menu(tevent_menu event) {
    udat code = event->Code;
    switch (code) {
      case G_EJECT:
	break;
      case G_CLOSE:
	exit(0);
	break;
      case G_KILL:
	if (pl) {
	    pl->del(pl);
            pl = NULL;
	}
	break;
      case G_RESCAN:
	scan_remote();
	break;
      default:
        if (code >= G_REMOTE_FIRST && factories && code - G_REMOTE_FIRST < factories->size) {
            disconnect_remote();
            connect_remote(code - G_REMOTE_FIRST);
        }
	break;
    }
}

/*
 * mouse handling
 *
 */
static void event_gadget(tevent_gadget event) {
    if (!pl)
        return;
    
    switch (event->Code) {
      case G_BACK:
        pl->prev(pl);
	break;
      case G_PLAY:
        pl->play(pl);
	break;
      case G_PAUSE:
        pl->pause(pl);
	break;
      case G_STOP:
        pl->stop(pl);
	break;
      case G_NEXT:
        pl->next(pl);
	break;
      case G_EJECT:
        /* TODO */
	break;
      case G_RESCAN:
        scan_remote();
	break;
      default:
	break;
    }
}

static void event_mouse(tevent_mouse event) {
    if (!pl)
	return;

    if (event->Code == PRESS_LEFT && event->W == win && event->Y == 1) {
	/*
	 * left mouse button: 
	 *   - set volume
	 *   - jump to track time
	 */
	if (event->X <= 12) {
            long delta = trackTime / 20;
            if (event->X <= 5)
                delta = -delta;
            pl->seek(pl, delta);

	} else if (event->X >= 14) {
	    if (event->X <= 16) {
		volume = (volume - 2) & ~(long)1;
	    } else {
		volume = (volume + 2) & ~(long)1;
	    }
            if (volume < 0)
                volume = 0;
            else if (volume > 100)
                volume = 100;
            volume = pl->set_volume(pl, volume);
	}
    } else if (event->Code == PRESS_RIGHT && event->W == win) {
	/*
	 * right mouse button: 
	 *   - switch between displaying elapsed time / displaying time to play
	 *   - switch between title scrolltype
	 */
	if (event->Y == 1 && event->X <= 12)
	    displayElapsedTime ^= 1;
	else if (event->Y == 0)
	    trackScrollMode ^= 1;
    } else if (event->Code == PRESS_MIDDLE && event->W == win) {
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
    if (init_gui(argc, argv) == 0) {
        scan_remote();
	main_loop();
    }
    
    return 0;
}
