#pragma once

#include <string.h>


struct errorcode
{
    int code;
    char error_flag[100];
    char function[100];
    char description[100];
};

