/* Local source copy: each mod is independently loadable. */
#include "api.h"
#include "../../runtime/native/input.h"
#include "../../runtime/native/local_actor.h"
#include "payment.h"
#include "../build-menu-framework/sdk/construction_unity.h"

static struct {
    PTR log,world_field,alive,restoring,focused,typing,blocked,first,frame,delta;
    PTR joined,host,local,player,wallet,decimal_int,decimal_compare,decimal_subtract,charge,string_equal;
    PTR camera,transform,position,forward;
    void (*set_field)(PTR,PTR,void *);
    int (*value_type)(PTR);
    short (*key)(int);PTR (*foreground)(void);DWORD (*pid)(PTR,DWORD *);
    BOOL (*title)(PTR,const char *);
    DRoot world,walker;int last_frame,eligible,host_ready,disabled;DWORD thread;
    unsigned keys[4];CTPayment payments;
} bm;
static void bevent(const char *s){char b[512],*p=cat(b,"{\"event\":\"");p=cat(p,s);p=cat(p,"\"}\n");DWORD n=0;WriteFile(bm.log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(bm.log);}
static PTR bfield(PTR p,const char *name){return moon_build_field(p,name);}
static int bint(PTR p,const char *name){return moon_build_int(p,name);}
static int bhost(void){int ok=1;if(!boolean(bm.joined,0,&ok))return ok;U64 a=0,b=0;PTR h=call(bm.host,0,0,&ok);if(h)dcopy(&a,unbox(h),8);PTR l=call(bm.local,0,0,&ok);if(l)dcopy(&b,unbox(l),8);return ok&&a&&a==b;}
static int baction(const char *name,int key){return moon_input_key_action(name,(unsigned)key,1).pressed;}
static int bcontext(PTR world,PTR walker){
    if(bm.disabled||!world||!walker||!moon_local_actor_admit(walker))return 0;
    int ok=1;PTR actual=0;field_value(walker,bm.world_field,&actual);
    if(actual!=world)return 0;
    void *wa[]={world};PTR live=call(bm.alive,0,wa,&ok);
    if(!live||!*(unsigned char *)unbox(live)||!ok)return 0;
    void *pa[]={walker};live=call(bm.alive,0,pa,&ok);
    return live&&*(unsigned char *)unbox(live)&&!boolean(bm.restoring,world,&ok)&&bhost()&&ok;
}
#include "payment_unity.h"
static int bframe(Context *context){
    if(bm.disabled||!moon_local_actor_admit((PTR)context->rcx))return 0;if(!bm.thread)bm.thread=GetCurrentThreadId();if(bm.thread!=GetCurrentThreadId())return 0;
    int ok=1,frame=integer(bm.frame,0,&ok);if(!ok||frame==bm.last_frame)return 0;bm.last_frame=frame;
    PTR walker=(PTR)context->rcx,world=0;field_value(walker,bm.world_field,&world);
    if(world!=dget(bm.world)||walker!=dget(bm.walker)){ddrop(&bm.world);ddrop(&bm.walker);bm.world=dhold(world);bm.walker=dhold(walker);bm.eligible=bm.host_ready=0;bm.payments.blocked=0;return -1;}
    DWORD pid=0;PTR window=bm.foreground();if(window)bm.pid(window,&pid);
    bm.host_ready=bcontext(world,walker);
    bm.eligible=bm.host_ready&&pid==GetCurrentProcessId()&&boolean(bm.focused,0,&ok)&&!boolean(bm.typing,0,&ok)&&!boolean(bm.blocked,walker,&ok)&&boolean(bm.first,walker,&ok)&&ok;
    return 1;
}
static void bstatus(const char *s){DWORD pid=0;PTR w=bm.foreground();if(w)bm.pid(w,&pid);if(pid==GetCurrentProcessId())bm.title(w,s);}
static int baim(DVec *origin,DVec *direction){int ok=1;DRoot camera=dhold(call(bm.camera,0,0,&ok)),tr=camera?dhold(call(bm.transform,dget(camera),0,&ok)):0;if(tr){*origin=dvector(bm.position,dget(tr),&ok);*direction=dvector(bm.forward,dget(tr),&ok);}else ok=0;ddrop(&tr);ddrop(&camera);return ok;}
static float bscore(DVec origin,DVec direction,DVec point){return moon_build_target_score((MoonBuildPoint){origin.x,origin.y,origin.z},(MoonBuildPoint){direction.x,direction.y,direction.z},(MoonBuildPoint){point.x,point.y,point.z},8,.8f,.005f,1e8f);}
static DRoot benum(PTR collection,int *ok){return moon_build_enumerate(collection,ok);}
static PTR bnext(DRoot iterator,int *ok){return moon_build_next(iterator,bm.value_type,ok);}
static void bend(DRoot *it){moon_build_end(it,bm.value_type);}
static int bstart(PTR domain,PTR game,const char *id){
    if(!bind_runtime(domain,game)||!dapi(game)||!moon_local_actor_bind())return 0;
    char folder[1000],path[1200];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));if(!n||n>=sizeof(folder))return 0;char *p=cat(path,folder);p=cat(p,"\\");p=cat(p,id);cat(p,".jsonl");bm.log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(bm.log==(PTR)(U64)-1)return 0;
