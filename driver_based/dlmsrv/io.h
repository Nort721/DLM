#pragma once

#include <Windows.h>
#include <fltUser.h>
#include <iostream>

#pragma comment (lib, "FltLib.lib")

#define REPLY_MESSAGE_SIZE   (sizeof(FILTER_REPLY_HEADER) + sizeof(ULONG))

//typedef struct _MESSAGE_REPLY
//{
//    FILTER_REPLY_HEADER head;
//    BOOLEAN approved;
//} MESSAGE_REPLY, * PMESSAGE_REPLY;
//
//typedef struct _MESSAGE
//{
//    FILTER_MESSAGE_HEADER head;
//    ULONG hash;
//} MESSAGE, * PMESSAGE;

typedef struct _MESSAGE_REPLY
{
    BOOLEAN approved;
} MESSAGE_REPLY, * PMESSAGE_REPLY;

typedef struct _MESSAGE
{
    ULONG hash;
} MESSAGE, * PMESSAGE;

typedef struct _MESSAGE_REPLY_UM : public FILTER_REPLY_HEADER, public _MESSAGE_REPLY
{
} MESSAGE_REPLY_UM, * PMESSAGE_REPLY_UM;

typedef struct _MESSAGE_UM : public FILTER_MESSAGE_HEADER, public _MESSAGE
{
} MESSAGE_UM, * PMESSAGE_UM;

BOOLEAN SendPendingGet(HANDLE hDriverPort);