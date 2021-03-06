/*
Chat module plugin for Miranda IM

Copyright (C) 2003 J�rgen Persson

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "../commonheaders.h"
#include <math.h>
#include <mbstring.h>
#include <shlwapi.h>


// The code for streaming the text is to a large extent copied from
// the srmm module and then modified to fit the chat module.

extern FONTINFO		aFonts[OPTIONS_FONTCOUNT];
extern HICON		hIcons[30];
extern MYGLOBALS	myGlobals;

static PBYTE pLogIconBmpBits[14];
static int logIconBmpSize[sizeof(pLogIconBmpBits) / sizeof(pLogIconBmpBits[0])];

static int logPixelSY = 0;
static int logPixelSX = 0;
static char *szDivider = "\\strike----------------------------------------------------------------------------\\strike0";
static char CHAT_rtfFontsGlobal[OPTIONS_FONTCOUNT + 2][RTFCACHELINESIZE];
static char *CHAT_rtffonts = 0;
static char szMicroLFeed[150];

void GetIconSize(HICON hIcon, int* sizeX, int* sizeY);

static int EventToIndex(LOGINFO * lin)
{
	switch(lin->iType)
	{
	case GC_EVENT_MESSAGE: 
		{
		if(lin->bIsMe) 
			return 10;
		else 
			return 9;
		}
	case GC_EVENT_JOIN: return 3;
	case GC_EVENT_PART: return 4;
	case GC_EVENT_QUIT: return 5;
	case GC_EVENT_NICK: return 7;
	case GC_EVENT_KICK: return 6;
	case GC_EVENT_NOTICE: return 8;
	case GC_EVENT_TOPIC: return 11;
	case GC_EVENT_INFORMATION:return 12;
	case GC_EVENT_ADDSTATUS: return 13;
	case GC_EVENT_REMOVESTATUS: return 14;
	case GC_EVENT_ACTION: return 15;
	default:break;
	}
	return 0;
}
static BYTE EventToSymbol(LOGINFO *lin)
{
    switch(lin->iType) {
        case GC_EVENT_MESSAGE:
            if(lin->bIsMe)
                return 0x37;
            else
                return 0x38;
        case GC_EVENT_JOIN:
            return 0x34;
        case GC_EVENT_PART:
            return 0x33;
        case GC_EVENT_QUIT:
            return 0x39;
        case GC_EVENT_NICK:
            return 0x71;
        case GC_EVENT_KICK:
            return 0x72;
        case GC_EVENT_NOTICE:
            return 0x28;
        case GC_EVENT_INFORMATION:
            return 0x69;
        case GC_EVENT_ADDSTATUS:
            return 0x35;
        case GC_EVENT_REMOVESTATUS:
            return 0x36;
        case GC_EVENT_ACTION:
            return 0x60;
    }
    return 0x73;
}
static int EventToIcon(LOGINFO * lin)
{
	switch(lin->iType)
	{
	case GC_EVENT_MESSAGE: 
		{
		if(lin->bIsMe) 
			return ICON_MESSAGEOUT;
		else 
			return ICON_MESSAGE;
		}
	case GC_EVENT_JOIN: return ICON_JOIN;
	case GC_EVENT_PART: return ICON_PART;
	case GC_EVENT_QUIT: return ICON_QUIT;
	case GC_EVENT_NICK: return ICON_NICK;
	case GC_EVENT_KICK: return ICON_KICK;
	case GC_EVENT_NOTICE: return ICON_NOTICE;
	case GC_EVENT_TOPIC: return ICON_TOPIC;
	case GC_EVENT_INFORMATION:return ICON_INFO;
	case GC_EVENT_ADDSTATUS: return ICON_ADDSTATUS;
	case GC_EVENT_REMOVESTATUS: return ICON_REMSTATUS;
	case GC_EVENT_ACTION: return ICON_ACTION;
	default:break;
	}
	return 0;
}


static char *Log_SetStyle(int style, int fontindex)
{
    if(style < OPTIONS_FONTCOUNT)
        return CHAT_rtffonts + (style * RTFCACHELINESIZE);
    else
        return "";
    /*
    static char szStyle[128];
    mir_snprintf(szStyle, sizeof(szStyle), "\\f%u\\cf%u\\ul0\\highlight0\\b%d\\i%d\\fs%u", style, style+1, aFonts[fontindex].lf.lfWeight >= FW_BOLD ? 1 : 0, aFonts[fontindex].lf.lfItalic, 2 * abs(aFonts[fontindex].lf.lfHeight) * 74 / logPixelSY);
    return szStyle;
    */
}

static void Log_Append(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
	va_list va;
	int charsDone = 0;

	va_start(va, fmt);
	for (;;) {
		charsDone = mir_vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
		if (charsDone >= 0)
			break;
		*cbBufferAlloced += 4096;
		*buffer = (char *) realloc(*buffer, *cbBufferAlloced);
	}
	va_end(va);
	*cbBufferEnd += charsDone;
}

