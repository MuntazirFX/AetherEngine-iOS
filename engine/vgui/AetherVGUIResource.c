#include "AetherVGUIResource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char*skip(const char*p){while(*p&&((unsigned char)*p<=32||*p==';'))p++;return p;}
static const char*token(const char*p,char*out,size_t cap){p=skip(p);if(!*p){if(cap)out[0]=0;return p;}char q=0;if(*p=='"'||*p=='\''){q=*p++;}size_t n=0;while(*p&&((q&&*p!=q)||(!q&&(unsigned char)*p>32&&*p!='{'&&*p!='}'))){if(*p=='/'&&p[1]=='/')break;if(n+1<cap)out[n++]=*p;p++;}if(q&&*p==q)p++;out[n]=0;return p;}
void aether_vgui_resource_init(aether_vgui_resource_t*r){if(r)memset(r,0,sizeof*r);}
aether_result_t aether_vgui_resource_parse(aether_vgui_resource_t*r,const char*t){if(!r||!t)return AETHER_ERR_INVALID_ARG;aether_vgui_resource_init(r);const char*p=t;char k[96],v[256];while(*p&&r->count<AETHER_VGUI_RESOURCE_MAX){p=token(p,k,sizeof k);if(!*k)break;p=skip(p);if(*p=='{'){p++;continue;}p=token(p,v,sizeof v);if(!*v){p++;continue;}aether_str_copy(r->items[r->count].key,sizeof r->items[0].key,k);aether_str_copy(r->items[r->count].value,sizeof r->items[0].value,v);r->count++;}return AETHER_OK;}
const char*aether_vgui_resource_get(const aether_vgui_resource_t*r,const char*k,const char*f){if(!r||!k)return f;for(u32 i=0;i<r->count;i++)if(aether_str_eq(r->items[i].key,k))return r->items[i].value;return f;}
aether_result_t aether_vgui_resource_load_file(aether_vgui_resource_t*r,const char*path){if(!r||!path)return AETHER_ERR_INVALID_ARG;FILE*f=fopen(path,"rb");if(!f)return AETHER_ERR_IO;if(fseek(f,0,SEEK_END)!=0){fclose(f);return AETHER_ERR_IO;}long n=ftell(f);if(n<0||n>1024*1024){fclose(f);return AETHER_ERR_IO;}rewind(f);char*b=malloc((size_t)n+1);if(!b){fclose(f);return AETHER_ERR_OUT_OF_MEM;}size_t got=fread(b,1,(size_t)n,f);fclose(f);b[got]=0;aether_result_t rc=aether_vgui_resource_parse(r,b);free(b);return rc;}
