#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__)
#else
#define VERBOSE_PRINT(S, ...) ((void) 0) // do nothing
#endif

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create(agent->mutex);
  agent->match   = uthread_cond_create(agent->mutex);
  agent->tobacco = uthread_cond_create(agent->mutex);
  agent->smoke   = uthread_cond_create(agent->mutex);
  return agent;
}

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

// # of threads waiting for a signal. Used to ensure that the agent
// only signals once all other threads are ready.
int num_active_threads = 0;

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

//
// TODO
// You will probably need to add some procedures and struct etc.
//
int total=0;
int smoketotal =0;
int signalCount = 0;

uthread_mutex_t m1;
uthread_cond_t paperCond,tobaccoCond,matchCond,dispatchCond;

//=================C O M M E N T S======================================
// listener for tobacco signal from agent thread. once caught, increments total by 
//enum of tobacco and increments signalCount by 1, then signals the dspatcher thread
//which will handle the signals
void* tobaccoSignaller(void* av) {
  struct Agent* a = av;
  uthread_mutex_lock(a->mutex);
  if (++num_active_threads ==7){
    uthread_cond_signal(a->smoke);
  }
  while(smoketotal<NUM_ITERATIONS){
    uthread_cond_wait(a->tobacco);
    if (smoketotal<NUM_ITERATIONS){
      total+=TOBACCO;
      signalCount++;
      uthread_cond_signal(dispatchCond);
      VERBOSE_PRINT ("sent tobacco signal to dispatcher\n");
    }else {
      break;
    }  
  }
  VERBOSE_PRINT ("tobacco signaller thread exiting\n");
  uthread_mutex_unlock(a->mutex);
  return NULL;
}

//=================C O M M E N T S======================================
// listener for match signal from agent thread. once caught, increments total by 
//enum of match and increments signalCount by 1, then signals the dspatcher thread
//which will handle the signals
void* matchSignaller(void* av) {
  struct Agent* a = av;
  uthread_mutex_lock(a->mutex);
  if (++num_active_threads ==7){
    uthread_cond_signal(a->smoke);
  }
  while(smoketotal<NUM_ITERATIONS){
    uthread_cond_wait(a->match);
    if (smoketotal<NUM_ITERATIONS){
      total+=MATCH;
      signalCount++;
      uthread_cond_signal(dispatchCond);
      VERBOSE_PRINT ("sent match signal to dispatcher\n");
    }else {
      break;
    }  
  }
  VERBOSE_PRINT ("match signaller thread exiting\n");
  uthread_mutex_unlock(a->mutex);
  return NULL;
}

//=================C O M M E N T S======================================
// listener for paper signal from agent thread. once caught, increments total by 
//enum of paper and increments signalCount by 1, then signals the dspatcher thread
//which will handle the signals
void* paperSignaller(void* av) {
  struct Agent* a = av;
  uthread_mutex_lock(a->mutex);
  if (++num_active_threads ==7){
    uthread_cond_signal(a->smoke);
  }
  while(smoketotal<NUM_ITERATIONS){
    uthread_cond_wait(a->paper);
    if (smoketotal<NUM_ITERATIONS){
      total+=PAPER;
      signalCount++;
      uthread_cond_signal(dispatchCond);
      VERBOSE_PRINT ("sent paper signal to dispatcher\n");
    }else {
      break;
    }
  }
  VERBOSE_PRINT ("paper signaller thread exiting\n");
  uthread_mutex_unlock(a->mutex);
  return NULL;
}

//=================C O M M E N T S======================================
//waits for dedicated paper conditional variable signal from dispatcher thread
//when signal is received, increment appropriate smoke_count, and signal smoke to
//agent thread to go to next iteration. also iincrements total number of smokes
void* paperSmoker(void* av) {
  struct Agent* a = av;
  uthread_mutex_lock(a->mutex);
  if (++num_active_threads ==7){
    uthread_cond_signal(a->smoke);
  }
  
  while(smoketotal<NUM_ITERATIONS){
    uthread_cond_wait(paperCond);
    if (smoketotal<NUM_ITERATIONS){
      smoke_count[PAPER]++;
      uthread_cond_signal(a->smoke);
      smoketotal++;
    }
    if (smoketotal==NUM_ITERATIONS){
      uthread_cond_signal(a->tobacco);
      uthread_cond_signal(a->match);
      uthread_cond_signal(a->paper);
      uthread_cond_signal(paperCond);
      uthread_cond_signal(matchCond);
      uthread_cond_signal(tobaccoCond);
      break;
    }
    // printf("smoke number: %d is tobacco\n\n", smoketotal);
  }
  VERBOSE_PRINT ("paper smoker thread exiting\n");
  uthread_mutex_unlock(a->mutex);
  return NULL;
}