static int Log_AppendRTF(LOGSTREAMDATA* streamData,char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
	va_list va;
    int charsDone, i;

    va_start(va, fmt);
    for (;;) {
        charsDone = mir_vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
        if (charsDone >= 0)
            break;
        *cbBufferAlloced += 4096;
        *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
    }
    va_end(va);
    *cbBufferEnd += charsDone;
    for (i = *cbBufferEnd - charsDone; (*buffer)[i]; i++) 
	{
        if ((*buffer)[i] == '\r' && (*buffer)[i + 1] == '\n') 
		{
            if (*cbBufferEnd + 5 > *cbBufferAlloced) 
			{
                *cbBufferAlloced += 4096;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
            MoveMemory(*buffer + i + 6, *buffer + i + 2, *cbBufferEnd - i - 1);
            CopyMemory(*buffer + i, "\\line ", 6);
            *cbBufferEnd += 4;
        }
        else if ((*buffer)[i] == '\n') 
		{
            if (*cbBufferEnd + 6 > *cbBufferAlloced) 
			{
                *cbBufferAlloced += 4096;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
            MoveMemory(*buffer + i + 6, *buffer + i + 1, *cbBufferEnd - i);
            CopyMemory(*buffer + i, "\\line ", 6);
            *cbBufferEnd += 5;
        }

		else if ( (*buffer)[i]=='%') 
		{
			char szTemp[200];
			int iLen = 0;
			int iOldCount = 0;

			szTemp[0] = '\0';		

			switch ((*buffer)[i + 1])
			{
			case '\0':
				mir_snprintf(szTemp, sizeof(szTemp), "%s", "%");
				iOldCount = 1;
				break;
			case '%':
				mir_snprintf(szTemp, sizeof(szTemp), "%s", "%");
				iOldCount = 2;
				break;
			case 'c':
			case 'f':
				if(g_Settings.StripFormat || streamData->bStripFormat)
					szTemp[0] = '\0';
				else if((*buffer)[i + 2] != '\0' && (*buffer)[i + 3] != '\0')
				{
					char szTemp3[3];
					int col;
					szTemp3[0] = (*buffer)[i + 2];
					szTemp3[1] = (*buffer)[i + 3];
					szTemp3[2] = '\0';

//					lstrcpynA(szTemp3, (char *)((*buffer)[i + 2]), 3);
					col = atoi(szTemp3);
					col += 19;
					mir_snprintf(szTemp, sizeof(szTemp), ((*buffer)[i + 1]=='c')?"\\cf%u ":"\\highlight%u ", col);
				}
				iOldCount = 4;
				break;
			case 'C':
			case 'F':
				if(g_Settings.StripFormat || streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					int j = streamData->lin->bIsHighlighted?16:EventToIndex(streamData->lin);
					if( ((*buffer)[i + 1]) == 'C' )
						mir_snprintf(szTemp, sizeof(szTemp), "\\cf%u ", j+1);
					else
						mir_snprintf(szTemp, sizeof(szTemp), "\\highlight0 ");
				}
				iOldCount = 2;
				break;
			case 'b':
			case 'u':
			case 'i':
				if(streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					mir_snprintf(szTemp, sizeof(szTemp), (*buffer)[i + 1] == 'u'?"\\%cl ":"\\%c ", (*buffer)[i + 1]);
				}
				iOldCount = 2;
				break;
			case 'B':
			case 'U':
			case 'I':
				if(streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					mir_snprintf(szTemp, sizeof(szTemp), (*buffer)[i + 1] == 'U' ? "\\%cl0 " : "\\%c0 ", (char)CharLowerA((char *)(*buffer)[i + 1]));
				}
				iOldCount = 2;
				break;
			case 'r':
				if(streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					int index = EventToIndex(streamData->lin);
					mir_snprintf(szTemp, sizeof(szTemp), "%s ", Log_SetStyle(index, index));
				}
				iOldCount = 2;
				break;
			default:break;
			}
			if(iOldCount)
			{
				iLen = lstrlenA(szTemp);
				if (*cbBufferEnd + (iLen-iOldCount)+1 > *cbBufferAlloced) {
					*cbBufferAlloced += 4096;
					*buffer = (char *) realloc(*buffer, *cbBufferAlloced);
				}
				MoveMemory(*buffer + i + iLen, *buffer + i + iOldCount, *cbBufferEnd - i - iOldCount+1);
				if(iLen > 0)
				{
					CopyMemory(*buffer + i, szTemp, iLen);
					*cbBufferEnd += iLen - iOldCount;
					i += iLen;
					i -=1;
				}
				else
				{
					*cbBufferEnd -= iOldCount;
					i -= 1;
				}
			}

		}
		
       else if ((*buffer)[i] == '\t' && !streamData->bStripFormat) {
            if (*cbBufferEnd + 5 > *cbBufferAlloced) {
                *cbBufferAlloced += 4096;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
            MoveMemory(*buffer + i + 5, *buffer + i + 1, *cbBufferEnd - i);
            CopyMemory(*buffer + i, "\\tab ", 5);
            *cbBufferEnd += 4;
        }
        else if (((*buffer)[i] == '\\' || (*buffer)[i] == '{' || (*buffer)[i] == '}') && !streamData->bStripFormat) {
            if (*cbBufferEnd + 2 > *cbBufferAlloced) {
                *cbBufferAlloced += 4096;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
            MoveMemory(*buffer + i + 1, *buffer + i, *cbBufferEnd - i + 1);
            (*buffer)[i] = '\\';
            ++*cbBufferEnd;
            i++;
        }
    }
    return _mbslen(*buffer + *cbBufferEnd);

}
static void AddEventToBuffer(char **buffer, int *bufferEnd, int *bufferAlloced, LOGSTREAMDATA *streamData)
{
	char szTemp[512];
	char szTemp2[512];
	char * pszNick = NULL;
	if(streamData->lin->pszNick )
	{
		if(g_Settings.LogLimitNames && lstrlenA(streamData->lin->pszNick) > 20)
		{
			lstrcpynA(szTemp2, streamData->lin->pszNick, 20);
			lstrcpynA(szTemp2+20, "...", 4);
		}
		else
			lstrcpynA(szTemp2, streamData->lin->pszNick, 511);

		if(streamData->lin->pszUserInfo)
			mir_snprintf(szTemp, sizeof(szTemp), "%s (%s)", szTemp2, streamData->lin->pszUserInfo);
		else
			mir_snprintf(szTemp, sizeof(szTemp), "%s", szTemp2);
		pszNick = szTemp;
	}

	if(streamData && streamData->lin)
	{
		switch(streamData->lin->iType)
		{
		case GC_EVENT_MESSAGE:
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, "%s", streamData->lin->pszText);break;
		case GC_EVENT_ACTION:
			if(streamData->lin->pszNick && streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s %s"), streamData->lin->pszNick, streamData->lin->pszText);break;
		case GC_EVENT_JOIN:
			if(pszNick)
				if(!streamData->lin->bIsMe)
					Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s has joined"), pszNick);
				else
					Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("You have joined %s"), streamData->si->pszName);
					break;
		case GC_EVENT_PART:
			if(pszNick)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s has left"), pszNick);
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, ": %s", streamData->lin->pszText);
				break;
		case GC_EVENT_QUIT:
			if(pszNick)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s has disconnected"), pszNick);
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, ": %s", streamData->lin->pszText);
				break;
		case GC_EVENT_NICK:
			if(pszNick && streamData->lin->pszText)
				if(!streamData->lin->bIsMe)
					Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s is now known as %s"), pszNick, streamData->lin->pszText);
				else
					Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("You are now known as %s"), streamData->lin->pszText);
					break;
		case GC_EVENT_KICK:
			if(streamData->lin->pszNick && streamData->lin->pszStatus)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s kicked %s"), streamData->lin->pszStatus, streamData->lin->pszNick);
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, ": %s", streamData->lin->pszText);
				break;
		case GC_EVENT_NOTICE:
			if(streamData->lin->pszNick && streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("Notice from %s: %s"), streamData->lin->pszNick, streamData->lin->pszText);break;
		case GC_EVENT_TOPIC:
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("The topic is \'%s%s\'"), streamData->lin->pszText, "%r");
			if(streamData->lin->pszNick)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate(" (set by %s)"), streamData->lin->pszNick);
			break;
		case GC_EVENT_INFORMATION:
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, streamData->lin->bIsMe?"--> %s":"%s", streamData->lin->pszText);break;
		case GC_EVENT_ADDSTATUS:
			if(streamData->lin->pszNick && streamData->lin->pszText && streamData->lin->pszStatus)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s enables \'%s\' status for %s"), streamData->lin->pszText, streamData->lin->pszStatus, streamData->lin->pszNick);break;
		case GC_EVENT_REMOVESTATUS:
			if(streamData->lin->pszNick && streamData->lin->pszText && streamData->lin->pszStatus)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s disables \'%s\' status for %s"), streamData->lin->pszText , streamData->lin->pszStatus, streamData->lin->pszNick);break;
		default:break;
		}
	}

}

