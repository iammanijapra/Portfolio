/* 
 * This problem has you solve the classic "bounded buffer" problem with
 * one producer and multiple consumers:
 *
 *                                       ------------
 *                                    /->| consumer |
 *                                    |  ------------
 *                                    |
 *  ------------                      |  ------------
 *  | producer |-->   bounded buffer --->| consumer |
 *  ------------                      |  ------------
 *                                    |
 *                                    |  ------------
 *                                    \->| consumer |
 *                                       ------------
 *
 *  The program below includes everything but the implementation of the
 *  bounded buffer itself.  main() does this:
 *
 *  1. starts N consumers as per the first argument (default 1)
 *  2. starts the producer
 *
 *  The producer reads positive integers from standard input and passes those
 *  into the buffer.  The consumers read those integers and "perform a
 *  command" based on them (all they really do is sleep for some period...)
 *
 *  on EOF of stdin, the producer passes N copies of -1 into the buffer.
 *  The consumers interpret -1 as a signal to exit.
 */

#define _REENTRANT
#include <stdio.h>
#include <stdlib.h>             /* atoi() */
#include <unistd.h>             /* usleep() */
#include <assert.h>             /* assert() */
#include <signal.h>             /* signal() */
#include <alloca.h>             /* alloca() */
#include <pthread.h>

/**************************************************************************\
 *                                                                        *
 * Bounded buffer.  This is the only part you need to modify.  Your       *
 * buffer should have space for up to 10 integers in it at a time.        *
 *                                                                        *
 * Add any data structures you need (globals are fine) and fill in        *
 * implementations for these three procedures:                            *
 *                                                                        *
 * void buffer_init(void)                                                 *
 *                                                                        *
 *      buffer_init() is called by main() at the beginning of time to     *
 *      perform any required initialization.  I.e. initialize the buffer, *
 *      any mutex/condition variables, etc.                               *
 *                                                                        *
 * void buffer_insert(int number)                                         *
 *                                                                        *
 *      buffer_insert() inserts a number into the next available slot in  *
 *      the buffer.  If no slots are available, the thread should wait    *
 *      (not spin-wait!) for an empty slot to become available.           *
 *                                                                        *
 * int buffer_extract(void)                                               *
 *                                                                        *
 *      buffer_extract() removes and returns the number in the next       *
 *      available slot.  If no number is available, the thread should     *
 *      wait (not spin-wait!) for a number to become available.  Note     *
 *      that multiple consumers may call buffer_extract() simulaneously.  *
 *                                                                        *
\**************************************************************************/

typedef struct {
	int queue[10];
	int head;
	int tail;
	int size;
} boundedBuffer;

static boundedBuffer buffer;
static pthread_mutex_t insert_mutex;
static pthread_mutex_t extract_mutex;
static pthread_cond_t noSlotsAvailable;
static pthread_cond_t noNumAvailable;

void buffer_init(void)
{
  //printf("buffer_init called:\n");
  
  int i;
  for (i = 0; i < 10; i++)
  {
  		buffer.queue[i] = 0;
  }
  
  buffer.head = 0;
  buffer.tail = 0;
  buffer.size = 0;
  
  pthread_mutex_init(&insert_mutex, NULL);
  pthread_mutex_init(&extract_mutex, NULL);
  pthread_cond_init(&noNumAvailable, NULL);
  pthread_cond_init(&noSlotsAvailable, NULL);
  pthread_cond_signal(&noSlotsAvailable);
  pthread_cond_signal(&noNumAvailable);
}

void buffer_insert(int number)
{
  //printf("buffer_insert called with %d:\n", number);
  pthread_mutex_lock(&insert_mutex);
  while(buffer.size == 10)
  {
  		pthread_cond_wait(&noSlotsAvailable, &insert_mutex);
  }
  if (buffer.tail == buffer.head)
  {
  		buffer.queue[0] = number;
  		buffer.tail++;
  }
  else if (buffer.tail < 9)
  {
  		buffer.queue[buffer.tail] = number;
  		buffer.tail++;
  }
  else
  {
  		buffer.queue[buffer.tail] = number;
  		buffer.tail = 0;
  }
  buffer.size++;
  pthread_mutex_unlock(&insert_mutex);
}

