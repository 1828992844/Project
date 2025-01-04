/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* czi_JSON */
/* JSON parser in C. */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "czi_JSON.h"


static const char *global_ep;

const char *czi_JSON_GetErrorPtr(void) {return global_ep;}

static int czi_JSON_strcasecmp(const char *s1,const char *s2)
{
    if (!s1) return (s1==s2)?0:1;if (!s2) return 1;
    for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)    if(*s1 == 0)    return 0;
    return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*czi_JSON_malloc)(size_t sz) = malloc;
static void (*czi_JSON_free)(void *ptr) = free;


static char* czi_JSON_strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1;
      if (!(copy = (char*)czi_JSON_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
}

void czi_JSON_InitHooks(czi_JSON_Hooks* hooks)
{
    if (!hooks) { /* Reset hooks */
        czi_JSON_malloc = malloc;
        czi_JSON_free = free;
        return;
    }

    czi_JSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
    czi_JSON_free     = (hooks->free_fn)?hooks->free_fn:free;
}

/* Internal constructor. */
static czi_JSON *czi_JSON_New_Item(void)
{
    czi_JSON* node = (czi_JSON*)czi_JSON_malloc(sizeof(czi_JSON));
    if (node) memset(node,0,sizeof(czi_JSON));
    return node;
}

/* Delete a czi_JSON structure. */
void czi_JSON_Delete(czi_JSON *c)
{
    czi_JSON *next;
    while (c)
    {
        next=c->next;
        if (!(c->type&czi_JSON_IsReference) && c->child) czi_JSON_Delete(c->child);
        if (!(c->type&czi_JSON_IsReference) && c->valuestring) czi_JSON_free(c->valuestring);
        if (!(c->type&czi_JSON_StringIsConst) && c->string) czi_JSON_free(c->string);
        czi_JSON_free(c);
        c=next;
    }
}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number(czi_JSON *item,const char *num)
{
    double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

    if (*num=='-') sign=-1,num++;    /* Has sign? */
    if (*num=='0') num++;            /* is zero */
    if (*num>='1' && *num<='9')    do    n=(n*10.0)+(*num++ -'0');    while (*num>='0' && *num<='9');    /* Number? */
    if (*num=='.' && num[1]>='0' && num[1]<='9') {num++;        do    n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}    /* Fractional part? */
    if (*num=='e' || *num=='E')        /* Exponent? */
    {    num++;if (*num=='+') num++;    else if (*num=='-') signsubscale=-1,num++;        /* With sign? */
        while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');    /* Number? */
    }

    n=sign*n*pow(10.0,(scale+subscale*signsubscale));    /* number = +/- number.fraction * 10^+/- exponent */
    
    item->valuedouble=n;
    item->valueint=(int)n;
    item->type=czi_JSON_Number;
    return num;
}

static int pow2gt (int x)    {    --x;    x|=x>>1;    x|=x>>2;    x|=x>>4;    x|=x>>8;    x|=x>>16;    return x+1;    }

typedef struct {char *buffer; int length; int offset; } printbuffer;

static char* ensure(printbuffer *p,int needed)
{
    char *newbuffer;int newsize;
    if (!p || !p->buffer) return 0;
    needed+=p->offset;
    if (needed<=p->length) return p->buffer+p->offset;

    newsize=pow2gt(needed);
    newbuffer=(char*)czi_JSON_malloc(newsize);
    if (!newbuffer) {czi_JSON_free(p->buffer);p->length=0,p->buffer=0;return 0;}
    if (newbuffer) memcpy(newbuffer,p->buffer,p->length);
    czi_JSON_free(p->buffer);
    p->length=newsize;
    p->buffer=newbuffer;
    return newbuffer+p->offset;
}

static int update(printbuffer *p)
{
    char *str;
    if (!p || !p->buffer) return 0;
    str=p->buffer+p->offset;
    return p->offset+strlen(str);
}

/* Render the number nicely from the given item into a string. */
static char *print_number(czi_JSON *item,printbuffer *p)
{
    char *str=0;
    double d=item->valuedouble;
    if (d==0)
    {
        if (p)    str=ensure(p,2);
        else    str=(char*)czi_JSON_malloc(2);    /* special case for 0. */
        if (str) strcpy(str,"0");
    }
    else if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
    {
        if (p)    str=ensure(p,21);
        else    str=(char*)czi_JSON_malloc(21);    /* 2^64+1 can be represented in 21 chars. */
        if (str)    sprintf(str,"%d",item->valueint);
    }
    else
    {
        if (p)    str=ensure(p,64);
        else    str=(char*)czi_JSON_malloc(64);    /* This is a nice tradeoff. */
        if (str)
        {
            if (d*0!=0)                                                    sprintf(str,"null");    /* This checks for NaN and Infinity */
            else if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60)    sprintf(str,"%.0f",d);
            else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)                    sprintf(str,"%e",d);
            else                                                        sprintf(str,"%f",d);
        }
    }
    return str;
}

