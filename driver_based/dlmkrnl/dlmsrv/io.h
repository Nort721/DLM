#pragma once

#include <Windows.h>
#include <fltUser.h>
#include <iostream>

#pragma comment (lib, "FltLib.lib")

typedef struct _HASHCHECK
{
    ULONG hash;
    BOOLEAN approved;
} HASHCHECK, * PHASHCHECK;

typedef struct _MESSAGE_REPLY
{
    FILTER_REPLY_HEADER head;
    HASHCHECK hashcheck;
} MESSAGE_REPLY, * PMESSAGE_REPLY;

typedef struct _MESSAGE
{
    FILTER_MESSAGE_HEADER head;
    HASHCHECK hashcheck;
} MESSAGE, * PMESSAGE;

BOOLEAN SendPendingGet(HANDLE hDriverPort);