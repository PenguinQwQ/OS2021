#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t L=0;
  for (;s[L++]!='\0';);
  return L-1;
}

char *strcpy(char* dst,const char* src) {
  size_t i=0;
  for (;src[i]!='\0';i++)
     dst[i]=src[i];
  dst[i]='\0';
  return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
  size_t i=0;
  for (;i<n && src[i]!='\0';i++)
     dst[i]=src[i];
  for (;i<n;i++) dst[i]='\0';
  return dst;
}

char* strcat(char* dst, const char* src) {
  size_t len=strlen(dst),i=0;
  for (;src[i]!='\0';i++)
     *(dst+len+i)=src[i];
  *(dst+len+i)='\0';
  return dst;
}

int strcmp(const char* s1, const char* s2) {
  size_t i=0;
  for (;s1[i]==s2[i] && s1[i]!='\0';i++);
  return (int)(s1[i])-(int)(s2[i]);
}

int strncmp(const char* s1, const char* s2, size_t n) {
  size_t i=0;
  for (;i<n && s1[i]==s2[i] && s1[i]!='\0';i++);
  return (i==n?0:(int)s1[i]-(int)s2[i]);
}

void* memset(void* v,int c,size_t n) {
  char *s=(char*)v;
  for (size_t i=0;i<n;i++){
     *(s+i)=c;
  }
  return v;
}

void* memmove(void* dst,const void* src,size_t n) {
  char *a=(char*)src,*b=(char*)dst;
  size_t i;
  if (a<=b)
     for (i=n-1;~i;i--)
        *(b+i)=*(a+i);
  else
     for (i=0;i<n;i++)
        *(b+i)=*(a+i);
  return dst;
}

void* memcpy(void* out, const void* in, size_t n) {
  //putch('h');
  char *a=(char*)in,*b=(char*)out;
  //putch('h');
  size_t i=0;
  for (;i<n;i++) {
     //putch('a');
     *(b+i)=*(a+i);
  }
  //putch('e');
  //putch('\n');
  return out;
}

int memcmp(const void* s1, const void* s2, size_t n) {
  char *a=(char*)s1,*b=(char*)s2;
  size_t i=0;
  for (;i<n && *(a+i)==*(b+i);i++);
  return (i==n?0:(int)*(a+i)-(int)*(b+i));
}

#endif
