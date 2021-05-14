#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int vsprintf(char *out, const char *fmt, va_list ap) {
  return 0;
}

#define print_()\
  va_list args;\
  char *s,sd[102];\
  int d,L2;\
  char s2[102];\
  va_start(args,fmt);\
  size_t p=0;\
  for (size_t i=0;fmt[i]!='\0';) {\
     if (fmt[i]=='%') {\
        i++;\
        for (L2=0;'0'<=fmt[i]&&fmt[i]<='9';s2[L2++]=fmt[i++]);\
        char pc=' ';\
        if (s2[0]=='0') pc='0';\
        int r=0;\
        for (int i=0;i<L2;i++) r=r*10+s2[i]-'0';\
        size_t l=0;\
        int op=0;\
        if (fmt[i]=='d') {\
           d=va_arg(args,int);\
           if (d<0) d=-d,op=1;\
           if (!d) *(sd+l++)='0';\
           else for (;d;d/=10) *(sd+l++)=d%10+'0';\
           if (op) *(sd+l++)='-';\
           sd[l]='\0';\
           if (l<r) for (int j=r-l;j--;)*(out+p++)=pc;\
           for (size_t j=l-1;~j;j--) *(out+p++)=sd[j];\
        }\
        else if (fmt[i]=='x'||fmt[i]=='p') {\
           unsigned d=va_arg(args,unsigned);\
           if (!d) *(sd+l++)='0';\
           else for (;d;d/=16) *(sd+l++)=(d%16<10?d%16+'0':d%16-10+'a');\
           sd[l]='\0';\
           if (l<r) for (int j=r-l;j--;)*(out+p++)=pc;\
           for (size_t j=l-1;~j;j--) *(out+p++)=sd[j];\
        }\
        else if (fmt[i]=='u') {\
           unsigned d=va_arg(args,unsigned);\
           if (!d) *(sd+l++)='0';\
           else for (;d;d/=10) *(sd+l++)=d%10+'0';\
           sd[l]='\0';\
           if (l<r) for (int j=r-l;j--;)*(out+p++)=pc;\
           for (size_t j=l-1;~j;j--) *(out+p++)=sd[j];\
        }\
        else if (fmt[i]=='c') {\
           char c;\
           c=va_arg(args,int);\
           *(out+p++)=c;\
        }\
        else if (fmt[i]=='s') {\
           s=va_arg(args,char*);\
           l=strlen(s);\
           if (l<r) for (int j=r-l;j--;)*(out+p++)=pc;\
           for (size_t j=0;j<l;j++) *(out+p++)=s[j];\
        }\
        i++;\
     }\
     else if (fmt[i]=='\\') {\
        i++;\
        if (fmt[i]=='n') *(out+p++)='\n';\
        i++;\
     }\
     else {\
        *(out+p++)=fmt[i++];\
     }\
  }\
  *(out+p)='\0';\
  va_end(args);

int sprintf(char *out, const char *fmt, ...) {
  print_()
  return p;
}


int printf(const char *fmt, ...) {
  static char out[65536];
  size_t k=0;
  print_()
  for (;*(out+k)!='\0';putch(*(out+k++))) {
      //if (*(out+k)=='0') for(;;);
  }
  return p;
}


int snprintf(char *out, size_t n, const char *fmt, ...) {
  return 0;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  return 0;
}

#endif
