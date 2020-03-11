// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hu_stuff.c,v 1.27 1998/05/10 19:03:41 jim Exp $
//
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:  Heads-up displays
//
//-----------------------------------------------------------------------------

// killough 5/3/98: remove unnecessary headers

#include "doomstat.h"
#include "hu_stuff.h"
#include "hu_lib.h"
#include "st_stuff.h" /* jff 2/16/98 need loc of status bar */
#include "w_wad.h"
#include "s_sound.h"
#include "dstrings.h"
#include "sounds.h"
#include "d_deh.h"   /* Ty 03/27/98 - externalization of mapnamesx arrays */
#include "r_draw.h"

// global heads up display controls

int hud_displayed;    //jff 2/23/98 turns heads-up display on/off

//
// Locally used constants, shortcuts.
//
// Ty 03/28/98 -
// These four shortcuts modifed to reflect char ** of mapnamesx[]
#define HU_TITLE  (*mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2 (*mapnames2[gamemap-1])
#define HU_TITLEP (*mapnamesp[gamemap-1])
#define HU_TITLET (*mapnamest[gamemap-1])
#define HU_TITLEHEIGHT  1
#define HU_TITLEX 0
//jff 2/16/98 change 167 to ST_Y-1
#define HU_TITLEY (ST_Y - 1 - SHORT(hu_font[0]->height)) 

//jff 2/16/98 add coord text widget coordinates
#define HU_COORDX (SCREENWIDTH - 13*SHORT(hu_font['A'-HU_FONTSTART]->width))
//jff 3/3/98 split coord widget into three lines in upper right of screen
#define HU_COORDX_Y (1 + 0*SHORT(hu_font['A'-HU_FONTSTART]->height))
#define HU_COORDY_Y (2 + 1*SHORT(hu_font['A'-HU_FONTSTART]->height))
#define HU_COORDZ_Y (3 + 2*SHORT(hu_font['A'-HU_FONTSTART]->height))

//#define HU_INPUTTOGGLE  't' // not used                           // phares
#define HU_INPUTX HU_MSGX
#define HU_INPUTY (HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH 64
#define HU_INPUTHEIGHT  1

#define key_alt   key_strafe                                        // phares
#define key_shift key_speed
extern int  key_chat;
extern int  key_escape;
extern int  key_shift;
extern int  key_alt;
extern int  destination_keys[MAXPLAYERS];                           // phares

char* chat_macros[] =    // Ty 03/27/98 - *not* externalized
{
  HUSTR_CHATMACRO0,
  HUSTR_CHATMACRO1,
  HUSTR_CHATMACRO2,
  HUSTR_CHATMACRO3,
  HUSTR_CHATMACRO4,
  HUSTR_CHATMACRO5,
  HUSTR_CHATMACRO6,
  HUSTR_CHATMACRO7,
  HUSTR_CHATMACRO8,
  HUSTR_CHATMACRO9
};

char* player_names[] =     // Ty 03/27/98 - *not* externalized
{
  HUSTR_PLRGREEN,
  HUSTR_PLRINDIGO,
  HUSTR_PLRBROWN,
  HUSTR_PLRRED
};

//jff 3/17/98 translate player colmap to text color ranges
int plyrcoltran[MAXPLAYERS]={CR_GREEN,CR_GRAY,CR_BROWN,CR_RED};

char chat_char;                 // remove later.
static player_t*  plr;

// font sets
patch_t* hu_font[HU_FONTSIZE];
patch_t* hu_fontk[HU_FONTSIZE];//jff 3/7/98 added for graphic key indicators

// widgets
static hu_textline_t  w_title;
static hu_stext_t     w_message;
static hu_itext_t     w_chat;
static hu_itext_t     w_inputbuffer[MAXPLAYERS];
static hu_textline_t  w_coordx; //jff 2/16/98 new coord widget for automap
static hu_textline_t  w_coordy; //jff 3/3/98 split coord widgets automap
static hu_textline_t  w_coordz; //jff 3/3/98 split coord widgets automap
static hu_mtext_t     w_rtext;  //jff 2/26/98 text message refresh widget

static boolean    always_off = false;
static char       chat_dest[MAXPLAYERS];
boolean           chat_on;
static boolean    message_on;
static boolean    message_list_on;   // killough 11/98
static boolean    has_message;       // killough 12/98
static boolean    reviewing_message; // killough 11/98
boolean           message_dontfuckwithme;
static boolean    message_nottobefuckedwith;
static int        message_counter;
static int        message_list_counter;         // killough 11/98
static int        hud_msg_count;     // killough 11/98
static int        message_count;     // killough 11/98
static int        chat_count;        // killough 11/98