static unsigned parse_hex4(const char *str)
{
    unsigned h=0;
    if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
    h=h<<4;str++;
    if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
    h=h<<4;str++;
    if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
    h=h<<4;str++;
    if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
    return h;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(czi_JSON *item,const char *str,const char **ep)
{
    const char *ptr=str+1,*end_ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
    if (*str!='\"') {*ep=str;return 0;}    /* not a string! */

    while (*end_ptr!='\"' && *end_ptr && ++len)
    {
        if (*end_ptr++ == '\\')
        {
        if (*end_ptr == '\0')
        {
            /* prevent buffer overflow when last input character is a backslash */
            return 0;
        }
        end_ptr++;    /* Skip escaped quotes. */
        }
    }

    out=(char*)czi_JSON_malloc(len+1);    /* This is how long we need for the string, roughly. */
    if (!out) return 0;
    item->valuestring=out; /* assign here so out will be deleted during czi_JSON_Delete() later */
    item->type=czi_JSON_String;
    
    ptr=str+1;ptr2=out;
    while (ptr < end_ptr)
    {
        if (*ptr!='\\') *ptr2++=*ptr++;
        else
        {
            ptr++;
            switch (*ptr)
            {
                case 'b': *ptr2++='\b';    break;
                case 'f': *ptr2++='\f';    break;
                case 'n': *ptr2++='\n';    break;
                case 'r': *ptr2++='\r';    break;
                case 't': *ptr2++='\t';    break;
                case 'u':     /* transcode utf16 to utf8. */
                    uc=parse_hex4(ptr+1);ptr+=4;    /* get the unicode char. */
                    if (ptr >= end_ptr) {*ep=str;return 0;}    /* invalid */
                    
                    if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)    {*ep=str;return 0;}    /* check for invalid.   */
                    
                    if (uc>=0xD800 && uc<=0xDBFF)    /* UTF16 surrogate pairs.    */
                    {
                        if (ptr+6 > end_ptr)    {*ep=str;return 0;}    /* invalid */
                        if (ptr[1]!='\\' || ptr[2]!='u')    {*ep=str;return 0;}    /* missing second-half of surrogate.    */
                        uc2=parse_hex4(ptr+3);ptr+=6;
                        if (uc2<0xDC00 || uc2>0xDFFF)       {*ep=str;return 0;}    /* invalid second-half of surrogate.    */
                        uc=0x10000 + (((uc&0x3FF)<<10) | (uc2&0x3FF));
                    }

                    len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;
                    
                    switch (len) {
                        case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 1: *--ptr2 =(uc | firstByteMark[len]);
                    }
                    ptr2+=len;
                    break;
                default:  *ptr2++=*ptr; break;
            }
            ptr++;
        }
    }
    *ptr2=0;
    if (*ptr=='\"') ptr++;
    return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str,printbuffer *p)
{
    const char *ptr;char *ptr2,*out;int len=0,flag=0;unsigned char token;

    if (!str)
    {
        if (p)    out=ensure(p,3);
        else    out=(char*)czi_JSON_malloc(3);
        if (!out) return 0;
        strcpy(out,"\"\"");
        return out;
    }
    
    for (ptr=str;*ptr;ptr++) flag|=((*ptr>0 && *ptr<32)||(*ptr=='\"')||(*ptr=='\\'))?1:0;
    if (!flag)
    {
        len=ptr-str;
        if (p) out=ensure(p,len+3);
        else        out=(char*)czi_JSON_malloc(len+3);
        if (!out) return 0;
        ptr2=out;*ptr2++='\"';
        strcpy(ptr2,str);
        ptr2[len]='\"';
        ptr2[len+1]=0;
        return out;
    }
    
    ptr=str;while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;}
    
    if (p)    out=ensure(p,len+3);
    else    out=(char*)czi_JSON_malloc(len+3);
    if (!out) return 0;

    ptr2=out;ptr=str;
    *ptr2++='\"';
    while (*ptr)
    {
        if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
        else
        {
            *ptr2++='\\';
            switch (token=*ptr++)
            {
                case '\\':    *ptr2++='\\';    break;
                case '\"':    *ptr2++='\"';    break;
                case '\b':    *ptr2++='b';    break;
                case '\f':    *ptr2++='f';    break;
                case '\n':    *ptr2++='n';    break;
                case '\r':    *ptr2++='r';    break;
                case '\t':    *ptr2++='t';    break;
                default: sprintf(ptr2,"u%04x",token);ptr2+=5;    break;    /* escape and print */
            }
        }
    }
    *ptr2++='\"';*ptr2++=0;
    return out;
}
/* Invote print_string_ptr (which is useful) on an item. */
static char *print_string(czi_JSON *item,printbuffer *p)    {return print_string_ptr(item->valuestring,p);}