//=================C O M M E N T S======================================
//waits for dedicated tobacco conditional variable signal from dispatcher thread
//when signal is received, increment appropriate smoke_count, and signal smoke to
//agent thread to go to next iteration. also iincrements total number of smokes
void* tobaccoSmoker(void* av) {
  struct Agent* a = av;
  uthread_mutex_lock(a->mutex);
  if (++num_active_threads ==7){
    uthread_cond_signal(a->smoke);
  }
  
  while(smoketotal<NUM_ITERATIONS){
    uthread_cond_wait(tobaccoCond);
    if (smoketotal<NUM_ITERATIONS){
      
      smoke_count[TOBACCO]++;
      uthread_cond_signal(a->smoke);
      smoketotal++;
    }
    if (smoketotal==NUM_ITERATIONS){
      uthread_cond_signal(a->tobacco);
      uthread_cond_signal(a->match);
      uthread_cond_signal(a->paper);
      uthread_cond_signal(paperCond);
      uthread_cond_signal(matchCond);
      uthread_cond_signal(tobaccoCond);
      break;
    }
    // printf("smoke number: %d is tobacco\n\n", smoketotal);
  }
  VERBOSE_PRINT ("tobacco smoker thread exiting\n");
  uthread_mutex_unlock(a->mutex);
  return NULL;
}

//=================C O M M E N T S======================================
//waits for dedicated match conditional variable signal from dispatcher thread
//when signal is received, increment appropriate smoke_count, and signal smoke to
//agent thread to go to next iteration. also iincrements total number of smokes
void* matchSmoker(void* av) {
  struct Agent* a = av;
  uthread_mutex_lock(a->mutex);
  if (++num_active_threads ==7){
    uthread_cond_signal(a->smoke);
  }
  
  while(smoketotal<NUM_ITERATIONS){
    uthread_cond_wait(matchCond);
    if (smoketotal<NUM_ITERATIONS){
      smoke_count[MATCH]++;
      uthread_cond_signal(a->smoke);
      smoketotal++;
    }
    if (smoketotal==NUM_ITERATIONS){
      uthread_cond_signal(a->tobacco);
      uthread_cond_signal(a->match);
      uthread_cond_signal(a->paper);
      uthread_cond_signal(paperCond);
      uthread_cond_signal(matchCond);
      uthread_cond_signal(tobaccoCond);
      break;
    }
    // printf("smoke number: %d is tobacco\n\n", smoketotal);
  }
  VERBOSE_PRINT ("match smoker thread exiting\n");
  uthread_mutex_unlock(a->mutex);
  return NULL;
}

//=================C O M M E N T S======================================
//this handles which smoker to wake up, if necessary. catches a common signal sent by
//the 3 listener/signaller threads. if it's the first signal, this does nothing and 
//goes back to waiting for next one. upon second signal sent, resets signalCount to 0,
// and based on the total of sent signals, signals the appropriate smoker thread and
//resets total to 0;
void* dispatcher(void* av) {
  struct Agent* a = av;
  uthread_mutex_lock(a->mutex);
  if (++num_active_threads ==7){
    uthread_cond_signal(a->smoke);
  }
  
  while(smoketotal<NUM_ITERATIONS){
    while(signalCount<2){
      uthread_cond_wait(dispatchCond);
      // printf("%d signals received\n", signalCount);
    }
    signalCount = 0;
    // 
    switch (total)
    {
    case 3: //match and paper available
      uthread_cond_signal(tobaccoCond);
      break;
    case 5: //match and tobacco available
      uthread_cond_signal(paperCond);
      break;
    case 6: //paper and tobacco available
      uthread_cond_signal(matchCond);
      break;
    default:
      break;
    }
    total =0;
    if (smoketotal==(NUM_ITERATIONS-1)){
      break;
    }
  }
  VERBOSE_PRINT ("dispatcher thread exiting\n");
  uthread_mutex_unlock(a->mutex);
  return NULL;
}