char *MakeTimeStamp(char * pszStamp, time_t time)
{

	static char szTime[30];
    _tzset();
	strftime(szTime, 29, pszStamp, localtime(&time));
	return szTime;
}

static char* Log_CreateRTF(LOGSTREAMDATA *streamData)
{
 	char *buffer, *header;
    int bufferAlloced, bufferEnd, i, me = 0;
	LOGINFO * lin = streamData->lin;
    MODULEINFO *mi = MM_FindModule(streamData->si->pszModule);
	// guesstimate amount of memory for the RTF
    bufferEnd = 0;
	bufferAlloced = streamData->bRedraw ? 1024 * (streamData->si->iEventCount+2) : 2048;
    buffer = (char *) malloc(bufferAlloced);
	buffer[0] = '\0';
	


	// ### RTF HEADER
	header = mi->pszHeader;
    streamData->crCount = mi->nColorCount;
        
	if(header)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, header);


	// ### RTF BODY (one iteration per event that should be streamed in)
	while(lin)
	{
		// filter
		if(streamData->si->iType != GCW_CHATROOM || !streamData->si->bFilterEnabled || (streamData->si->iLogFilterFlags&lin->iType) != 0)
		{
            if(streamData->dat->dwFlags & MWF_DIVIDERWANTED || lin->dwFlags & MWF_DIVIDERWANTED) {
                static char szStyle_div[128] = "\0";
                if(szStyle_div[0] == 0)
                    mir_snprintf(szStyle_div, 128, "\\f%u\\cf%u\\ul0\\b%d\\i%d\\fs%u", 17, 18, 0, 0, 5);
                
                lin->dwFlags |= MWF_DIVIDERWANTED;
                if(lin->prev || !streamData->bRedraw)
                    Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\par\\qc\\sl-1\\highlight%d %s ---------------------------------------------------------------------------------------", 18, szStyle_div);
                streamData->dat->dwFlags &= ~MWF_DIVIDERWANTED;
            }
            // create new line, and set font and color
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\par\\ql\\sl0%s ", Log_SetStyle(0, 0));

            Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\v~-+%d+-~\\v0 ", lin);
            //_DebugTraceA("log stats: begin %d, end %d, cur %d", streamData->si->pLog, streamData->si->pLogEnd, lin);
			// Insert icon
            if (g_Settings.LogSymbols)                // use symbols
                Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s %c", Log_SetStyle(17, 17), EventToSymbol(lin));
            else {
                if (lin->iType&g_Settings.dwIconFlags || lin->bIsHighlighted&&g_Settings.dwIconFlags&GC_EVENT_HIGHLIGHT)
                {
                    int iIndex = (lin->bIsHighlighted&&g_Settings.dwIconFlags&GC_EVENT_HIGHLIGHT) ? ICON_HIGHLIGHT : EventToIcon(lin);
                    Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\f0\\fs14");
                    while (bufferAlloced - bufferEnd < logIconBmpSize[0])
                        bufferAlloced += 4096;
                    buffer = (char *) realloc(buffer, bufferAlloced);
                    CopyMemory(buffer + bufferEnd, pLogIconBmpBits[iIndex], logIconBmpSize[iIndex]);
                    bufferEnd += logIconBmpSize[iIndex];
                }
            }

			if(g_Settings.TimeStampEventColour)
			{
				// colored timestamps
				static char szStyle[256];
				int iii;
				if(lin->pszNick && lin->iType == GC_EVENT_MESSAGE)
				{
					iii = lin->bIsHighlighted?16:(lin->bIsMe ? 2 : 1);
					mir_snprintf(szStyle, sizeof(szStyle), "\\f0\\cf%u\\ul0\\highlight0\\b%d\\i%d\\fs%u", iii+1, aFonts[0].lf.lfWeight >= FW_BOLD ? 1 : 0, aFonts[0].lf.lfItalic, 2 * abs(aFonts[0].lf.lfHeight) * 74 / logPixelSY);
					Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", szStyle);
				}
				else
				{
					iii = lin->bIsHighlighted?16:EventToIndex(lin);
					mir_snprintf(szStyle, sizeof(szStyle), "\\f0\\cf%u\\ul0\\highlight0\\b%d\\i%d\\fs%u", iii+1, aFonts[0].lf.lfWeight >= FW_BOLD ? 1 : 0, aFonts[0].lf.lfItalic, 2 * abs(aFonts[0].lf.lfHeight) * 74 / logPixelSY);
					Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", szStyle);
				}
			}
			else
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(0, 0 ));
			// insert a TAB if necessary to put the timestamp in the right position
			if (g_Settings.dwIconFlags)
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tab ");

			//insert timestamp
			if(g_Settings.ShowTime)
			{
				char szTimeStamp[30];
				char szOldTimeStamp[30];
				
				lstrcpynA(szTimeStamp, MakeTimeStamp(g_Settings.pszTimeStamp, lin->time), 30);
				lstrcpynA(szOldTimeStamp, MakeTimeStamp(g_Settings.pszTimeStamp, streamData->si->LastTime), 30);
				if(!g_Settings.ShowTimeIfChanged || streamData->si->LastTime == 0 || lstrcmpA(szTimeStamp, szOldTimeStamp))
				{
					streamData->si->LastTime = lin->time;
					Log_Append(&buffer, &bufferEnd, &bufferAlloced, szTimeStamp);
				}
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tab ");
			}

			// Insert the nick
			if(lin->pszNick && lin->iType == GC_EVENT_MESSAGE)
			{
				char pszTemp[300];
				char * p1;
                STATUSINFO *ti;
                char pszIndicator[2] = "\0\0";
                int  crNickIndex = 0;
                
                if(g_Settings.ClassicIndicators || g_Settings.ColorizeNicks) {
                    USERINFO *ui = streamData->si->pUsers;
                    
                    while(ui) {
                        if(!strcmp(ui->pszNick, lin->pszNick)) {
                            ti = TM_FindStatus(streamData->si->pStatuses, TM_WordToString(streamData->si->pStatuses, ui->Status));
                            if(ti && (int)ti->hIcon < streamData->si->iStatusCount) {
                                int id = streamData->si->iStatusCount - (int)ti->hIcon - 1;
                                switch(id) {
                                    case 1:
                                        pszIndicator[0] = '+';
                                        crNickIndex = 2;
                                        break;
                                    case 2:
                                        pszIndicator[0] = '%';
                                        crNickIndex = 1;
                                        break;
                                    case 3:
                                        pszIndicator[0] = '@';
                                        crNickIndex = 0;
                                        break;
                                    case 4:
                                        pszIndicator[0] = '!';
                                        crNickIndex = 3;
                                        break;
                                    case 5:
                                        pszIndicator[0] = '*';
                                        crNickIndex = 4;
                                        break;
                                    default:
                                        pszIndicator[0] = 0;
                                }
                            }
                            break;
                        }
                        ui = ui->next;
                    }
                }
                Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(lin->bIsMe ? 2 : 1, lin->bIsMe ? 2 : 1));

                if(g_Settings.ClassicIndicators)
                    Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s", pszIndicator);
                
				lstrcpynA(pszTemp, lin->bIsMe ? g_Settings.pszOutgoingNick : g_Settings.pszIncomingNick, 299);
				p1 = strstr(pszTemp, "%n");
				if(p1)
					p1[1] = 's';
				
                if(!lin->bIsMe) {
                    if(g_Settings.ClickableNicks)
                        Log_Append(&buffer, &bufferEnd, &bufferAlloced, "~~++#");
                    if(g_Settings.ColorizeNicks && pszIndicator[0])
                        Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\cf%u ", OPTIONS_FONTCOUNT + streamData->crCount + crNickIndex + 1);
                }
                
                Log_AppendRTF(streamData, &buffer, &bufferEnd, &bufferAlloced, pszTemp, lin->pszNick);
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, " ");
			}

			// Insert the message
			{
				i = lin->bIsHighlighted?16:EventToIndex(lin);
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(i, i));
				streamData->lin = lin;
				AddEventToBuffer(&buffer, &bufferEnd, &bufferAlloced, streamData);
			}

		}
		lin = lin->prev;
	}
	// ### RTF END
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}");
	return buffer;
}