/* Predeclare these prototypes. */
static const char *parse_value(czi_JSON *item,const char *value,const char **ep);
static char *print_value(czi_JSON *item,int depth,int fmt,printbuffer *p);
static const char *parse_array(czi_JSON *item,const char *value,const char **ep);
static char *print_array(czi_JSON *item,int depth,int fmt,printbuffer *p);
static const char *parse_object(czi_JSON *item,const char *value,const char **ep);
static char *print_object(czi_JSON *item,int depth,int fmt,printbuffer *p);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in) {while (in && *in && (unsigned char)*in<=32) in++; return in;}

/* Parse an object - create a new root, and populate. */
czi_JSON *czi_JSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated)
{
    const char *end=0,**ep=return_parse_end?return_parse_end:&global_ep;
    czi_JSON *c=czi_JSON_New_Item();
    *ep=0;
    if (!c) return 0;       /* memory fail */

    end=parse_value(c,skip(value),ep);
    if (!end)    {czi_JSON_Delete(c);return 0;}    /* parse failure. ep is set. */

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated) {end=skip(end);if (*end) {czi_JSON_Delete(c);*ep=end;return 0;}}
    if (return_parse_end) *return_parse_end=end;
    return c;
}
/* Default options for czi_JSON_Parse */
czi_JSON *czi_JSON_Parse(const char *value) {return czi_JSON_ParseWithOpts(value,0,0);}

/* Render a czi_JSON item/entity/structure to text. */
char *czi_JSON_Print(czi_JSON *item)                {return print_value(item,0,1,0);}
char *czi_JSON_PrintUnformatted(czi_JSON *item)    {return print_value(item,0,0,0);}

char *czi_JSON_PrintBuffered(czi_JSON *item,int prebuffer,int fmt)
{
    printbuffer p;
    p.buffer=(char*)czi_JSON_malloc(prebuffer);
    p.length=prebuffer;
    p.offset=0;
    return print_value(item,0,fmt,&p);
}


/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(czi_JSON *item,const char *value,const char **ep)
{
    if (!value)                        return 0;    /* Fail on null. */
    if (!strncmp(value,"null",4))    { item->type=czi_JSON_NULL;  return value+4; }
    if (!strncmp(value,"false",5))    { item->type=czi_JSON_False; return value+5; }
    if (!strncmp(value,"true",4))    { item->type=czi_JSON_True; item->valueint=1;    return value+4; }
    if (*value=='\"')                { return parse_string(item,value,ep); }
    if (*value=='-' || (*value>='0' && *value<='9'))    { return parse_number(item,value); }
    if (*value=='[')                { return parse_array(item,value,ep); }
    if (*value=='{')                { return parse_object(item,value,ep); }

    *ep=value;return 0;    /* failure. */
}