/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can modify it if you like, but be sure that all it does
 * is choose 2 random resources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};

  srandom(time(NULL));
  
  uthread_mutex_lock (a->mutex);
  // Wait until all other threads are waiting for a signal
  while (num_active_threads < 7){
    uthread_cond_wait (a->smoke);
  }
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    int r = random() % 6;
    switch(r) {
    case 0:
      signal_count[TOBACCO]++;
      // VERBOSE_PRINT ("match available\n");
      uthread_cond_signal (a->match);
      // VERBOSE_PRINT ("paper available\n");
      uthread_cond_signal (a->paper);
      VERBOSE_PRINT ("I EXPECT A TOBACOO SMOKER: match and paper\n");
      break;
    case 1:
      signal_count[PAPER]++;
      // VERBOSE_PRINT ("match available\n");
      uthread_cond_signal (a->match);
      // VERBOSE_PRINT ("tobacco available\n");
      uthread_cond_signal (a->tobacco);
      VERBOSE_PRINT ("I EXPECT A PAPER SMOKER: match and tobacco\n");
      break;
    case 2:
      signal_count[MATCH]++;
      // VERBOSE_PRINT ("paper available\n");
      uthread_cond_signal (a->paper);
      // VERBOSE_PRINT ("tobacco available\n");
      uthread_cond_signal (a->tobacco);
      VERBOSE_PRINT ("I EXPECT A MATCH SMOKER: paper and tobacco\n");
      break;
    case 3:
      signal_count[TOBACCO]++;
      // VERBOSE_PRINT ("paper available\n");
      uthread_cond_signal (a->paper);
      // VERBOSE_PRINT ("match available\n");
      uthread_cond_signal (a->match);
      VERBOSE_PRINT ("I EXPECT A TOBACOO SMOKER: paper and match\n");
      break;
    case 4:
      signal_count[PAPER]++;
      // VERBOSE_PRINT ("tobacco available\n");
      uthread_cond_signal (a->tobacco);
      // VERBOSE_PRINT ("match available\n");
      uthread_cond_signal (a->match);
      VERBOSE_PRINT ("I EXPECT A PAPER SMOKER: tobacco and match\n");
      break;
    case 5:
      signal_count[MATCH]++;
      // VERBOSE_PRINT ("tobacco available\n");
      uthread_cond_signal (a->tobacco);
      // VERBOSE_PRINT ("paper available\n");
      uthread_cond_signal (a->paper);
      VERBOSE_PRINT ("I EXPECT A MATCH SMOKER: tobacco and paper\n");
      break;
    }
    // VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
    // printf("%d",i);
    uthread_cond_wait (a->smoke);
  }
  VERBOSE_PRINT ("agent thread exiting\n");
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

int main (int argc, char** argv) {
  struct Agent* a = createAgent();
  uthread_t agent_thread,paperSmoker_thread,matchSmoker_thread,tobaccoSmoker_thread,
  paperSignaller_thread,matchSignaller_thread,tobaccoSignaller_thread,dispatcher_thread;

  uthread_init(1);

  // TODO
  //=================C O M M E N T S======================================
  // Threads have a built in feature to call let agent know they're ready, and when all
  // threads are live, agent will run, otherwise agent waits. Because of this, threads 
  // can be created in any order and will work fine 
  // ALSO:
  // threads all have a built in functionallity to call and flush out all other threads 
  // when final smoke is done. unnecessary but i did it anyways so all threads can be
  // joined so it's more tidy
 
  agent_thread = uthread_create(agent, a);
  paperSignaller_thread = uthread_create(paperSignaller, a);
  matchSignaller_thread = uthread_create(matchSignaller, a);
  tobaccoSignaller_thread = uthread_create(tobaccoSignaller, a);
  dispatcher_thread = uthread_create(dispatcher, a);

  
  dispatchCond = uthread_cond_create(a->mutex);
  paperCond = uthread_cond_create(a->mutex);
  matchCond = uthread_cond_create(a->mutex);
  tobaccoCond = uthread_cond_create(a->mutex);

  paperSmoker_thread = uthread_create(paperSmoker, a);
  matchSmoker_thread = uthread_create(matchSmoker, a);
  tobaccoSmoker_thread = uthread_create(tobaccoSmoker, a);
  
  
  uthread_join(agent_thread, NULL);
  uthread_join(matchSignaller_thread,NULL);
  uthread_join(paperSignaller_thread,NULL);
  uthread_join(tobaccoSignaller_thread,NULL);
  uthread_join(matchSmoker_thread,NULL);
  uthread_join(paperSmoker_thread,NULL);
  uthread_join(tobaccoSmoker_thread,NULL);
  uthread_join(dispatcher_thread,NULL);

  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);

  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);

  uthread_cond_destroy(dispatchCond);
  uthread_cond_destroy(paperCond);
  uthread_cond_destroy(matchCond);
  uthread_cond_destroy(tobaccoCond);
  uthread_cond_destroy(a->paper);
  uthread_cond_destroy(a->match);
  uthread_cond_destroy(a->tobacco);
  uthread_cond_destroy(a->smoke);
  uthread_mutex_destroy(a->mutex);
  free(a);
  
  return 0;
}
