/* Test-only persistent pthread worker, with deterministic fault injection.
   Not built into firmware. Backend obeys join/quiescence even on failures. */
#include "synth.h"
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <sched.h>
static pthread_t thread;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static int started,quit,queued,done,mode;
static void (*function)(void *);
static void *argument;
static unsigned submitted,executed,joined,refused,attempts,fail_at;
static void *thread_main(void *unused){
 (void)unused;
 pthread_mutex_lock(&mutex);
 for(;;){
  while(!queued && !quit)pthread_cond_wait(&changed,&mutex);
  if(quit)break;
  void (*fn)(void *)=function;void *arg=argument;
  queued=0;pthread_mutex_unlock(&mutex);
  /* Perturb scheduling deterministically, while all state is owned. */
  if((submitted&7u)==0)sched_yield();
  fn(arg);
  pthread_mutex_lock(&mutex);++executed;done=1;pthread_cond_broadcast(&changed);
 }
 pthread_mutex_unlock(&mutex);return 0;
}
static int submit(void (*fn)(void *),void *arg){
 ++attempts;
 if(mode==0 || (mode==3 && attempts%7==0)){++refused;return 0;}
 ++submitted;
 if(mode==1){fn(arg);++executed;return 1;}
 if(mode==5 && submitted>=fail_at)return 1; /* Accepted but no state touched; wait reports failure. */
 pthread_mutex_lock(&mutex);assert(!queued && done);
 function=fn;argument=arg;done=0;queued=1;pthread_cond_broadcast(&changed);
 pthread_mutex_unlock(&mutex);return 1;
}
static int wait_job(void){
 if(mode!=1 && !(mode==5 && submitted>=fail_at)){pthread_mutex_lock(&mutex);while(!done)pthread_cond_wait(&changed,&mutex);pthread_mutex_unlock(&mutex);}
 ++joined;return !((mode==4 || mode==5) && submitted>=fail_at);
}
void worker_backend_install(unsigned new_mode){
 assert(new_mode<=5);mode=(int)new_mode;fail_at=0;
 if(!started){quit=0;queued=0;done=1;assert(!pthread_create(&thread,0,thread_main,0));started=1;}
 synth_worker_config(submit,wait_job);
}
void worker_backend_fail_after(unsigned count){assert(count);fail_at=submitted+count;}
void worker_backend_bad_pair(unsigned first){synth_worker_config(first?submit:0,first?0:wait_job);}
unsigned worker_backend_count(unsigned which){
 unsigned result;
 pthread_mutex_lock(&mutex);
 switch(which){case 0:result=submitted;break;case 1:result=executed;break;case 2:result=joined;break;case 3:result=refused;break;default:result=attempts;}
 pthread_mutex_unlock(&mutex);return result;
}
void worker_backend_close(void){
 if(!started)return;
 pthread_mutex_lock(&mutex);assert(done && !queued);quit=1;pthread_cond_broadcast(&changed);pthread_mutex_unlock(&mutex);
 assert(!pthread_join(thread,0));started=0;
}
