#ifndef MOON_CONVEYOR_PRESENTATION_UNITY_H
#define MOON_CONVEYOR_PRESENTATION_UNITY_H
static int ct_alive(PTR object,int *ok){
    if(!object) return 0;
    void *a[]={object};
    PTR result=call(bm.alive,0,a,ok);
    if(!result){*ok=0;return 0;}
    return *ok&&*(unsigned char *)unbox(result);
}
static DRoot ct_material(PTR renderer,int *ok){
    PTR material=call(shared_material,renderer,0,ok);
    DRoot root=dhold(material);
    if(material&&!root) *ok=0;
    return root;
}
/* Ambiguous detach/destroy keeps all roots and blocks further mutation. The
 * next local frame retries cleanup; a renderer never keeps a destroyed clone. */
static int ct_visual_drop(CTEntry *entry){
    int ok=1;
    PTR renderer=dget(entry->renderer),paint=dget(entry->paint);
    if(paint){
        int live=ct_alive(renderer,&ok);
        if(!ok) return 0;
        if(live){
            DRoot current=ct_material(renderer,&ok);
            if(!ok){ddrop(&current);return 0;}
            int attached=dget(current)==paint;
            ddrop(&current);
            if(attached){
                void *a[]={dget(entry->original)};
                call(set_shared_material,renderer,a,&ok);
                if(!ok) return 0;
                current=ct_material(renderer,&ok);
                int detached=ok&&dget(current)!=paint;
                ddrop(&current);
                if(!detached) return 0;
            }
        }
        if(ct_alive(paint,&ok)){
            void *a[]={paint};
            call(destroy,0,a,&ok);
        }
        if(!ok) return 0;
    }
    ddrop(&entry->paint);
    ddrop(&entry->original);
    ddrop(&entry->renderer);
    return 1;
}
#endif
