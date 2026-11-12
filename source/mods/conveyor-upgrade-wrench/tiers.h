#ifndef MOON_CONVEYOR_TIERS_H
#define MOON_CONVEYOR_TIERS_H
/* Snapshot objects are reference identities, never boxed values. The adapter
 * checks that invariant at binding and roots every lookup result. */
#define CT_LIMIT 256
typedef struct { int id, tier; DRoot snapshot,renderer,original,paint; } CTEntry;
static int ct_visual_drop(CTEntry *entry);
static CTEntry ct_entries[CT_LIMIT];
static int ct_count;
static int ct_index(int id) {
    for (int i=0;i<ct_count;i++) if (ct_entries[i].id==id) return i;
    return -1;
}
static int upgraded(int id) {
    int i=ct_index(id);return i<0?0:ct_entries[i].tier;
}
static int ct_forget(int i) {
    /* A failed material detach retains ownership for a later cleanup retry. */
    if (!ct_visual_drop(ct_entries+i)) return 0;
    ddrop(&ct_entries[i].snapshot);
    ct_count--;
    if (i!=ct_count) ct_entries[i]=ct_entries[ct_count];
    ct_entries[ct_count]=(CTEntry){0};
    return 1;
}
static int ct_reset(void) {
    for (int i=ct_count;i>0;i--) ct_forget(i-1);
    return ct_count==0;
}
static int ct_prune(void) {
    for (int i=0;i<ct_count;) {
        int ok=1;DRoot current=ct_snapshot(ct_entries[i].id,&ok);
        int same=current&&dget(current)==dget(ct_entries[i].snapshot);
        ddrop(&current);
        if (!ok) return 0;
        if (same) i++;else if (!ct_forget(i)) return 0;
    }
    return 1;
}
static int ct_promote(int id, DRoot snapshot, int maximum, int price2, int price3) {
    if (!snapshot || maximum<2 || maximum>3) return 0;
    int i=ct_index(id);
    if (i>=0 && dget(ct_entries[i].snapshot)!=dget(snapshot)) { if (!ct_forget(i)) return 0;i=-1; }
    int tier=i<0?1:ct_entries[i].tier;
    if (tier>=maximum || (i<0 && ct_count>=CT_LIMIT)) return 0;
    /* Reserve ownership before payment. No allocation can fail after debit. */
    DRoot reserved=dhold(dget(snapshot));if (!reserved) return 0;
    int ok=1;DRoot current=ct_snapshot(id,&ok);
    int live=ok&&current&&dget(current)==dget(reserved);ddrop(&current);
    if (!live || !bcharge(tier==2?price3:price2)) { ddrop(&reserved);return 0; }
    if (i<0) { i=ct_count++;ct_entries[i]=(CTEntry){.id=id,.snapshot=reserved}; }
    else ddrop(&reserved);
    ct_entries[i].tier=tier+1;
    return tier+1;
}
#endif
