#include <time.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"

#define ceu_out_assert(v) ceu_sys_assert(v)
void ceu_sys_assert (int v) {
    assert(v);
}

#define ceu_out_log(m,s) ceu_sys_log(m,s)
void ceu_sys_log (int mode, long s) {
    switch (mode) {
        case 0:
            printf("%s", (char*)s);
            break;
        case 1:
            printf("%lX", s);
            break;
        case 2:
            printf("%ld", s);
            break;
    }
}

#define ceu_out_emit(a,b,c,d) ceu_out_event_F(a,b,c,d)
#define ceu_out_wclock_set(us) (DT=us)
#define ceu_out_end(v) (END=1)

mqd_t ceu_queue_write;
s32 DT;
int END = 0;

#include "_ceu_app.c"

typedef struct _Link {
    mqd_t queue;
    char  name[255];
    s16   input;
    struct _Link* nxt;
} Link;

Link* LINKS[CEU_OUT_n+1];

int ceu_out_event_F (tceu_app* app, int id_out, int len, char* data) {
    int cnt = 0;
    Link* cur;
    for (cur=LINKS[id_out]; cur; cur=cur->nxt) {
        char _buf[MSGSIZE];
        *((s16*)_buf) = cur->input;

        memcpy(_buf+sizeof(s16), data, len);

#ifdef CEU_VECTOR
        // REF_1
        // vector in the last argument?
        // copy the plain vector and the payload to the end of "_buf", and 
        // update "len"
        {
            u8 vector_offset = (((u8*)data)[0]);
            if (vector_offset > 0) {
                tceu_vector* v = *((tceu_vector**)(data + vector_offset));
                memcpy(_buf+sizeof(s16)+len, v, sizeof(tceu_vector));
                len += sizeof(tceu_vector);
                memcpy(_buf+sizeof(s16)+len, v->mem, v->nxt);
                len += v->nxt;
            }
        }
#endif

        if (mq_send(cur->queue, _buf, len+sizeof(s16), 0) == 0)
            cnt++;
    }
    //return cnt || LINKS[id_out]==NULL;
    return (cnt > 0) ? 0 : 1;
}