static DWORD CALLBACK Log_StreamCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
    LOGSTREAMDATA *lstrdat = (LOGSTREAMDATA *) dwCookie;
 
	if(lstrdat)
	{
		// create the RTF
		if (lstrdat->buffer == NULL) 
		{
			lstrdat->bufferOffset = 0;
			lstrdat->buffer = Log_CreateRTF(lstrdat);
			lstrdat->bufferLen = lstrlenA(lstrdat->buffer);
		}

		// give the RTF to the RE control
		*pcb = min(cb, lstrdat->bufferLen - lstrdat->bufferOffset);
		CopyMemory(pbBuff, lstrdat->buffer + lstrdat->bufferOffset, *pcb);
		lstrdat->bufferOffset += *pcb;

		// free stuff if the streaming operation is complete
		if (lstrdat->bufferOffset == lstrdat->bufferLen) 
		{
			free(lstrdat->buffer);
			lstrdat->buffer = NULL;
		}
	}
	
    return 0;
}

void Log_StreamInEvent(HWND hwndDlg,  LOGINFO* lin, SESSION_INFO* si, BOOL bRedraw, BOOL bPhaseTwo)
{
	EDITSTREAM stream;
	LOGSTREAMDATA streamData;
	CHARRANGE oldsel, sel, newsel;
	POINT point ={0};
	SCROLLINFO scroll;
	WPARAM wp;
	HWND hwndRich;
    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
    
	if(hwndDlg == 0 || lin == 0 || si == 0 || dat == 0)
		return;

	hwndRich = GetDlgItem(hwndDlg, IDC_CHAT_LOG);
	ZeroMemory(&streamData, sizeof(LOGSTREAMDATA));
	streamData.hwnd = hwndRich;
	streamData.si = si;
	streamData.lin = lin;
	streamData.bStripFormat = FALSE;
    streamData.dat = dat;
    
//	bPhaseTwo = bRedraw && bPhaseTwo;
	
	if(bRedraw || si->iType != GCW_CHATROOM || !si->bFilterEnabled || (si->iLogFilterFlags&lin->iType) != 0)
	{
		BOOL bFlag = FALSE;

		ZeroMemory(&stream, sizeof(stream));
		stream.pfnCallback = Log_StreamCallback;
		stream.dwCookie = (DWORD) & streamData;
		scroll.cbSize= sizeof(SCROLLINFO);
		scroll.fMask= SIF_RANGE | SIF_POS|SIF_PAGE;
		GetScrollInfo(GetDlgItem(hwndDlg, IDC_CHAT_LOG), SB_VERT, &scroll);
		SendMessage(hwndRich, EM_GETSCROLLPOS, 0, (LPARAM) &point);

		// do not scroll to bottom if there is a selection
		SendMessage(hwndRich, EM_EXGETSEL, 0, (LPARAM) &oldsel);
		if (oldsel.cpMax != oldsel.cpMin)
			SendMessage(hwndRich, WM_SETREDRAW, FALSE, 0);

		//set the insertion point at the bottom
		sel.cpMin = sel.cpMax = GetRichTextLength(hwndRich);
		SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & sel);

		// fix for the indent... must be a M$ bug
		if(sel.cpMax == 0) 
			bRedraw = TRUE;

		// should the event(s) be appended to the current log
		wp = bRedraw?SF_RTF:SFF_SELECTION|SF_RTF;

		//get the number of pixels per logical inch
		if(bRedraw)
		{
			HDC hdc;
			hdc = GetDC(NULL);
			logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
			logPixelSX = GetDeviceCaps(hdc, LOGPIXELSX);
			ReleaseDC (NULL, hdc);
			SendMessage(hwndRich, WM_SETREDRAW, FALSE, 0);
			bFlag = TRUE;
//			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}

		// stream in the event(s)
		streamData.lin = lin;
		streamData.bRedraw = bRedraw;
		SendMessage(hwndRich, EM_STREAMIN, wp, (LPARAM) & stream);

		// do smileys
		SendMessage(hwndRich, EM_EXGETSEL, (WPARAM)0, (LPARAM)&newsel);
		if (myGlobals.g_SmileyAddAvail && (bRedraw 
						|| (lin->pszText 
			             && lin->iType != GC_EVENT_JOIN
			             && lin->iType != GC_EVENT_NICK
			             && lin->iType != GC_EVENT_ADDSTATUS
			             && lin->iType != GC_EVENT_REMOVESTATUS
						 )))
		{
			SMADD_RICHEDIT3 sm = {0};
            FINDTEXTEXA fi;

//			newsel.cpMin = newsel.cpMax - lstrlenA(lin->pszText) - 10;
			newsel.cpMin = sel.cpMin;
			if(newsel.cpMin < 0)
				newsel.cpMin = 0;


            if(g_Settings.ClickableNicks) {
                CHARFORMAT2 cf2 = {0};
                FINDTEXTEXA fi2;

                fi2.lpstrText = " ";
                fi.chrg.cpMin = bRedraw ? 0 : sel.cpMin;
                fi.chrg.cpMax = -1;
                fi.lpstrText = "~~++#";
                cf2.cbSize = sizeof(cf2);

                while(SendMessageA(hwndRich, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) > -1) {
                    SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM)&fi.chrgText);
                    SendMessage(hwndRich, EM_REPLACESEL, TRUE, (LPARAM)_T(""));
                    fi2.chrg.cpMin = fi.chrgText.cpMin;
                    fi2.chrg.cpMax = -1;

                    if(SendMessageA(hwndRich, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi2) > -1) {
                        fi2.chrgText.cpMin = fi.chrgText.cpMin;
                        fi2.chrgText.cpMax--;
                        SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM)&fi2.chrgText);
                        cf2.dwMask = CFM_LINK;
                        cf2.dwEffects = CFE_LINK;
                        SendMessage(hwndRich, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
                    }
                    fi.chrg.cpMin = fi.chrgText.cpMax;
                }
            }
            
			ZeroMemory(&sm, sizeof(sm));
			sm.cbSize = sizeof(sm);
			sm.hwndRichEditControl = hwndRich;
			sm.Protocolname = si->pszModule;
			sm.rangeToReplace = bRedraw?NULL:&newsel;
			sm.disableRedraw = TRUE;
            sm.hContact = si->hContact;
			CallService(MS_SMILEYADD_REPLACESMILEYS, 0, (LPARAM)&sm);

		}

        if(si->wasTrimmed) {
            char szPattern[50];
            FINDTEXTEXA fi;

            _snprintf(szPattern, 40, "~-+%d+-~", si->pLogEnd);
            fi.lpstrText = szPattern;
            fi.chrg.cpMin = 0;
            fi.chrg.cpMax = -1;
            if(SendMessageA(hwndRich, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) != 0) {
                CHARRANGE sel;
                sel.cpMin = 0;
                sel.cpMax = 20;
                SendMessage(hwndRich, EM_SETSEL, 0, fi.chrgText.cpMax + 1);
                SendMessageA(hwndRich, EM_REPLACESEL, TRUE, (LPARAM)"");
            }
            si->wasTrimmed = FALSE;
        }

		// scroll log to bottom if the log was previously scrolled to bottom, else restore old position
		if (bRedraw ||  (UINT)scroll.nPos >= (UINT)scroll.nMax-scroll.nPage-5 || scroll.nMax-scroll.nMin-scroll.nPage < 50)
		{ 
			SendMessage(GetParent(hwndRich), GC_SCROLLTOBOTTOM, 0, 0);
		}
		else 
			SendMessage(hwndRich, EM_SETSCROLLPOS, 0, (LPARAM) &point);

		// do we need to restore the selection
		if (oldsel.cpMax != oldsel.cpMin)
		{
			SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & oldsel);
			SendMessage(hwndRich, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(hwndRich, NULL, TRUE);
		}

		// need to invalidate the window
		if(bFlag)
		{
			sel.cpMin = sel.cpMax = GetRichTextLength(hwndRich);
			SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & sel);
			SendMessage(hwndRich, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(hwndRich, NULL, TRUE);
		}
		

	}

}