/* Render a value to text. */
static char *print_value(czi_JSON *item,int depth,int fmt,printbuffer *p)
{
    char *out=0;
    if (!item)
        return 0;
    if (p)
    {
        switch ((item->type)&255)
        {
            case czi_JSON_NULL:    {out=ensure(p,5);    if (out) strcpy(out,"null");    break;}
            case czi_JSON_False:    {out=ensure(p,6);    if (out) strcpy(out,"false");    break;}
            case czi_JSON_True:    {out=ensure(p,5);    if (out) strcpy(out,"true");    break;}
            case czi_JSON_Number:    out=print_number(item,p);break;
            case czi_JSON_String:    out=print_string(item,p);break;
            case czi_JSON_Array:    out=print_array(item,depth,fmt,p);break;
            case czi_JSON_Object:    out=print_object(item,depth,fmt,p);break;
        }
    }
    else
    {
        switch ((item->type)&255)
        {
            case czi_JSON_NULL:    out=czi_JSON_strdup("null");    break;
            case czi_JSON_False:    out=czi_JSON_strdup("false");break;
            case czi_JSON_True:    out=czi_JSON_strdup("true"); break;
            case czi_JSON_Number:    out=print_number(item,0);break;
            case czi_JSON_String:    out=print_string(item,0);break;
            case czi_JSON_Array:    out=print_array(item,depth,fmt,0);break;
            case czi_JSON_Object:    out=print_object(item,depth,fmt,0);break;
        }
    }
    return out;
}

/* Build an array from input text. */
static const char *parse_array(czi_JSON *item,const char *value,const char **ep)
{
    czi_JSON *child;
    if (*value!='[')    {*ep=value;return 0;}    /* not an array! */

    item->type=czi_JSON_Array;
    value=skip(value+1);
    if (*value==']') return value+1;    /* empty array. */

    item->child=child=czi_JSON_New_Item();
    if (!item->child) return 0;         /* memory fail */
    value=skip(parse_value(child,skip(value),ep));    /* skip any spacing, get the value. */
    if (!value) return 0;

    while (*value==',')
    {
        czi_JSON *new_item;
        if (!(new_item=czi_JSON_New_Item())) return 0;     /* memory fail */
        child->next=new_item;new_item->prev=child;child=new_item;
        value=skip(parse_value(child,skip(value+1),ep));
        if (!value) return 0;    /* memory fail */
    }

    if (*value==']') return value+1;    /* end of array */
    *ep=value;return 0;    /* malformed. */
}

/* Render an array to text */
static char *print_array(czi_JSON *item,int depth,int fmt,printbuffer *p)
{
    char **entries;
    char *out=0,*ptr,*ret;int len=5;
    czi_JSON *child=item->child;
    int numentries=0,i=0,fail=0;
    size_t tmplen=0;
    
    /* How many entries in the array? */
    while (child) numentries++,child=child->next;
    /* Explicitly handle numentries==0 */
    if (!numentries)
    {
        if (p)    out=ensure(p,3);
        else    out=(char*)czi_JSON_malloc(3);
        if (out) strcpy(out,"[]");
        return out;
    }

    if (p)
    {
        /* Compose the output array. */
        i=p->offset;
        ptr=ensure(p,1);if (!ptr) return 0;    *ptr='[';    p->offset++;
        child=item->child;
        while (child && !fail)
        {
            print_value(child,depth+1,fmt,p);
            p->offset=update(p);
            if (child->next) {len=fmt?2:1;ptr=ensure(p,len+1);if (!ptr) return 0;*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;p->offset+=len;}
            child=child->next;
        }
        ptr=ensure(p,2);if (!ptr) return 0;    *ptr++=']';*ptr=0;
        out=(p->buffer)+i;
    }
    else
    {
        /* Allocate an array to hold the values for each */
        entries=(char**)czi_JSON_malloc(numentries*sizeof(char*));
        if (!entries) return 0;
        memset(entries,0,numentries*sizeof(char*));
        /* Retrieve all the results: */
        child=item->child;
        while (child && !fail)
        {
            ret=print_value(child,depth+1,fmt,0);
            entries[i++]=ret;
            if (ret) len+=strlen(ret)+2+(fmt?1:0); else fail=1;
            child=child->next;
        }
        
        /* If we didn't fail, try to malloc the output string */
        if (!fail)    out=(char*)czi_JSON_malloc(len);
        /* If that fails, we fail. */
        if (!out) fail=1;

        /* Handle failure. */
        if (fail)
        {
            for (i=0;i<numentries;i++) if (entries[i]) czi_JSON_free(entries[i]);
            czi_JSON_free(entries);
            return 0;
        }
        
        /* Compose the output array. */
        *out='[';
        ptr=out+1;*ptr=0;
        for (i=0;i<numentries;i++)
        {
            tmplen=strlen(entries[i]);memcpy(ptr,entries[i],tmplen);ptr+=tmplen;
            if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;}
            czi_JSON_free(entries[i]);
        }
        czi_JSON_free(entries);
        *ptr++=']';*ptr++=0;
    }
    return out;    
}

