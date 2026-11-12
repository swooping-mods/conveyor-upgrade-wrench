#ifndef MOON_CONVEYOR_PAYMENT_UNITY_H
#define MOON_CONVEYOR_PAYMENT_UNITY_H
/* Decimal comparisons retain cents and reject an uncertain debit. */
typedef struct { DRoot world,walker,player,expected;unsigned char value[16]; } BPayment;
static int bpayment_context(BPayment *p){
    if(dget(p->world)!=dget(bm.world)||dget(p->walker)!=dget(bm.walker)||!bcontext(dget(p->world),dget(p->walker)))return 0;
    int ok=1;DRoot current=dhold(call(bm.player,0,0,&ok));int same=0;
    if(ok&&current){void *a[]={dget(p->player),dget(current)};PTR result=call(bm.string_equal,0,a,&ok);same=ok&&result&&*(unsigned char *)unbox(result);}
    ddrop(&current);
    return same&&dget(p->world)==dget(bm.world)&&dget(p->walker)==dget(bm.walker)&&bcontext(dget(p->world),dget(p->walker));
}
static int bpayment_prepare(void *context,int cost){
    BPayment *p=context;int ok=1;
    if(!bcontext(dget(bm.world),dget(bm.walker)))return 0;
    p->world=dhold(dget(bm.world));p->walker=dhold(dget(bm.walker));p->player=dhold(call(bm.player,0,0,&ok));
    if(!ok||!p->world||!p->walker||!p->player)return 0;
    void *who[]={dget(p->player)};DRoot money=dhold(call(bm.wallet,dget(p->world),who,&ok));
    void *ctor[]={&cost};if(ok&&money)call(bm.decimal_int,p->value,ctor,&ok);else ok=0;
    int enough=0;
    if(ok){void *compare[]={unbox(dget(money)),p->value};PTR result=call(bm.decimal_compare,0,compare,&ok);enough=ok&&result&&*(int *)unbox(result)>=0;}
    if(enough){void *subtract[]={unbox(dget(money)),p->value};p->expected=dhold(call(bm.decimal_subtract,0,subtract,&ok));}
    ddrop(&money);
    /* ChargeBuild applies a signed delta; comparison/subtraction used +cost. */
    int debit=-cost;void *debit_ctor[]={&debit};
    if(enough&&ok&&p->expected)call(bm.decimal_int,p->value,debit_ctor,&ok);
    return enough&&ok&&p->expected&&bpayment_context(p);
}
static int bpayment_debit(void *context){
    BPayment *p=context;int ok=1;
    if(!bpayment_context(p))return 0;
    void *a[]={dget(p->player),p->value};
    call(bm.charge,dget(p->world),a,&ok);return ok;
}
static int bpayment_confirm(void *context){
    BPayment *p=context;int ok=1;
    if(!bpayment_context(p))return 0;
    void *who[]={dget(p->player)};
    DRoot money=dhold(call(bm.wallet,dget(p->world),who,&ok));int exact=0;
    if(ok&&money){void *a[]={unbox(dget(money)),unbox(dget(p->expected))};PTR result=call(bm.decimal_compare,0,a,&ok);exact=ok&&result&&*(int *)unbox(result)==0;}
    ddrop(&money);return exact&&bpayment_context(p);
}
static void bpayment_release(void *context){
    BPayment *p=context;ddrop(&p->expected);ddrop(&p->player);ddrop(&p->walker);ddrop(&p->world);
}
static int bcharge(int cost){
    static const CTPaymentApi api={bpayment_prepare,bpayment_debit,bpayment_confirm,bpayment_release};
    BPayment payment={0};int blocked=bm.payments.blocked;
    int result=ct_pay(&bm.payments,&api,&payment,cost);
    if(!blocked&&bm.payments.blocked)bevent("payment-unconfirmed-session-blocked");
    return result;
}
#endif
