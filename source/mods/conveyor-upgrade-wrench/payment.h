#ifndef MOON_CONVEYOR_PAYMENT_H
#define MOON_CONVEYOR_PAYMENT_H
/* The adapter prepares an exact expected balance before the only debit. Once
 * debit has been entered, exceptions are ambiguous and forbid a later retry. */
typedef struct { int busy, blocked; } CTPayment;
typedef struct {
    int (*prepare)(void *, int);
    int (*debit)(void *);
    int (*confirm)(void *);
    void (*release)(void *);
} CTPaymentApi;
static inline int ct_pay(CTPayment *state, const CTPaymentApi *api, void *context, int cost) {
    if (state->busy || state->blocked || cost < 0) return 0;
    state->busy = 1;
    int paid = 0;
    if (api->prepare(context, cost)) {
        if (!cost) paid = 1;
        else if (api->debit(context) && api->confirm(context)) paid = 1;
        else state->blocked = 1;
    }
    api->release(context);
    state->busy = 0;
    return paid;
}
#endif