extern int        showMessages;
static boolean    headsupactive = false;

int hud_msg_lines;  // number of message lines in window

int message_list;      // killough 11/98: made global

#define HUD_MSG_TIMER (HU_MSGTIMEOUT * (1000/TICRATE))
#define MESSAGE_TIMER (HU_MSGTIMEOUT * (1000/TICRATE))
#define CHAT_MSG_TIMER (HU_MSGTIMEOUT * (1000/TICRATE))

//jff 2/16/98 initialization strings for ammo, health, armor widgets
static char hud_coordstrx[32];
static char hud_coordstry[32];
static char hud_coordstrz[32];

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//
// Ty 03/27/98 - externalized map name arrays - now in d_deh.c
// and converted to arrays of pointers to char *
// See modified HUTITLEx macros
extern char **mapnames[];
extern char **mapnames2[];
extern char **mapnamesp[];
extern char **mapnamest[];

// key tables
// jff 5/10/98 french support removed, 
// as it was not being used and couldn't be easily tested
//
const char* shiftxform;

const char english_shiftxform[] =
{
  0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31,
  ' ', '!', '"', '#', '$', '%', '&',
  '"', // shift-'
  '(', ')', '*', '+',
  '<', // shift-,
  '_', // shift--
  '>', // shift-.
  '?', // shift-/
  ')', // shift-0
  '!', // shift-1
  '@', // shift-2
  '#', // shift-3
  '$', // shift-4
  '%', // shift-5
  '^', // shift-6
  '&', // shift-7
  '*', // shift-8
  '(', // shift-9
  ':',
  ':', // shift-;
  '<',
  '+', // shift-=
  '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '[', // shift-[
  '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
  ']', // shift-]
  '"', '_',
  '\'', // shift-`
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', '|', '}', '~', 127
};

//
// HU_Init()
//
// Initialize the heads-up display, text that overwrites the primary display
//
// Passed nothing, returns nothing
//
void HU_Init(void)
{
  int  i, j;
  char buffer[9];

  shiftxform = english_shiftxform;

  // load the heads-up font
  j = HU_FONTSTART;
  for (i=0;i<HU_FONTSIZE;i++,j++)
    {
      if (j<96)
        {
          sprintf(buffer, "STCFN%.3d",j);
          hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
        }
      else if (j>122)
        {
          sprintf(buffer, "STBR%.3d",j);
          hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
        }
      else
        hu_font[i] = hu_font[0]; //jff 2/16/98 account for gap
    }

  //jff 2/26/98 load patches for keys and double keys
  hu_fontk[0] = (patch_t *) W_CacheLumpName("STKEYS0", PU_STATIC);
  hu_fontk[1] = (patch_t *) W_CacheLumpName("STKEYS1", PU_STATIC);
  hu_fontk[2] = (patch_t *) W_CacheLumpName("STKEYS2", PU_STATIC);
  hu_fontk[3] = (patch_t *) W_CacheLumpName("STKEYS3", PU_STATIC);
  hu_fontk[4] = (patch_t *) W_CacheLumpName("STKEYS4", PU_STATIC);
  hu_fontk[5] = (patch_t *) W_CacheLumpName("STKEYS5", PU_STATIC);
}

//
// HU_Stop()
//
// Make the heads-up displays inactive
//
// Passed nothing, returns nothing
//
void HU_Stop(void)
{
  headsupactive = false;
}