/* Build an object from the text. */
static const char *parse_object(czi_JSON *item,const char *value,const char **ep)
{
    czi_JSON *child;
    if (*value!='{')    {*ep=value;return 0;}    /* not an object! */
    
    item->type=czi_JSON_Object;
    value=skip(value+1);
    if (*value=='}') return value+1;    /* empty array. */
    
    item->child=child=czi_JSON_New_Item();
    if (!item->child) return 0;
    value=skip(parse_string(child,skip(value),ep));
    if (!value) return 0;
    child->string=child->valuestring;child->valuestring=0;
    if (*value!=':') {*ep=value;return 0;}    /* fail! */
    value=skip(parse_value(child,skip(value+1),ep));    /* skip any spacing, get the value. */
    if (!value) return 0;
    
    while (*value==',')
    {
        czi_JSON *new_item;
        if (!(new_item=czi_JSON_New_Item()))    return 0; /* memory fail */
        child->next=new_item;new_item->prev=child;child=new_item;
        value=skip(parse_string(child,skip(value+1),ep));
        if (!value) return 0;
        child->string=child->valuestring;child->valuestring=0;
        if (*value!=':') {*ep=value;return 0;}    /* fail! */
        value=skip(parse_value(child,skip(value+1),ep));    /* skip any spacing, get the value. */
        if (!value) return 0;
    }
    
    if (*value=='}') return value+1;    /* end of array */
    *ep=value;return 0;    /* malformed. */
}