#if defined( _UNICODE )
	#define RTF_FORMAT "{\\f%u\\fnil\\fcharset%u %S;}"
#else
	#define RTF_FORMAT "{\\f%u\\fnil\\fcharset%u %s;}"
#endif

char * Log_CreateRtfHeader(MODULEINFO * mi)
{
	char *buffer;
    int bufferAlloced, bufferEnd, i = 0;

	// guesstimate amount of memory for the RTF header
    bufferEnd = 0;
	bufferAlloced = 4096;
	buffer = (char *) realloc(mi->pszHeader, bufferAlloced);
	buffer[0] = '\0';


	//get the number of pixels per logical inch
	if(logPixelSY == 0) {
		HDC hdc;
		hdc = GetDC(NULL);
		logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
		logPixelSX = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(NULL, hdc);
	}

	// ### RTF HEADER

	// font table
    Log_Append(&buffer, &bufferEnd, &bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");
	for (i = 0; i < OPTIONS_FONTCOUNT ; i++)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, RTF_FORMAT, i, aFonts[i].lf.lfCharSet, aFonts[i].lf.lfFaceName);

	// colour table
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}{\\colortbl ;");

    for (i = 0; i < OPTIONS_FONTCOUNT; i++)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(aFonts[i].color), GetGValue(aFonts[i].color), GetBValue(aFonts[i].color));

	for(i = 0; i < mi->nColorCount; i++)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(mi->crColors[i]), GetGValue(mi->crColors[i]), GetBValue(mi->crColors[i]));

    for(i = 0; i < 5; i++)
        Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(g_Settings.nickColors[i]), GetGValue(g_Settings.nickColors[i]), GetBValue(g_Settings.nickColors[i]));
    
    // new paragraph
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}\\pard\\sl%d", 1000);

	// set tabs and indents
	{ 
		int iIndent = 0;

        if(g_Settings.LogSymbols) {
            char szString[2];
            LOGFONT lf;
            HFONT hFont;
            int iText;
            
            szString[1] = 0;
            szString[0] = 0x28;
            LoadMsgDlgFont(17, &lf, NULL, CHAT_FONTMODULE);
            hFont = CreateFontIndirect(&lf);
            iText = GetTextPixelSize(szString, hFont, TRUE) + 3;
            DeleteObject(hFont);
            iIndent += (iText * 1440)/logPixelSX;
            Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent);
        } else if(g_Settings.dwIconFlags) {
			iIndent += ((g_Settings.ScaleIcons ? 14 : 20) * 1440)/logPixelSX;
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent);
		}
		if(g_Settings.ShowTime)
		{
			int iSize = (g_Settings.LogTextIndent*1440)/logPixelSX;
 			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent + iSize );
			if(g_Settings.LogIndentEnabled)
				iIndent += iSize;
		}