//
// HU_Start(void)
//
// Create and initialize the heads-up widgets, software machines to
// maintain, update, and display information over the primary display
//
// This routine must be called after any change to the heads up configuration
// in order for the changes to take effect in the actual displays
//
// Passed nothing, returns nothing
//
void HU_Start(void)
{
  int   i;
  char* s;

  if (headsupactive)                    // stop before starting
    HU_Stop();

  plr = &players[displayplayer];        // killough 3/7/98
  message_on = false;
  message_dontfuckwithme = false;
  message_nottobefuckedwith = false;
  chat_on = false;

  // killough 11/98:
  reviewing_message = message_list_on = false;
  message_counter = message_list_counter = 0;
  hud_msg_count = (HUD_MSG_TIMER * TICRATE) / 1000 + 1;
  message_count = (MESSAGE_TIMER * TICRATE) / 1000 + 1;
  chat_count    = (CHAT_MSG_TIMER * TICRATE) / 1000 + 1;

  // create the message widget
  // messages to player in upper-left of screen
  HUlib_initSText(&w_message, HU_MSGX, HU_MSGY, HU_MSGHEIGHT, hu_font,
		  HU_FONTSTART, colrngs[6], &message_on);

  //jff 2/16/98 added some HUD widgets
  // create the map title widget - map title display in lower left of automap
  HUlib_initTextLine(&w_title, HU_TITLEX, HU_TITLEY, hu_font,
		     HU_FONTSTART, colrngs[6]);

  // create the hud text refresh widget
  // scrolling display of last hud_msg_lines messages received

  if (hud_msg_lines>HU_MAXMESSAGES)
    hud_msg_lines=HU_MAXMESSAGES;

  //jff 2/26/98 add the text refresh widget initialization
  HUlib_initMText(&w_rtext, 0, 0, SCREENWIDTH,
		  (hud_msg_lines+2)*HU_REFRESHSPACING, hu_font,
		  HU_FONTSTART, colrngs[6],
		  &message_list_on);      // killough 11/98

  // initialize the automap's level title widget

  // [FG] fix crash when gamemap is not initialized
  if (gamestate == GS_LEVEL && gamemap > 0)
  {
  s = gamemode != commercial ? HU_TITLE : gamemission == pack_tnt ?
    HU_TITLET : gamemission == pack_plut ? HU_TITLEP : HU_TITLE2;
  }
  else
  s = "";

  while (*s)
    HUlib_addCharToTextLine(&w_title, *s++);

  // create the automaps coordinate widget
  // jff 3/3/98 split coord widget into three lines: x,y,z

  HUlib_initTextLine(&w_coordx, HU_COORDX, HU_COORDX_Y, hu_font,
		     HU_FONTSTART, colrngs[6]);
  HUlib_initTextLine(&w_coordy, HU_COORDX, HU_COORDY_Y, hu_font,
		     HU_FONTSTART, colrngs[6]);
  HUlib_initTextLine(&w_coordz, HU_COORDX, HU_COORDZ_Y, hu_font,
		     HU_FONTSTART, colrngs[6]);
  
  // initialize the automaps coordinate widget
  //jff 3/3/98 split coordstr widget into 3 parts
  sprintf(hud_coordstrx,"X: %-5d",0); //jff 2/22/98 added z
  s = hud_coordstrx;
  while (*s)
    HUlib_addCharToTextLine(&w_coordx, *s++);
  sprintf(hud_coordstry,"Y: %-5d",0); //jff 3/3/98 split x,y,z
  s = hud_coordstry;
  while (*s)
    HUlib_addCharToTextLine(&w_coordy, *s++);
  sprintf(hud_coordstrz,"Z: %-5d",0); //jff 3/3/98 split x,y,z
  s = hud_coordstrz;
  while (*s)
    HUlib_addCharToTextLine(&w_coordz, *s++);

  // create the chat widget
  HUlib_initIText
    (
     &w_chat,
     HU_INPUTX,
     HU_INPUTY,
     hu_font,
     HU_FONTSTART,
     colrngs[6],
     &chat_on
     );

  // create the inputbuffer widgets, one per player
  for (i=0 ; i<MAXPLAYERS ; i++)
    HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, colrngs[6],
		    &always_off);

  // now allow the heads-up display to run
  headsupactive = true;
}

