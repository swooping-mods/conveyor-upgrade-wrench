#include "../../runtime/native/gameplay.h"
#include "building.h"
#include "config.h"
#include "../shop-framework/client.h"
#include "../hints-framework/sdk/client.h"
static const MoonShop *fw_shop;
static const MoonHints *fw_hints;
static int fw_started;
int _fltused=0;
static int purchased,active,upgrades[256],tiers[256],count,hammer,unlock_price,price2,price3,key_buy,key_use,key_mode;
static PTR conveyors,path,nearest,riders_field,route_point,holding,component,player_upgrades,views,material,set_color,cancel;
static int upgraded(int id){for(int i=0;i<count;i++)if(upgrades[i]==id)return tiers[i];return 0;}
static void recolor(void){
 int ok=1;PTR network=call(views,dget(bm.world),0,&ok),dict=bfield(network,"views");if(!dict)return;PTR get=get_method(object_class(dict),"get_Item",1),has=get_method(object_class(dict),"ContainsKey",1);
 for(int i=0;i<count&&ok;i++){void *a[]={upgrades+i};PTR exists=call(has,dict,a,&ok);if(!exists||!*(unsigned char *)unbox(exists))continue;DRoot view=dhold(call(get,dict,a,&ok));PTR renderer=bfield(dget(view),"renderer");if(renderer){PTR mat=call(material,renderer,0,&ok);DColor c=tiers[i]==2?(DColor){.12f,.48f,1,1}:(DColor){1,.42f,.06f,1};void *b[]={&c};if(mat)call(set_color,mat,b,&ok);}ddrop(&view);}
}
static PTR builder(int *ok){void *a[]={dtype("MoonCleaners","MoonDepotBuilder")};return call(component,dget(bm.walker),a,ok);}
static int maximum_tier(void){int ok=1;PTR id=call(bm.player,0,0,&ok);void *a[]={id};DRoot snapshot=dhold(call(player_upgrades,dget(bm.world),a,&ok));int tier=bint(dget(snapshot),"inventoryTier");ddrop(&snapshot);return ok&&tier>=2?3:2;}