/*
		{ // text indent
		int iSize = (135*1440)/logPixelSX;
 		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent + iSize );
			if(g_Settings.LogIndentEnabled)
				iIndent += iSize;

		}
*/
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\fi-%u\\li%u", iIndent, iIndent);
	}
	return buffer;
}
#define RTFPICTHEADERMAXSIZE   78
void LoadMsgLogBitmaps(void)
{
	HICON hIcon;
	HBITMAP hBmp, hoBmp;
	HDC hdc, hdcMem;
	BITMAPINFOHEADER bih = { 0 };
	int widthBytes, i;
	RECT rc;
	HBRUSH hBkgBrush;
	int rtfHeaderSize;
	PBYTE pBmpBits;
    int iIconSize;
    int sizeX = 0, sizeY = 0;

    if(hIcons[0])
		GetIconSize(hIcons[0], &sizeX, &sizeY);
	else
		sizeX = 16;

    if(sizeX >= 12)
        iIconSize = g_Settings.ScaleIcons ? 12 : 16;
    else
        iIconSize = sizeX;

	hBkgBrush = CreateSolidBrush(DBGetContactSettingDword(NULL, "Chat", "ColorLogBG", GetSysColor(COLOR_WINDOW)));
	bih.biSize = sizeof(bih);
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biHeight = iIconSize;
	bih.biPlanes = 1;
	bih.biWidth = iIconSize;
	widthBytes = ((bih.biWidth * bih.biBitCount + 31) >> 5) * 4;
	rc.top = rc.left = 0;
	rc.right = iIconSize;
	rc.bottom = iIconSize;
	hdc = GetDC(NULL);
	hBmp = CreateCompatibleBitmap(hdc, bih.biWidth, bih.biHeight);
	hdcMem = CreateCompatibleDC(hdc);

	pBmpBits = (PBYTE) malloc(widthBytes * bih.biHeight);
	for (i = 0; i < sizeof(pLogIconBmpBits) / sizeof(pLogIconBmpBits[0]); i++) {
		hIcon = hIcons[i];
		pLogIconBmpBits[i] = (PBYTE) malloc(RTFPICTHEADERMAXSIZE + (bih.biSize + widthBytes * bih.biHeight) * 2);
		rtfHeaderSize = sprintf(pLogIconBmpBits[i], "{\\pict\\dibitmap0\\wbmbitspixel%u\\wbmplanes1\\wbmwidthbytes%u\\picw%u\\pich%u ", bih.biBitCount, widthBytes, bih.biWidth, bih.biHeight);
		hoBmp = (HBITMAP) SelectObject(hdcMem, hBmp);

        FillRect(hdcMem, &rc, hBkgBrush);
		DrawIconEx(hdcMem, 0, 1, hIcon, iIconSize, iIconSize, 0, NULL, DI_NORMAL);
        
		SelectObject(hdcMem, hoBmp);
		GetDIBits(hdc, hBmp, 0, bih.biHeight, pBmpBits, (BITMAPINFO *) & bih, DIB_RGB_COLORS);
		{
			int n;
			for (n = 0; n < sizeof(BITMAPINFOHEADER); n++)
				sprintf(pLogIconBmpBits[i] + rtfHeaderSize + n * 2, "%02X", ((PBYTE) & bih)[n]);
			for (n = 0; n < widthBytes * bih.biHeight; n += 4)
				sprintf(pLogIconBmpBits[i] + rtfHeaderSize + (bih.biSize + n) * 2, "%02X%02X%02X%02X", pBmpBits[n], pBmpBits[n + 1], pBmpBits[n + 2], pBmpBits[n + 3]);
		}
		logIconBmpSize[i] = rtfHeaderSize + (bih.biSize + widthBytes * bih.biHeight) * 2 + 1;
		pLogIconBmpBits[i][logIconBmpSize[i] - 1] = '}';
	}
	free(pBmpBits);
	DeleteDC(hdcMem);
	DeleteObject(hBmp);
	ReleaseDC(NULL, hdc);
	DeleteObject(hBkgBrush);

    /* cache RTF font headers */

    //get the number of pixels per logical inch
    if(logPixelSY == 0) {
        HDC hdc;
        hdc = GetDC(NULL);
        logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
        logPixelSX = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
    }

    for(i = 0; i < OPTIONS_FONTCOUNT; i++)
        mir_snprintf(CHAT_rtfFontsGlobal[i], RTFCACHELINESIZE, "\\f%u\\cf%u\\ul0\\highlight0\\b%d\\i%d\\fs%u", i, i + 1, aFonts[i].lf.lfWeight >= FW_BOLD ? 1 : 0, aFonts[i].lf.lfItalic, 2 * abs(aFonts[i].lf.lfHeight) * 74 / logPixelSY);
    CHAT_rtffonts = &(CHAT_rtfFontsGlobal[0][0]);
    mir_snprintf(szMicroLFeed, sizeof(szMicroLFeed), "%s\\par\\sl-1%s", Log_SetStyle(0, 0), Log_SetStyle(0, 0));
}

void FreeMsgLogBitmaps(void)
{
    int i;
    for (i = 0; i < sizeof(pLogIconBmpBits) / sizeof(pLogIconBmpBits[0]); i++)
        free(pLogIconBmpBits[i]);
}