//
// HU_Drawer()
//
// Draw all the pieces of the heads-up display
//
// Passed nothing, returns nothing
//
void HU_Drawer(void)
{
  extern int ddt_cheating;
  char *s;
  player_t *plr;

  plr = &players[displayplayer];         // killough 3/7/98
  // draw the automap widgets if automap is displayed
  if (automapactive)
    {
      // map title
      HUlib_drawTextLine(&w_title, false);
    }

  if (automapactive && ddt_cheating)
    {
      fixed_t x,y,z;   // killough 10/98:
      void AM_Coordinates(const mobj_t *, fixed_t *, fixed_t *, fixed_t *);

      // killough 10/98: allow coordinates to display non-following pointer 
      AM_Coordinates(plr->mo, &x, &y, &z);

      //jff 2/16/98 output new coord display
      // x-coord
      sprintf(hud_coordstrx,"X: %-5d", x>>FRACBITS); // killough 10/98
      HUlib_clearTextLine(&w_coordx);
      s = hud_coordstrx;
      while (*s)
        HUlib_addCharToTextLine(&w_coordx, *s++);
      HUlib_drawTextLine(&w_coordx, false);

      //jff 3/3/98 split coord display into x,y,z lines
      // y-coord
      sprintf(hud_coordstry,"Y: %-5d", y>>FRACBITS); // killough 10/98
      HUlib_clearTextLine(&w_coordy);
      s = hud_coordstry;
      while (*s)
        HUlib_addCharToTextLine(&w_coordy, *s++);
      HUlib_drawTextLine(&w_coordy, false);

      //jff 3/3/98 split coord display into x,y,z lines  
      //jff 2/22/98 added z
      // z-coord
      sprintf(hud_coordstrz,"Z: %-5d", z>>FRACBITS);  // killough 10/98
      HUlib_clearTextLine(&w_coordz);
      s = hud_coordstrz;
      while (*s)
        HUlib_addCharToTextLine(&w_coordz, *s++);
      HUlib_drawTextLine(&w_coordz, false);
    }

  //jff 3/4/98 display last to give priority
  // jff 4/24/98 Erase current lines before drawing current
  // needed when screen not fullsize
  // killough 11/98: only do it when not fullsize
  if (scaledviewheight < 200)
    HU_Erase(); 

  //jff 4/21/98 if setup has disabled message list while active, turn it off
  // if the message review is enabled show the scrolling message review
  // if the message review not enabled, show the standard message widget
  // killough 11/98: simplified

  if (message_list)
    HUlib_drawMText(&w_rtext);
  else
    HUlib_drawSText(&w_message);
  
  // display the interactive buffer for chat entry
  HUlib_drawIText(&w_chat);
}

//
// HU_Erase()
//
// Erase hud display lines that can be trashed by small screen display
//
// Passed nothing, returns nothing
//

void HU_Erase(void)
{
  // erase the message display or the message review display
  if (!message_list)
    HUlib_eraseSText(&w_message);
  else
    HUlib_eraseMText(&w_rtext);
  
  // erase the interactive text buffer for chat entry
  HUlib_eraseIText(&w_chat);

  // erase the automap title
  HUlib_eraseTextLine(&w_title);
}

//
// HU_Ticker()
//
// Update the hud displays once per frame
//
// Passed nothing, returns nothing
//
void HU_Ticker(void)
{
  // killough 11/98: support counter for message list as well as regular msg
  if (message_list_counter && !--message_list_counter)
    {
      reviewing_message = message_list_on = false;
    }

  // tick down message counter if message is up
  if (message_counter && !--message_counter)
    reviewing_message = message_on = message_nottobefuckedwith = false;

  // if messages on, or "Messages Off" is being displayed
  // this allows the notification of turning messages off to be seen
  // display message if necessary

  if ((showMessages || message_dontfuckwithme) && plr->message &&
      (!message_nottobefuckedwith || message_dontfuckwithme))
    {
      //post the message to the message widget
      HUlib_addMessageToSText(&w_message, 0, plr->message);

      //jff 2/26/98 add message to refresh text widget too
      HUlib_addMessageToMText(&w_rtext, 0, plr->message);

      // clear the message to avoid posting multiple times
      plr->message = 0;
	  
      // killough 11/98: display message list, possibly timed
      if (message_list)
	{
	  message_list_counter = hud_msg_count;
	  message_list_on = true;
	}
      else
	{
	  message_on = true;       // note a message is displayed
	  // start the message persistence counter	      
	  message_counter = message_count;
	}

      has_message = true;        // killough 12/98

      // transfer "Messages Off" exception to the "being displayed" variable
      message_nottobefuckedwith = message_dontfuckwithme;

      // clear the flag that "Messages Off" is being posted
      message_dontfuckwithme = 0;
    }

  // check for incoming chat characters
  if (netgame)
    {
      int i, rc;
      char c;

      for (i=0; i<MAXPLAYERS; i++)
        {
          if (!playeringame[i])
            continue;
          if (i != consoleplayer
              && (c = players[i].cmd.chatchar))
            {
              if (c <= HU_BROADCAST)
                chat_dest[i] = c;
              else
                {
                  if (c >= 'a' && c <= 'z')
                    c = (char) shiftxform[(unsigned char) c];
                  rc = HUlib_keyInIText(&w_inputbuffer[i], c);
                  if (rc && c == KEYD_ENTER)
                    {
                      if (w_inputbuffer[i].l.len
                          && (chat_dest[i] == consoleplayer+1
                              || chat_dest[i] == HU_BROADCAST))
                        {
                          HUlib_addMessageToSText(&w_message,
                                                  player_names[i],
                                                  w_inputbuffer[i].l.l);

			  has_message = true;        // killough 12/98
                          message_nottobefuckedwith = true;
                          message_on = true;
                          message_counter = chat_count;  // killough 11/98
			  S_StartSound(0, gamemode == commercial ?
				       sfx_radio : sfx_tink);
                        }
                      HUlib_resetIText(&w_inputbuffer[i]);
                    }
                }
              players[i].cmd.chatchar = 0;
            }
        }
    }
}