int buffer_extract(void)
{
  //printf("buffer_extract called:\n");
  pthread_mutex_lock(&extract_mutex);
  int returnVal;
  while (buffer.size == 0)
  {
  		pthread_cond_wait(&noNumAvailable, &extract_mutex);
  }
  if (buffer.head < 9)
  {
  		returnVal = buffer.queue[buffer.head];
  		buffer.head++;
  }
  else
  {
  		returnVal = buffer.queue[buffer.head];
  		buffer.head = 0;
  }
  buffer.size--;
  pthread_mutex_unlock(&extract_mutex);
  return returnVal;
}

/**************************************************************************\
 *                                                                        *
 * consumer thread.  main starts any number of consumer threads.  Each    *
 * consumer thread reads and "interprets" numbers from the bounded        *
 * buffer.                                                                *
 *                                                                        *
 * The interpretation is as follows:                                      *
 *                                                                        *
 * o  positive integer N: sleep for N * 100ms                             *
 * o  negative integer:  exit                                             *
 *                                                                        *
 * Note that a thread procedure (called by pthread_create) is required to *
 * take a void * as an argument and return a void * as a result.  We play *
 * a dirty trick: we pass the thread number (main()'s idea of the thread  *
 * number) as the "void *" argument.  Hence the casts.  The return value  *
 * is ignored so we return NULL.                                          *
 *                                                                        *
\**************************************************************************/

void *consumer_thread(void *raw_consumerno)
{
  int consumerno = (int)raw_consumerno; /* dirty trick to pass in an integer */

  printf("  consumer %d: starting\n", consumerno);
  while (1)
    {
      int number = buffer_extract();

      if (number < 0)
        break;

      usleep(100000 * number);  /* "interpret" the command */
      printf("  consumer %d: %d\n", consumerno, number);
      fflush(stdout);
    }

  printf("  consumer %d: exiting\n", consumerno);
  return(NULL);
}

/**************************************************************************\
 *                                                                        *
 * producer.  main calls the producer as an ordinary procedure rather     *
 * than creating a new thread.  In other words the original "main" thread *
 * becomes the "producer" thread.                                         *
 *                                                                        *
 * The producer reads numbers from stdin, and inserts them into the       *
 * bounded buffer.  On EOF from stdin, it finished up by inserting a -1   *
 * for every consumer thread so that all the consumer thread shut down    *
 * cleanly.                                                               *
 *                                                                        *
\**************************************************************************/

#define MAXLINELEN 128

void producer(int nconsumers)
{
  char buffer[MAXLINELEN];
  int number;

  printf("  producer: starting\n");

  while (fgets(buffer, MAXLINELEN, stdin) != NULL)
    {
      number = atoi(buffer);
      printf("producer: %d\n", number);
      buffer_insert(number);
    }

  printf("producer: read EOF, sending %d '-1' numbers\n", nconsumers);
  for (number = 0; number < nconsumers; number++)
    buffer_insert(-1);

  printf("producer: exiting\n");
}

/*************************************************************************\
 *                                                                       *
 * main program.  Main calls buffer_init and does other initialization,  *
 * fires of N copies of the consumer (N given by command-line argument), *
 * then calls the producer.                                              *
 *                                                                       *
\*************************************************************************/

int main(int argc, char *argv[])
{
  pthread_t *consumers;
  int nconsumers = 1;
  int kount;

  if (argc > 1)
    nconsumers = atoi(argv[1]);

  printf("main: nconsumers = %d\n", nconsumers);
  assert(nconsumers >= 0);

  /*
   * 1. initialization
   */
  buffer_init();
  signal(SIGALRM, SIG_IGN);     /* evil magic for usleep() under solaris */

  /*
   * 2. start up N consumer threads
   */
  consumers = (pthread_t *)alloca(nconsumers * sizeof(pthread_t));
  for (kount = 0; kount < nconsumers; kount++)
    {
      int test = pthread_create(&consumers[kount], /* pthread number */
                                NULL,            /* "attributes" (unused) */
                                consumer_thread, /* procedure */
                                (void *)kount);  /* hack: consumer number */

      assert(test == 0);
    }

  /*
   * 3. run the producer in this thread.
   */
  producer(nconsumers);

  /*
   * n. clean up: the producer told all the consumers to shut down (by
   *    sending -1 to each).  Now wait for them all to finish.
   */
  for (kount = 0; kount < nconsumers; kount++)
    {
      int test = pthread_join(consumers[kount], NULL);

      assert(test == 0);
    }
  return(0);

  //Spring 2016
}