#define BM(F,NS,T,N,S) bm.F=dmethod(NS,T,N,S);if(!bm.F){bevent(T "::" N);return 0;}
    BM(alive,"UnityEngine","Object","op_Implicit","UnityEngine.Object");BM(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");BM(focused,"UnityEngine","Application","get_isFocused","");BM(typing,"MoonCleaners","MoonInput","get_Typing","");BM(blocked,"MoonCleaners","MoonWalker","get_MenuBlocked","");BM(first,"MoonCleaners","MoonWalker","get_IsFirstPerson","");BM(frame,"UnityEngine","Time","get_frameCount","");BM(delta,"UnityEngine","Time","get_deltaTime","");
    BM(joined,"MoonCleaners","MoonCrew","get_Joined","");BM(host,"MoonCleaners","MoonCrew","get_HostId","");BM(local,"Steamworks","SteamUser","GetSteamID","");BM(player,"MoonCleaners","MoonCrew","get_LocalPlayerId","");BM(wallet,"MoonCleaners","MoonWorld","WalletDollars","System.String");
#undef BM
    bm.string_equal=dmethod("System","String","Equals","System.String;System.String");
    bm.decimal_compare=dmethod("System","Decimal","Compare","System.Decimal;System.Decimal");bm.decimal_subtract=dmethod("System","Decimal","Subtract","System.Decimal;System.Decimal");
    bm.decimal_int=dmethod("System","Decimal",".ctor","System.Int32");bm.charge=dmethod("MoonCleaners","MoonWorld","ChargeBuild","System.String;System.Decimal");bm.camera=dmethod("UnityEngine","Camera","get_main","");bm.transform=dmethod("UnityEngine","Component","get_transform","");bm.position=dmethod("UnityEngine","Transform","get_position","");bm.forward=dmethod("UnityEngine","Transform","get_forward","");
    bm.value_type=(int(*)(PTR))GetProcAddress(game,"il2cpp_class_is_valuetype");bm.world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");bm.set_field=(void(*)(PTR,PTR,void *))GetProcAddress(game,"il2cpp_field_set_value");PTR user=LoadLibraryA("user32.dll");bm.key=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");bm.foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");bm.pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");bm.title=(BOOL(*)(PTR,const char *))GetProcAddress(user,"SetWindowTextA");bm.last_frame=-1;
    (void)baction;(void)dzero;(void)dtype;(void)darray;(void)dnumber;(void)bint;(void)bfield;(void)bcharge;(void)bstatus;(void)baim;(void)bscore;(void)benum;(void)bnext;(void)bend;
    return bm.value_type&&bm.world_field&&bm.set_field&&bm.key&&bm.foreground&&bm.pid&&bm.title&&bm.decimal_int&&bm.decimal_compare&&bm.decimal_subtract&&bm.string_equal&&bm.charge&&bm.camera&&bm.transform&&bm.position&&bm.forward;
}
