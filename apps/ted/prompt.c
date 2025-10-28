#include <string.h>
#include <stdio.h>
#include "ted.h"

void prompt_draw(tui_t *tui, prompt_t *p)
{
    int tlen, blen;
    int soff, bf, be;

    //offsets and lengths
    blen = 0;
    tlen = 8 + strlen(p->text);
    for(int i = 0; i < p->nbtn; i++)
    {
        blen += 6 + strlen(p->btn[i].text);
    }

    soff = (tui->cols - tlen) / 2;
    bf   = tlen - blen;
    be   = (bf / 2) + (bf % 2);
    bf   = bf / 2;

    // colors
    printf("\e[100;30m");

    // top
    printf("\e[10;%dH", soff);
    printf("┌");
    for(int i = 0; i < tlen; i++) printf("─");
    printf("┐");

    // text line
    printf("\e[11;%dH", soff);
    printf("│    %s    │", p->text);

    // empty line
    printf("\e[12;%dH", soff);
    printf("│%*s│", tlen, "");

    // buttons
    printf("\e[13;%dH", soff);
    printf("│%*s", bf, "");
    for(int i = 0; i < p->nbtn; i++)
    {
        if(p->btn[i].active)
        {
            printf(" \e[97m[ %s ]\e[30m ", p->btn[i].text);
        }
        else
        {
            printf(" [ %s ] ", p->btn[i].text);
        }
    }
    printf("%*s│", be, "");

    // bottom
    printf("\e[14;%dH", soff);
    printf("└");
    for(int i = 0; i < tlen; i++) printf("─");
    printf("┘");

    // reset colors and flush
    printf("\e[0m");
    fflush(stdout);
}

void prompt_left(prompt_t *p)
{
    for(int i = 0; i < p->nbtn; i++)
    {
        if(p->btn[i].active)
        {
            if(i > 0)
            {
                p->btn[i-1].active = 1;
                p->btn[i].active = 0;
            }
            return;
        }
    }
}

void prompt_right(prompt_t *p)
{
    for(int i = 0; i < p->nbtn; i++)
    {
        if(p->btn[i].active)
        {
            if(i+1 < p->nbtn)
            {
                p->btn[i+1].active = 1;
                p->btn[i].active = 0;
            }
            return;
        }
    }
}