/* Render an object to text. */
static char *print_object(czi_JSON *item,int depth,int fmt,printbuffer *p)
{
    char **entries=0,**names=0;
    char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
    czi_JSON *child=item->child;
    int numentries=0,fail=0;
    size_t tmplen=0;
    /* Count the number of entries. */
    while (child) numentries++,child=child->next;
    /* Explicitly handle empty object case */
    if (!numentries)
    {
        if (p) out=ensure(p,fmt?depth+4:3);
        else    out=(char*)czi_JSON_malloc(fmt?depth+4:3);
        if (!out)    return 0;
        ptr=out;*ptr++='{';
        if (fmt) {*ptr++='\n';for (i=0;i<depth;i++) *ptr++='\t';}
        *ptr++='}';*ptr++=0;
        return out;
    }
    if (p)
    {
        /* Compose the output: */
        i=p->offset;
        len=fmt?2:1;    ptr=ensure(p,len+1);    if (!ptr) return 0;
        *ptr++='{';    if (fmt) *ptr++='\n';    *ptr=0;    p->offset+=len;
        child=item->child;depth++;
        while (child)
        {
            if (fmt)
            {
                ptr=ensure(p,depth);    if (!ptr) return 0;
                for (j=0;j<depth;j++) *ptr++='\t';
                p->offset+=depth;
            }
            print_string_ptr(child->string,p);
            p->offset=update(p);
            
            len=fmt?2:1;
            ptr=ensure(p,len);    if (!ptr) return 0;
            *ptr++=':';if (fmt) *ptr++='\t';
            p->offset+=len;
            
            print_value(child,depth,fmt,p);
            p->offset=update(p);

            len=(fmt?1:0)+(child->next?1:0);
            ptr=ensure(p,len+1); if (!ptr) return 0;
            if (child->next) *ptr++=',';
            if (fmt) *ptr++='\n';*ptr=0;
            p->offset+=len;
            child=child->next;
        }
        ptr=ensure(p,fmt?(depth+1):2);     if (!ptr) return 0;
        if (fmt)    for (i=0;i<depth-1;i++) *ptr++='\t';
        *ptr++='}';*ptr=0;
        out=(p->buffer)+i;
    }
    else
    {
        /* Allocate space for the names and the objects */
        entries=(char**)czi_JSON_malloc(numentries*sizeof(char*));
        if (!entries) return 0;
        names=(char**)czi_JSON_malloc(numentries*sizeof(char*));
        if (!names) {czi_JSON_free(entries);return 0;}
        memset(entries,0,sizeof(char*)*numentries);
        memset(names,0,sizeof(char*)*numentries);

        /* Collect all the results into our arrays: */
        child=item->child;depth++;if (fmt) len+=depth;
        while (child && !fail)
        {
            names[i]=str=print_string_ptr(child->string,0);
            entries[i++]=ret=print_value(child,depth,fmt,0);
            if (str && ret)
                len+=strlen(ret)+strlen(str)+2+(fmt?2+depth:0);
            else
                fail=1;
            child=child->next;
        }
        
        /* Try to allocate the output string */
        if (!fail)    out=(char*)czi_JSON_malloc(len);
        if (!out) fail=1;

        /* Handle failure */
        if (fail)
        {
            for (i=0;i<numentries;i++) {if (names[i]) czi_JSON_free(names[i]);if (entries[i]) czi_JSON_free(entries[i]);}
            czi_JSON_free(names);czi_JSON_free(entries);
            return 0;
        }
        
        /* Compose the output: */
        *out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0;
        for (i=0;i<numentries;i++)
        {
            if (fmt) for (j=0;j<depth;j++) *ptr++='\t';
            tmplen=strlen(names[i]);memcpy(ptr,names[i],tmplen);ptr+=tmplen;
            *ptr++=':';if (fmt) *ptr++='\t';
            strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
            if (i!=numentries-1) *ptr++=',';
            if (fmt) *ptr++='\n';*ptr=0;
            czi_JSON_free(names[i]);czi_JSON_free(entries[i]);
        }
        
        czi_JSON_free(names);czi_JSON_free(entries);
        if (fmt) for (i=0;i<depth-1;i++) *ptr++='\t';
        *ptr++='}';*ptr++=0;
    }
    return out;    
}

/* Get Array size/item / object item. */
int    czi_JSON_GetArraySize(czi_JSON *array)                            {czi_JSON *c=array->child;int i=0;while(c)i++,c=c->next;return i;}
czi_JSON *czi_JSON_GetArrayItem(czi_JSON *array,int item)                {czi_JSON *c=array?array->child:0;while (c && item>0) item--,c=c->next; return c;}
czi_JSON *czi_JSON_GetObjectItem(czi_JSON *object,const char *string)    {czi_JSON *c=object?object->child:0;while (c && czi_JSON_strcasecmp(c->string,string)) c=c->next; return c;}
int czi_JSON_HasObjectItem(czi_JSON *object,const char *string)        {return czi_JSON_GetObjectItem(object,string)?1:0;}

/* Utility for array list handling. */
static void suffix_object(czi_JSON *prev,czi_JSON *item) {prev->next=item;item->prev=prev;}
/* Utility for handling references. */
static czi_JSON *create_reference(czi_JSON *item) {czi_JSON *ref=czi_JSON_New_Item();if (!ref) return 0;memcpy(ref,item,sizeof(czi_JSON));ref->string=0;ref->type|=czi_JSON_IsReference;ref->next=ref->prev=0;return ref;}