static void accelerate(void){
    int ok=1;float dt=dnumber(bm.delta,0,&ok);if(!ok||dt<=0||dt>.1f)return;
    PTR riders=0;field_value(dget(bm.world),riders_field,&riders);if(!riders)return;
    /* Snapshot keys first: Dictionary.set_Item invalidates live enumeration. */
    int keys[512],n=0;PTR collection=call(get_method(object_class(riders),"get_Keys",0),riders,0,&ok);DRoot it=benum(collection,&ok);PTR item;
    while(n<512&&(item=bnext(it,&ok)))keys[n++]=*(int *)unbox(item);bend(&it);if(!ok)return;
    PTR get=get_method(object_class(riders),"get_Item",1),set=get_method(object_class(riders),"set_Item",2);
    for(int i=0;i<n;i++){
        void *a[]={keys+i};DRoot boxed=dhold(call(get,riders,a,&ok));PTR v=dget(boxed);if(!v)continue;
        if(upgraded(bint(v,"belt"))){PTR lag=get_field(object_class(v),"lag");float value=0;if(lag){field_value(v,lag,&value);if(value>-.5f&&value<2){value+=(upgraded(bint(v,"belt"))==3?3.3f:1.1f)*dt;bm.set_field(v,lag,&value);void *b[]={keys+i,unbox(v)};call(set,riders,b,&ok);}}}
        ddrop(&boxed);if(!ok){bm.disabled=1;bevent("rider-update-failed");return;}
    }
}
static void upgrade(void){
    DVec origin,direction;if(!baim(&origin,&direction)||count>=256)return;
    int maximum=maximum_tier();int ok=1;DRoot records=dhold(call(conveyors,dget(bm.world),0,&ok)),it=benum(dget(records),&ok);PTR record;int chosen=0;float best=.65f;
    DVec point={origin.x+direction.x*3,origin.y+direction.y*3,origin.z+direction.z*3};
    for(int i=0;i<4096&&(record=bnext(it,&ok));i++){
        int id=bint(record,"id");if(upgraded(id)>=maximum)continue;void *a[]={record};DRoot route=dhold(call(path,dget(bm.world),a,&ok));void *b[]={dget(route),&point};PTR distance=call(nearest,0,b,&ok);float along=distance?*(float *)unbox(distance):0;void *pa[]={&along};PTR sampled=call(route_point,dget(route),pa,&ok);DVec p={0};if(sampled)dcopy(&p,unbox(sampled),sizeof(p));float d=moon_build_distance2((MoonBuildPoint){p.x,p.y,p.z},(MoonBuildPoint){point.x,point.y,point.z});if(sampled&&d<best){best=d;chosen=id;}ddrop(&route);
    }bend(&it);ddrop(&records);int current=upgraded(chosen);if(chosen&&ok&&bcharge(current>=2?price3:price2)){int index=0;while(index<count&&upgrades[index]!=chosen)index++;if(index==count)upgrades[count++]=chosen;tiers[index]=current>=2?3:2;recolor();bevent(tiers[index]==3?"conveyor-mk3-orange":"conveyor-mk2-blue");}
}
static int fw_context(PTR world,PTR walker){int ok=1;return world==dget(bm.world)&&walker==dget(bm.walker)&&world&&!boolean(bm.restoring,world,&ok)&&bhost()&&ok&&!bm.disabled;}
static unsigned fw_state(PTR world,PTR walker){return fw_context(world,walker)?MS_AVAILABLE|(purchased?MS_OWNED:0):MS_BLOCKED;}
static int fw_purchase(PTR world,PTR walker){
 if(fw_state(world,walker)!=MS_AVAILABLE)return 0;int ok=1;
 if(!boolean(bm.focused,0,&ok)||boolean(bm.typing,0,&ok)||!ok)return 0;
 if(!bcharge(unlock_price))return 0;purchased=1;bevent("conveyor-engineering-unlocked");return 1;
}
static int fw_bind(PTR domain,PTR game){
 fw_shop=ms_connect(domain,game);fw_hints=mh_connect(domain,game);if(!fw_shop||!fw_hints)return 0;
 MoonShopDefinition offer={MS_ABI,sizeof(offer),"conveyor-upgrade-wrench","Conveyor Engineering","Unlock the hammer's in-place conveyor Upgrade action.",unlock_price,fw_state,fw_purchase};
 return fw_shop->register_item(&offer);
}
static char *fw_key(char *p,const char *name,int key){char label[64];if(moon_input_key_label(name,(unsigned)key,label,sizeof(label)))return cat(p,label);if(key>='A'&&key<='Z'){*p++=(char)key;*p=0;return p;}if(key>=0x70&&key<=0x87)return num(cat(p,"F"),(unsigned)(key-0x6f));return num(cat(p,"VK"),(unsigned)key);}
static void fw_controls(PTR walker,int held){
 char text[220],*p=text;
 if(hammer||active){p=fw_key(p,"conveyor-upgrade-wrench.upgrade",key_use);p=cat(p,": Upgrade aimed belt | ");p=fw_key(p,hammer?"conveyor-upgrade-wrench.mode":"conveyor-upgrade-wrench.toggle",hammer?key_mode:key_buy);cat(p,hammer?": Leave hammer upgrade mode":": Put away wrench");}
 else if(held&&purchased){p=fw_key(p,"conveyor-upgrade-wrench.mode",key_mode);p=cat(p,": Hammer upgrade mode | ");p=fw_key(p,"conveyor-upgrade-wrench.toggle",key_buy);cat(p,": Conveyor wrench");}
 else{fw_hints->clear("conveyor-upgrade-wrench");return;}
 fw_hints->publish("conveyor-upgrade-wrench",walker,text);
}
static int before(int id,Context *c){
 (void)id;int state=bframe(c);int toggle=baction("conveyor-upgrade-wrench.toggle",key_buy),use=baction("conveyor-upgrade-wrench.upgrade",key_use),mode=baction("conveyor-upgrade-wrench.mode",key_mode);if(state<0){fw_hints->clear("conveyor-upgrade-wrench");count=0;purchased=active=hammer=0;bevent("session-reset");return 0;}if(state<=0){fw_hints->clear("conveyor-upgrade-wrench");return 0;}
 if(bm.host_ready){accelerate();if(bm.last_frame%30==0)recolor();}if(!bm.eligible){fw_hints->clear("conveyor-upgrade-wrench");return 0;}
 int ok=1;PTR build=builder(&ok);int held=build&&boolean(holding,build,&ok);
 if(toggle&&purchased){active=!active;hammer=0;}
 if(mode&&held&&purchased){hammer=!hammer;active=0;if(hammer)call(cancel,build,0,&ok);bevent(hammer?"hammer-upgrade-action":"hammer-upgrade-ended");}
 if(hammer&&!held)hammer=0;if((active||hammer)&&use)upgrade();
 fw_controls(dget(bm.walker),held);return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(fw_started)return 1;
    if(!bstart(domain,game,"conveyor-upgrade-wrench"))return 0;
    cfg_start("MOONLAB_CONVEYOR_TIERS_CONFIG","MoonLab\\mods\\conveyor-upgrade-wrench\\settings.ini");unlock_price=cfg("UnlockPrice",125,0,10000);price2=cfg("Tier2Price",25,0,10000);price3=cfg("Tier3Price",75,0,10000);key_buy=cfg("UnlockKey",0x79,1,255);key_use=cfg("UpgradeKey",'E',1,255);key_mode=cfg("HammerModeKey",'U',1,255);
    route_point=dmethod("MoonCleaners","MoonConveyorPath","Point","System.Single");holding=dmethod("MoonCleaners","MoonDepotBuilder","get_HoldingBuildTool","");component=dmethod("UnityEngine","Component","GetComponent","System.Type");player_upgrades=dmethod("MoonCleaners","MoonWorld","PlayerUpgrades","System.String");views=dmethod("MoonCleaners","MoonWorld","get_ConveyorViews","");material=dmethod("UnityEngine","Renderer","get_material","");set_color=dmethod("UnityEngine","Material","set_color","UnityEngine.Color");cancel=dmethod("MoonCleaners","MoonDepotBuilder","CancelPlacement","");
    if(!route_point||!holding||!component||!player_upgrades||!views||!material||!set_color||!cancel)return 0;
    conveyors=dmethod("MoonCleaners","MoonWorld","get_BuiltConveyors","");path=dmethod("MoonCleaners","MoonWorld","ConveyorPath","MoonCleaners.MoonConveyorSnapshot");nearest=dmethod("MoonCleaners","MoonWorld","NearestDistance","MoonCleaners.MoonConveyorPath;UnityEngine.Vector3");riders_field=get_field(find_class("MoonCleaners","MoonWorld"),"riders");if(!conveyors||!path||!nearest||!riders_field)return 0;
    if(!fw_bind(domain,game)||!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;before_hook=before;if(!install_hooks())return 0;fw_started=1;bevent("ready");return 1;
}
