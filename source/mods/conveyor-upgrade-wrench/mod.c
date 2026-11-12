#include "../../runtime/native/gameplay.h"
#include "building.h"
#include "config.h"
#include "../shop-framework/client.h"
#include "../hints-framework/sdk/client.h"
static const MoonShop *fw_shop;
static const MoonHints *fw_hints;
static int fw_started;
int _fltused=0;
static int purchased,active,hammer,rider_overflow,unlock_price,price2,price3,key_buy,key_use,key_mode;
static PTR conveyors,path,nearest,riders_field,route_point,holding,component,player_upgrades,views;
static PTR shared_material,set_shared_material,set_color,instantiate,destroy,cancel,try_conveyor;
static PTR moving,removing,placing;
static DRoot ct_snapshot(int id,int *ok){
    PTR snapshot=0;void *a[]={&id,&snapshot};PTR result=call(try_conveyor,dget(bm.world),a,ok);
    if(!*ok||!result){*ok=0;return 0;}
    if(!*(unsigned char *)unbox(result))return 0;
    DRoot root=dhold(snapshot);if(!root)*ok=0;return root;
}
#include "tiers.h"
#include "presentation_unity.h"
static void ct_paint(CTEntry *entry,PTR renderer,int *ok){
    if(renderer!=dget(entry->renderer)&&!ct_visual_drop(entry)){*ok=0;return;}
    if(!entry->paint){
        entry->renderer=dhold(renderer);
        entry->original=ct_material(renderer,ok);
        if(!*ok||!entry->renderer||!entry->original){ct_visual_drop(entry);return;}
        void *a[]={dget(entry->original)};entry->paint=dhold(call(instantiate,0,a,ok));
        if(!*ok||!entry->paint){ct_visual_drop(entry);return;}
        void *b[]={dget(entry->paint)};call(set_shared_material,dget(entry->renderer),b,ok);
    }
    DColor color=entry->tier==2?(DColor){.12f,.48f,1,1}:(DColor){1,.42f,.06f,1};
    void *a[]={&color};if(*ok)call(set_color,dget(entry->paint),a,ok);
    if(!*ok)ct_visual_drop(entry);
}
static int recolor(void){
    int ok=1;DRoot network=dhold(call(views,dget(bm.world),0,&ok));
    DRoot dict=dhold(bfield(dget(network),"views"));
    if(!ok||!dict){ddrop(&dict);ddrop(&network);return ok;}
    PTR get=get_method(object_class(dget(dict)),"get_Item",1),has=get_method(object_class(dget(dict)),"ContainsKey",1);
    for(int i=0;i<ct_count&&ok;i++){
        CTEntry *entry=ct_entries+i;void *a[]={&entry->id};PTR exists=call(has,dget(dict),a,&ok);
        if(!ok||!exists)break;
        if(!*(unsigned char *)unbox(exists)){if(!ct_visual_drop(entry))ok=0;continue;}
        DRoot view=dhold(call(get,dget(dict),a,&ok));DRoot renderer=dhold(bfield(dget(view),"renderer"));
        if(ok&&ct_alive(dget(renderer),&ok))ct_paint(entry,dget(renderer),&ok);else if(!ct_visual_drop(entry))ok=0;
        ddrop(&renderer);ddrop(&view);
    }
    ddrop(&dict);ddrop(&network);return ok;
}
static PTR builder(int *ok){void *a[]={dtype("MoonCleaners","MoonDepotBuilder")};return call(component,dget(bm.walker),a,ok);}
static int maximum_tier(void){
    int ok=1;DRoot id=dhold(call(bm.player,0,0,&ok));void *a[]={dget(id)};
    DRoot snapshot=dhold(call(player_upgrades,dget(bm.world),a,&ok));
    int tier=bint(dget(snapshot),"inventoryTier");ddrop(&snapshot);ddrop(&id);
    return ok&&tier>=2?3:2;
}
static void accelerate(void){
    int ok=1;float dt=dnumber(bm.delta,0,&ok);if(!ok||dt<=0||dt>.1f)return;
    PTR borrowed=0;field_value(dget(bm.world),riders_field,&borrowed);DRoot riders=dhold(borrowed);if(!riders)return;
    /* Snapshot keys first: Dictionary.set_Item invalidates live enumeration. */
    int total=integer(get_method(object_class(dget(riders)),"get_Count",0),dget(riders),&ok);
    if(!ok||total<0){ddrop(&riders);bm.disabled=1;bevent("rider-count-failed");return;}
    if(total>512){if(!rider_overflow)bevent("rider-capacity-suspended");rider_overflow=1;ddrop(&riders);return;}
    if(rider_overflow)bevent("rider-capacity-resumed");rider_overflow=0;
    int keys[512],n=0;DRoot collection=dhold(call(get_method(object_class(dget(riders)),"get_Keys",0),dget(riders),0,&ok));
    DRoot it=benum(dget(collection),&ok);PTR item;
    while(n<512&&(item=bnext(it,&ok)))keys[n++]=*(int *)unbox(item);
    bend(&it);ddrop(&collection);if(!ok){ddrop(&riders);return;}
    PTR get=get_method(object_class(dget(riders)),"get_Item",1),set=get_method(object_class(dget(riders)),"set_Item",2);
    for(int i=0;i<n;i++){
        void *a[]={keys+i};DRoot boxed=dhold(call(get,dget(riders),a,&ok));PTR value=dget(boxed);
        if(!ok||!boxed){ddrop(&boxed);break;}
        int tier=upgraded(bint(value,"belt"));
        if(tier){PTR lag=get_field(object_class(value),"lag");float credit=0;
            if(lag){field_value(value,lag,&credit);if(credit>-.5f&&credit<2){
                credit+=(tier==3?3.3f:1.1f)*dt;bm.set_field(value,lag,&credit);
                void *b[]={keys+i,unbox(dget(boxed))};call(set,dget(riders),b,&ok);
            }}
        }
        ddrop(&boxed);if(!ok)break;
    }
    ddrop(&riders);if(!ok){bm.disabled=1;bevent("rider-update-failed");}
}
static void upgrade(void){
    if(rider_overflow)return;
    DVec origin,direction;if(!baim(&origin,&direction))return;
    int maximum=maximum_tier(),ok=1;
    DRoot records=dhold(call(conveyors,dget(bm.world),0,&ok)),it=benum(dget(records),&ok),chosen=0;
    PTR borrowed;float best=.65f;int chosen_id=0;
    DVec point={origin.x+direction.x*3,origin.y+direction.y*3,origin.z+direction.z*3};
    for(int i=0;i<4096&&ok&&(borrowed=bnext(it,&ok));i++){
        DRoot record=dhold(borrowed);if(!record){ok=0;break;}
        int id=bint(dget(record),"id");
        if(upgraded(id)<maximum&&(ct_index(id)>=0||ct_count<CT_LIMIT)){
            void *a[]={dget(record)};DRoot route=dhold(call(path,dget(bm.world),a,&ok));
            if(ok&&route){
                void *b[]={dget(route),&point};PTR distance=call(nearest,0,b,&ok);
                float along=distance?*(float *)unbox(distance):0;void *pa[]={&along};
                PTR sampled=distance?call(route_point,dget(route),pa,&ok):0;DVec p={0};
                if(sampled)dcopy(&p,unbox(sampled),sizeof(p));
                float d=moon_build_distance2((MoonBuildPoint){p.x,p.y,p.z},(MoonBuildPoint){point.x,point.y,point.z});
                if(ok&&sampled&&d<best){ddrop(&chosen);chosen=dhold(dget(record));chosen_id=id;best=d;if(!chosen)ok=0;}
            }
            ddrop(&route);
        }
        ddrop(&record);
    }
    bend(&it);ddrop(&records);
    int tier=ok&&chosen?ct_promote(chosen_id,chosen,maximum,price2,price3):0;
    ddrop(&chosen);if(tier){if(!recolor())bm.disabled=1;bevent(tier==3?"conveyor-mk3-orange":"conveyor-mk2-blue");}
}
static int fw_context(PTR world,PTR walker){return world==dget(bm.world)&&walker==dget(bm.walker)&&bcontext(world,walker)&&!rider_overflow&&!bm.payments.blocked&&!bm.payments.busy;}
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
 if(rider_overflow&&purchased){fw_hints->publish("conveyor-upgrade-wrench",walker,"Conveyor upgrades paused: more than 512 native riders");return;}
 char text[220],*p=text;
 if(hammer||active){p=fw_key(p,"conveyor-upgrade-wrench.upgrade",key_use);p=cat(p,": Upgrade aimed belt | ");p=fw_key(p,hammer?"conveyor-upgrade-wrench.mode":"conveyor-upgrade-wrench.toggle",hammer?key_mode:key_buy);cat(p,hammer?": Leave hammer upgrade mode":": Put away wrench");}
 else if(held&&purchased){p=fw_key(p,"conveyor-upgrade-wrench.mode",key_mode);p=cat(p,": Hammer upgrade mode | ");p=fw_key(p,"conveyor-upgrade-wrench.toggle",key_buy);cat(p,": Conveyor wrench");}
 else{fw_hints->clear("conveyor-upgrade-wrench");return;}
 fw_hints->publish("conveyor-upgrade-wrench",walker,text);
}
static int before(int id,Context *c){
    (void)id;
    /* Remote walker updates must not consume this actor's frame or key edges. */
    PTR local=0;if(!moon_local_actor_read(&local))return 0;
    if(!local){if(!ct_reset())bm.disabled=1;purchased=active=hammer=rider_overflow=0;fw_hints->clear("conveyor-upgrade-wrench");return 0;}
    if(local!=(PTR)c->rcx)return 0;
    if(bm.disabled){ct_reset();fw_hints->clear("conveyor-upgrade-wrench");return 0;}
    int state=bframe(c);
    if(state<0){fw_hints->clear("conveyor-upgrade-wrench");if(!ct_reset())bm.disabled=1;purchased=active=hammer=rider_overflow=0;bevent("session-reset");return 0;}
    if(state<=0){fw_hints->clear("conveyor-upgrade-wrench");return 0;}
    int toggle=baction("conveyor-upgrade-wrench.toggle",key_buy),use=baction("conveyor-upgrade-wrench.upgrade",key_use),mode=baction("conveyor-upgrade-wrench.mode",key_mode);
    if(bm.host_ready){
        if(!ct_prune()){bm.disabled=1;ct_reset();bevent("tier-identity-check-failed");return 0;}
        accelerate();if(!bm.disabled&&bm.last_frame%30==0&&!recolor())bm.disabled=1;
    }else if(ct_count||purchased){if(!ct_reset())bm.disabled=1;purchased=active=hammer=rider_overflow=0;bevent("host-context-lost");}
    if(bm.disabled)ct_reset();
    if(!bm.eligible||bm.disabled||bm.payments.blocked){fw_hints->clear("conveyor-upgrade-wrench");return 0;}
    int ok=1;DRoot build=dhold(builder(&ok));int held=build&&boolean(holding,dget(build),&ok);
    int conflict=build&&(boolean(moving,dget(build),&ok)||boolean(removing,dget(build),&ok));
    if(conflict||!ok){active=hammer=0;fw_hints->clear("conveyor-upgrade-wrench");ddrop(&build);return 0;}
    if(toggle&&purchased){active=!active;hammer=0;}
    if(mode&&held&&purchased){
        if(hammer)hammer=0;
        else{call(cancel,dget(build),0,&ok);hammer=ok&&!boolean(placing,dget(build),&ok)&&ok;}
        active=0;bevent(hammer?"hammer-upgrade-action":"hammer-upgrade-ended");
    }
    if(hammer&&!held)hammer=0;
    if(build&&boolean(placing,dget(build),&ok))active=hammer=0;
    if(ok&&(active||hammer)&&use)upgrade();
    fw_controls(dget(bm.walker),held);ddrop(&build);return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(fw_started)return 1;
    if(!bstart(domain,game,"conveyor-upgrade-wrench"))return 0;
    cfg_start("MOONLAB_CONVEYOR_TIERS_CONFIG","MoonLab\\mods\\conveyor-upgrade-wrench\\settings.ini");unlock_price=cfg("UnlockPrice",125,0,10000);price2=cfg("Tier2Price",25,0,10000);price3=cfg("Tier3Price",75,0,10000);key_buy=cfg("UnlockKey",0x79,1,255);key_use=cfg("UpgradeKey",'E',1,255);key_mode=cfg("HammerModeKey",'U',1,255);
    route_point=dmethod("MoonCleaners","MoonConveyorPath","Point","System.Single");holding=dmethod("MoonCleaners","MoonDepotBuilder","get_HoldingBuildTool","");component=dmethod("UnityEngine","Component","GetComponent","System.Type");player_upgrades=dmethod("MoonCleaners","MoonWorld","PlayerUpgrades","System.String");views=dmethod("MoonCleaners","MoonWorld","get_ConveyorViews","");shared_material=dmethod("UnityEngine","Renderer","get_sharedMaterial","");set_shared_material=dmethod("UnityEngine","Renderer","set_sharedMaterial","UnityEngine.Material");instantiate=dmethod("UnityEngine","Object","Instantiate","UnityEngine.Object");destroy=dmethod("UnityEngine","Object","Destroy","UnityEngine.Object");set_color=dmethod("UnityEngine","Material","set_color","UnityEngine.Color");cancel=dmethod("MoonCleaners","MoonDepotBuilder","CancelPlacement","");
    moving=dmethod("MoonCleaners","MoonDepotBuilder","get_IsMoving","");removing=dmethod("MoonCleaners","MoonDepotBuilder","get_IsRemoving","");placing=dmethod("MoonCleaners","MoonDepotBuilder","get_IsPlacing","");try_conveyor=dmethod("MoonCleaners","MoonWorld","TryGetConveyor","System.Int32;MoonCleaners.MoonConveyorSnapshot&");
    PTR snapshot_class=find_class("MoonCleaners","MoonConveyorSnapshot");
    if(!snapshot_class||bm.value_type(snapshot_class)||!try_conveyor||method_token(try_conveyor)!=100664883)return 0;
    if(!moving||method_token(moving)!=100663813||!removing||method_token(removing)!=100663826||!placing||method_token(placing)!=100663875)return 0;
    if(!route_point||!holding||!component||!player_upgrades||!views||!shared_material||!set_shared_material||!instantiate||!destroy||!set_color||!cancel||method_token(cancel)!=100663900)return 0;
    conveyors=dmethod("MoonCleaners","MoonWorld","get_BuiltConveyors","");path=dmethod("MoonCleaners","MoonWorld","ConveyorPath","MoonCleaners.MoonConveyorSnapshot");nearest=dmethod("MoonCleaners","MoonWorld","NearestDistance","MoonCleaners.MoonConveyorPath;UnityEngine.Vector3");riders_field=get_field(find_class("MoonCleaners","MoonWorld"),"riders");if(!conveyors||method_token(conveyors)!=100664881||!path||!nearest||!riders_field)return 0;
    if(!fw_bind(domain,game)||!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;before_hook=before;if(!install_hooks())return 0;fw_started=1;bevent("ready");return 1;
}