/* Add item to array/object. */
void   czi_JSON_AddItemToArray(czi_JSON *array, czi_JSON *item)                        {czi_JSON *c=array->child;if (!item) return; if (!c) {array->child=item;} else {while (c && c->next) c=c->next; suffix_object(c,item);}}
void   czi_JSON_AddItemToObject(czi_JSON *object,const char *string,czi_JSON *item)    {if (!item) return; if (item->string) czi_JSON_free(item->string);item->string=czi_JSON_strdup(string);czi_JSON_AddItemToArray(object,item);}
void   czi_JSON_AddItemToObjectCS(czi_JSON *object,const char *string,czi_JSON *item)    {if (!item) return; if (!(item->type&czi_JSON_StringIsConst) && item->string) czi_JSON_free(item->string);item->string=(char*)string;item->type|=czi_JSON_StringIsConst;czi_JSON_AddItemToArray(object,item);}
void    czi_JSON_AddItemReferenceToArray(czi_JSON *array, czi_JSON *item)                        {czi_JSON_AddItemToArray(array,create_reference(item));}
void    czi_JSON_AddItemReferenceToObject(czi_JSON *object,const char *string,czi_JSON *item)    {czi_JSON_AddItemToObject(object,string,create_reference(item));}

czi_JSON *czi_JSON_DetachItemFromArray(czi_JSON *array,int which)            {czi_JSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return 0;
    if (c->prev) c->prev->next=c->next;if (c->next) c->next->prev=c->prev;if (c==array->child) array->child=c->next;c->prev=c->next=0;return c;}
void   czi_JSON_DeleteItemFromArray(czi_JSON *array,int which)            {czi_JSON_Delete(czi_JSON_DetachItemFromArray(array,which));}
czi_JSON *czi_JSON_DetachItemFromObject(czi_JSON *object,const char *string) {int i=0;czi_JSON *c=object->child;while (c && czi_JSON_strcasecmp(c->string,string)) i++,c=c->next;if (c) return czi_JSON_DetachItemFromArray(object,i);return 0;}
void   czi_JSON_DeleteItemFromObject(czi_JSON *object,const char *string) {czi_JSON_Delete(czi_JSON_DetachItemFromObject(object,string));}

/* Replace array/object items with new ones. */
void   czi_JSON_InsertItemInArray(czi_JSON *array,int which,czi_JSON *newitem)        {czi_JSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) {czi_JSON_AddItemToArray(array,newitem);return;}
    newitem->next=c;newitem->prev=c->prev;c->prev=newitem;if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;}
void   czi_JSON_ReplaceItemInArray(czi_JSON *array,int which,czi_JSON *newitem)        {czi_JSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return;
    newitem->next=c->next;newitem->prev=c->prev;if (newitem->next) newitem->next->prev=newitem;
    if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;c->next=c->prev=0;czi_JSON_Delete(c);}
void   czi_JSON_ReplaceItemInObject(czi_JSON *object,const char *string,czi_JSON *newitem){int i=0;czi_JSON *c=object->child;while(c && czi_JSON_strcasecmp(c->string,string))i++,c=c->next;if(c){newitem->string=czi_JSON_strdup(string);czi_JSON_ReplaceItemInArray(object,i,newitem);}}

/* Create basic types: */
czi_JSON *czi_JSON_CreateNull(void)                    {czi_JSON *item=czi_JSON_New_Item();if(item)item->type=czi_JSON_NULL;return item;}
czi_JSON *czi_JSON_CreateTrue(void)                    {czi_JSON *item=czi_JSON_New_Item();if(item)item->type=czi_JSON_True;return item;}
czi_JSON *czi_JSON_CreateFalse(void)                    {czi_JSON *item=czi_JSON_New_Item();if(item)item->type=czi_JSON_False;return item;}
czi_JSON *czi_JSON_CreateBool(int b)                    {czi_JSON *item=czi_JSON_New_Item();if(item)item->type=b?czi_JSON_True:czi_JSON_False;return item;}
czi_JSON *czi_JSON_CreateNumber(double num)            {czi_JSON *item=czi_JSON_New_Item();if(item){item->type=czi_JSON_Number;item->valuedouble=num;item->valueint=(int)num;}return item;}
czi_JSON *czi_JSON_CreateString(const char *string)    {czi_JSON *item=czi_JSON_New_Item();if(item){item->type=czi_JSON_String;item->valuestring=czi_JSON_strdup(string);if(!item->valuestring){czi_JSON_Delete(item);return 0;}}return item;}
czi_JSON *czi_JSON_CreateArray(void)                    {czi_JSON *item=czi_JSON_New_Item();if(item)item->type=czi_JSON_Array;return item;}
czi_JSON *czi_JSON_CreateObject(void)                    {czi_JSON *item=czi_JSON_New_Item();if(item)item->type=czi_JSON_Object;return item;}