int main (int argc, char *argv[])
{
#ifdef CEU_WCLOCKS
    DT = CEU_WCLOCK_INACTIVE;
#endif

    int i;
    for (i=1; i<=CEU_OUT_n; i++) {
        LINKS[i] = NULL;
    }

    mqd_t queue_read  = mq_open(argv[1], O_RDONLY);
    mqd_t queue_write = mq_open(argv[1], O_WRONLY|O_NONBLOCK);
    ASR(queue_read!=-1 && queue_write!=-1);
    ceu_queue_write = queue_write;

    int ret = 0;

#ifdef CEU_ASYNCS
    int async_cnt = 1;
#else
    int async_cnt = 0;
#endif

    struct timespec ts_old;
    clock_gettime(CLOCK_REALTIME, &ts_old);

    byte CEU_DATA[sizeof(CEU_Main)];
#ifdef CEU_DEBUG
    memset(CEU_DATA, 0, sizeof(CEU_Main));
#endif
    tceu_app app;
        app.data = (tceu_org*) &CEU_DATA;
        app.init = &ceu_app_init;

    app.init(&app);

#ifdef CEU_IN_OS_START
    {
        //ceu_sys_go(&app, CEU_IN_OS_START, NULL);
        char _buf[MSGSIZE];
        *((s16*)_buf) = CEU_IN_OS_START;
        ASR(mq_send(queue_write, _buf, sizeof(s16), 0) == 0);
    }
#endif

    char _buf[MSGSIZE];

    while (!END)
    {
        struct timespec ts_now;
        clock_gettime(CLOCK_REALTIME, &ts_now);
        s32 dt = (ts_now.tv_sec  - ts_old.tv_sec)*1000000 +
                 (ts_now.tv_nsec - ts_old.tv_nsec)/1000;
        ts_old = ts_now;

        if (async_cnt == 0) {
#ifdef CEU_WCLOCKS
            if (DT == CEU_WCLOCK_INACTIVE) {
#endif
                ts_now.tv_sec += 100;
#ifdef CEU_WCLOCKS
            } else {
                DT -= dt;
                u64 t = ts_now.tv_sec*1000000LL +  ts_now.tv_nsec%1000LL;
                t += DT;
                ts_now.tv_sec  = t / 1000000LL;
                ts_now.tv_nsec = t % 1000000LL;
            }
#endif
        }

        if (mq_timedreceive(queue_read,_buf,sizeof(_buf),NULL,&ts_now) == -1)
        {
#ifdef CEU_WCLOCKS
            ceu_sys_go(&app, CEU_IN__WCLOCK, &dt);
#endif
#ifdef CEU_ASYNCS
            ceu_sys_go(&app, CEU_IN__ASYNC, NULL);
#endif
        }
        else
        {
            char* buf = _buf;

            int id_in = *((s16*)buf);
            buf += sizeof(s16);

            switch (id_in)
            {
                case QU_UNLINK: {   // link OUT_ BUF IN_
                    s16 id_out = *((s16*)buf);
                    buf += sizeof(s16);
                    char* name = buf;
                    buf += strlen(name)+1;
                    s16 id_in = *((s16*)buf);
                    buf += sizeof(s16);

                    ASR(id_out <= CEU_OUT_n);

                    Link *cur,*old;
                    for (old=NULL,cur=LINKS[id_out]; cur; old=cur,cur=cur->nxt) {
                        if ( (cur->input == id_in)
                             && (!strcmp(cur->name, name))) {
                            if (old == NULL)
                                LINKS[id_out] = cur->nxt;
                            else
                                old->nxt = cur->nxt;
                            free(cur);
                            break;
                        }
                    }

                    break;
                }
                case QU_LINK: {   // link OUT_ BUF IN_
                    s16 id_out = *((s16*)buf);
                    buf += sizeof(s16);
                    char* name = buf;
                    buf += strlen(name)+1;
                    s16 id_in = *((s16*)buf);
                    buf += sizeof(s16);

                    ASR(id_out <= CEU_OUT_n);
                    mqd_t queue = mq_open(name, O_WRONLY|O_NONBLOCK);
                    ASR(queue != -1);

                    Link* cur = LINKS[id_out];
                    Link* new = (Link*) malloc(sizeof(Link));
                    if (cur == NULL)
                        LINKS[id_out] = cur = new;
                    else {
                        while(cur->nxt)
                            cur = cur->nxt;
                        cur->nxt = new;
                    }
                    strcpy(new->name, name);
                    new->queue = queue;
                    new->input = id_in;
                    new->nxt   = NULL;

                    break;
                }
                case QU_WCLOCK: {
#ifdef CEU_WCLOCKS
                    ceu_sys_go(&app, CEU_IN__WCLOCK, buf);
                    while (DT <= 0) {
                        int dt = 0;
                        ceu_sys_go(&app, CEU_IN__WCLOCK, &dt);
                    }
#endif
                    break;
                }
                default: {
#ifdef CEU_EXTS
#ifdef CEU_VECTOR
                    // REF_1
                    u8 vector_offset = *((u8*)buf);
                    if (vector_offset > 0) {
                        tceu_vector** v = (tceu_vector**)(buf + vector_offset);
                        *v = (tceu_vector*)(buf + vector_offset + sizeof(tceu_vector*));
                        (*v)->mem = (char*)(buf + vector_offset + sizeof(tceu_vector*) + sizeof(tceu_vector));
                    }
#endif
                    ceu_sys_go(&app, id_in, buf);
#endif
                    break;
                }
            }

        }
    }

END:
    mq_close(queue_read);
    mq_close(queue_write);
    return ret;
}