#define QUEUESIZE   128

static char chatchars[QUEUESIZE];
static int  head = 0;
static int  tail = 0;

//
// HU_queueChatChar()
//
// Add an incoming character to the circular chat queue
//
// Passed the character to queue, returns nothing
//
void HU_queueChatChar(char c)
{
  if (((head + 1) & (QUEUESIZE-1)) == tail)
    plr->message = HUSTR_MSGU;
  else
    {
      chatchars[head++] = c;
      head &= QUEUESIZE-1;
    }
}

//
// HU_dequeueChatChar()
//
// Remove the earliest added character from the circular chat queue
//
// Passed nothing, returns the character dequeued
//
char HU_dequeueChatChar(void)
{
  char c;

  if (head != tail)
    {
      c = chatchars[tail++];
      tail &= QUEUESIZE-1;
    }
  else
    c = 0;
  return c;
}

//
// HU_Responder()
//
// Responds to input events that affect the heads up displays
//
// Passed the event to respond to, returns true if the event was handled
//
boolean HU_Responder(event_t *ev)
{
  static char   lastmessage[HU_MAXLINELENGTH+1];
  char*   macromessage;
  boolean   eatkey = false;
  static boolean  shiftdown = false;
  static boolean  altdown = false;
  unsigned char   c;
  int     i;
  int     numplayers;

  static int    num_nobrainers = 0;

  numplayers = 0;
  for (i=0 ; i<MAXPLAYERS ; i++)
    numplayers += playeringame[i];

  if (ev->data1 == key_shift)
    {
      shiftdown = ev->type == ev_keydown;
      return false;
    }

  if (ev->data1 == key_alt)
    {
      altdown = ev->type == ev_keydown;
      return false;
    }

  if (ev->type != ev_keydown)
    return false;

  if (!chat_on)
    {
      if (ev->data1 == KEYD_ENTER)                                 // phares
        {
	  //jff 2/26/98 toggle list of messages

	  // killough 11/98:
	  // Toggle message list only if a message is actively being reviewed.
	  if (has_message)
	    {
	      if (message_list ? message_list_on && reviewing_message :
		  message_on && reviewing_message)
		if (!(message_list = !message_list))
		  {
		    // killough 12/98:
		    // fix crash at startup if KEYD_ENTER held down
		    if (gametic && gamestate == GS_LEVEL)
		      HU_Erase(); //jff 4/28/98 erase behind messages

		    message_list_on = false;
		  }

	      // killough 11/98: Support timed or continuous message lists

	      if (!message_list)      // if not message list, refresh message
		{
		  message_counter = message_count;
		  reviewing_message = message_on = true;
		}
	      else
		{                     // message list, possibly timed
		  message_list_counter = hud_msg_count;
		  reviewing_message = message_list_on = true;
		}
	    }
          eatkey = true;
        }  //jff 2/26/98 no chat if message review is displayed
      else // killough 10/02/98: no chat if demo playback
        if (!demoplayback)
          if (!message_list)
          {
	    if (netgame && ev->data1 == key_chat)
	      {
		eatkey = chat_on = true;
		HUlib_resetIText(&w_chat);
		HU_queueChatChar(HU_BROADCAST);
	      }//jff 2/26/98
	    else    // killough 11/98: simplify
	      if (!message_list && netgame && numplayers > 2)
		for (i=0; i<MAXPLAYERS ; i++)
		  if (ev->data1 == destination_keys[i])
		  {
		    if (i == consoleplayer)
		      plr->message = 
			++num_nobrainers <  3 ? HUSTR_TALKTOSELF1 :
	                  num_nobrainers <  6 ? HUSTR_TALKTOSELF2 :
	                  num_nobrainers <  9 ? HUSTR_TALKTOSELF3 :
	                  num_nobrainers < 32 ? HUSTR_TALKTOSELF4 :
                                                HUSTR_TALKTOSELF5 ;
                  else
                    if (playeringame[i])
                      {
                        eatkey = chat_on = true;
                        HUlib_resetIText(&w_chat);
                        HU_queueChatChar((char)(i+1));
                        break;
                      }
		  }
          }
    }//jff 2/26/98 no chat functions if message review is displayed
  else
    if (!message_list)
      {
        c = ev->data1;
        // send a macro
        if (altdown)
          {
            c = c - '0';
            if (c > 9)
              return false;
            // fprintf(stderr, "got here\n");
            macromessage = chat_macros[c];
      
            // kill last message with a '\n'
            HU_queueChatChar((char)KEYD_ENTER); // DEBUG!!!                // phares
      
            // send the macro message
            while (*macromessage)
              HU_queueChatChar(*macromessage++);
            HU_queueChatChar((char)KEYD_ENTER);                            // phares
      
            // leave chat mode and notify that it was sent
            chat_on = false;
            strcpy(lastmessage, chat_macros[c]);
            plr->message = lastmessage;
            eatkey = true;
          }
        else
          {
            if (shiftdown || (c >= 'a' && c <= 'z'))
              c = shiftxform[c];
            eatkey = HUlib_keyInIText(&w_chat, c);
            if (eatkey)
              HU_queueChatChar(c);

            if (c == KEYD_ENTER)                                     // phares
              {
                chat_on = false;
                if (w_chat.l.len)
                  {
                    strcpy(lastmessage, w_chat.l.l);
                    plr->message = lastmessage;
                  }
              }
            else
              if (c == key_escape)                               // phares
                chat_on = false;
          }
      }
  return eatkey;
}