/* Create Arrays: */
czi_JSON *czi_JSON_CreateIntArray(const int *numbers,int count)        {int i;czi_JSON *n=0,*p=0,*a=czi_JSON_CreateArray();for(i=0;a && i<count;i++){n=czi_JSON_CreateNumber(numbers[i]);if(!n){czi_JSON_Delete(a);return 0;}if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
czi_JSON *czi_JSON_CreateFloatArray(const float *numbers,int count)    {int i;czi_JSON *n=0,*p=0,*a=czi_JSON_CreateArray();for(i=0;a && i<count;i++){n=czi_JSON_CreateNumber(numbers[i]);if(!n){czi_JSON_Delete(a);return 0;}if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
czi_JSON *czi_JSON_CreateDoubleArray(const double *numbers,int count)    {int i;czi_JSON *n=0,*p=0,*a=czi_JSON_CreateArray();for(i=0;a && i<count;i++){n=czi_JSON_CreateNumber(numbers[i]);if(!n){czi_JSON_Delete(a);return 0;}if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
czi_JSON *czi_JSON_CreateStringArray(const char **strings,int count)    {int i;czi_JSON *n=0,*p=0,*a=czi_JSON_CreateArray();for(i=0;a && i<count;i++){n=czi_JSON_CreateString(strings[i]);if(!n){czi_JSON_Delete(a);return 0;}if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}

/* Duplication */
czi_JSON *czi_JSON_Duplicate(czi_JSON *item,int recurse)
{
    czi_JSON *newitem,*cptr,*nptr=0,*newchild;
    /* Bail on bad ptr */
    if (!item) return 0;
    /* Create new item */
    newitem=czi_JSON_New_Item();
    if (!newitem) return 0;
    /* Copy over all vars */
    newitem->type=item->type&(~czi_JSON_IsReference),newitem->valueint=item->valueint,newitem->valuedouble=item->valuedouble;
    if (item->valuestring)    {newitem->valuestring=czi_JSON_strdup(item->valuestring);    if (!newitem->valuestring)    {czi_JSON_Delete(newitem);return 0;}}
    if (item->string)        {newitem->string=czi_JSON_strdup(item->string);            if (!newitem->string)        {czi_JSON_Delete(newitem);return 0;}}
    /* If non-recursive, then we're done! */
    if (!recurse) return newitem;
    /* Walk the ->next chain for the child. */
    cptr=item->child;
    while (cptr)
    {
        newchild=czi_JSON_Duplicate(cptr,1);        /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild) {czi_JSON_Delete(newitem);return 0;}
        if (nptr)    {nptr->next=newchild,newchild->prev=nptr;nptr=newchild;}    /* If newitem->child already set, then crosswire ->prev and ->next and move on */
        else        {newitem->child=newchild;nptr=newchild;}                    /* Set newitem->child and move to it */
        cptr=cptr->next;
    }
    return newitem;
}

void czi_JSON_Minify(char *json)
{
    char *into=json;
    while (*json)
    {
        if (*json==' ') json++;
        else if (*json=='\t') json++;    /* Whitespace characters. */
        else if (*json=='\r') json++;
        else if (*json=='\n') json++;
        else if (*json=='/' && json[1]=='/')  while (*json && *json!='\n') json++;    /* double-slash comments, to end of line. */
        else if (*json=='/' && json[1]=='*') {while (*json && !(*json=='*' && json[1]=='/')) json++;json+=2;}    /* multiline comments. */
        else if (*json=='\"'){*into++=*json++;while (*json && *json!='\"'){if (*json=='\\') *into++=*json++;*into++=*json++;}*into++=*json++;} /* string literals, which are \" sensitive. */
        else *into++=*json++;            /* All other characters. */
    }
    *into=0;    /* and null-terminate. */
}

int czi_JSON_IsArray(const czi_JSON *const item) {
    return item && item->type == czi_JSON_Array;
}

int czi_JSON_IsObject(const czi_JSON *const item) {
    return item && item->type == czi_JSON_Object;
}