//----------------------------------------------------------------------------
//
// $Log: hu_stuff.c,v $
// Revision 1.27  1998/05/10  19:03:41  jim
// formatted/documented hu_stuff
//
// Revision 1.26  1998/05/03  22:25:24  killough
// Provide minimal headers at top; nothing else
//
// Revision 1.25  1998/04/28  15:53:58  jim
// Fix message list bug in small screen mode
//
// Revision 1.24  1998/04/22  12:50:14  jim
// Fix lockout from dynamic message change
//
// Revision 1.23  1998/04/05  10:09:51  jim
// added STCFN096 lump
//
// Revision 1.22  1998/03/28  05:32:12  jim
// Text enabling changes for DEH
//
// Revision 1.19  1998/03/17  20:45:23  jim
// added frags to HUD
//
// Revision 1.18  1998/03/15  14:42:16  jim
// added green fist/chainsaw in HUD when berserk
//
// Revision 1.17  1998/03/10  07:07:15  jim
// Fixed display glitch in HUD cycle
//
// Revision 1.16  1998/03/09  11:01:48  jim
// fixed string overflow for DEH, added graphic keys
//
// Revision 1.15  1998/03/09  07:10:09  killough
// Use displayplayer instead of consoleplayer
//
// Revision 1.14  1998/03/05  00:57:37  jim
// Scattered HUD
//
// Revision 1.13  1998/03/04  11:50:48  jim
// Change automap coord display
//
// Revision 1.12  1998/02/26  22:58:26  jim
// Added message review display to HUD
//
// Revision 1.11  1998/02/23  14:20:51  jim
// Merged HUD stuff, fixed p_plats.c to support elevators again
//
// Revision 1.10  1998/02/23  04:26:07  killough
// really allow new hud stuff to be turned off COMPLETELY
//
// Revision 1.9  1998/02/22  12:51:26  jim
// HUD control on F5, z coord, spacing change
//
// Revision 1.7  1998/02/20  18:46:51  jim
// cleanup of HUD control
//
// Revision 1.6  1998/02/19  16:54:53  jim
// Optimized HUD and made more configurable
//
// Revision 1.5  1998/02/18  11:55:55  jim
// Fixed issues with HUD and reduced screen size
//
// Revision 1.3  1998/02/15  02:47:47  phares
// User-defined keys
//
// Revision 1.2  1998/01/26  19:23:22  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:55  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
