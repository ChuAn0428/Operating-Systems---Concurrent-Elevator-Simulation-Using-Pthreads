#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////
// CSCI 503 Operating Systems
// Lab 4 - Concurrent Elevator Simulation Using Pthreads
// Part 1: Basic version  
// Author: Chu-An Tsai
///////////////////////////////////////////////////////////////////////////

// Structure declaration
struct person
{
	int id; 
	int from_floor,to_floor;
	double arrival_time;
	struct person * next_person;
	struct person * previous_person;
};

struct elevator
{
	int id;
	int current_floor;
	struct person * people;
};

struct Semaphore_Dll 
{
    pthread_mutex_t mutex_lock;
    pthread_cond_t  cv;
    struct person * person_list;
    int list_size;
    int added_num_people;
    int removed_num_people;
};

typedef struct lv_t 
{
    int             tid;
    int             related_num_persons;
    struct elevator *elevator_inthethread;
    void            *gv;
}* LV;

typedef struct gv_t 
{
    int num_elevators;
    int num_floors;
    double beginning_time;
    int gen_person_time;
    int elevator_speed;
    int simulation_time;
    int random_seed;
 	int num_people_started;
	int num_people_finished;
  	int current_max_personid;
  	struct Semaphore_Dll sem_buffer;
}* GV;

// Get the current time
double get_cur_time() 
{
  	struct timeval   tv;
  	struct timezone  tz;
  	double cur_time;
  	gettimeofday(&tv, &tz);
  	cur_time = tv.tv_sec + tv.tv_usec / 1000000.0;

  	return cur_time;
}

// Initialize the semaphore structure
void init_sem_dll(struct Semaphore_Dll * sem_buffer) 
{
	sem_buffer->person_list = NULL;
	sem_buffer->list_size = 0;
	sem_buffer->added_num_people = 0;
	sem_buffer->removed_num_people= 0;
	if(pthread_mutex_init(&sem_buffer->mutex_lock, NULL)!=0)
	{
		perror("mutex init error!\n");
		exit(1);
	}
	if(pthread_cond_init(&sem_buffer->cv, NULL))
	{
		perror("cv init error!\n");
		exit(1);
	}
}

// Append the person to the end of existing people list
void add_person_to_end(struct Semaphore_Dll * sem_buffer, struct person* new_person)
{
	struct person ** person_list = &(sem_buffer->person_list);
	if(*person_list == NULL)
	{
		new_person->previous_person = NULL;
		*person_list = new_person;
		return;
	}
	struct person * move_pointer = *person_list;
	while(move_pointer->next_person!=NULL)
	{
		move_pointer = move_pointer->next_person;
	}
	move_pointer->next_person = new_person;
	new_person->previous_person = move_pointer;
	sem_buffer->list_size ++;
	sem_buffer->added_num_people++;
}

// Remove the head person in the person list
struct person * remove_fifo(struct Semaphore_Dll * sem_buffer)
{
	struct person ** person_list = &(sem_buffer->person_list);
	if(*person_list==NULL)
	{
		return NULL;
	}
	struct person * removed_person = *person_list;
	*person_list = (*person_list)->next_person;
	if(*person_list != NULL)
	{
		(*person_list)->previous_person = NULL;
		removed_person->next_person = NULL;
	}
	sem_buffer->list_size --;
	sem_buffer->removed_num_people ++;
	return removed_person;
}

// Init the local variable and elevator related variables.
void init_lv(LV lv, int tid, GV gv) 
{
  	lv->tid   = tid;
  	lv->gv    = gv;
  	lv->related_num_persons = 0;
  	lv->elevator_inthethread = (struct elevator *)malloc(sizeof(struct elevator));
  	lv->elevator_inthethread->id = tid;
  	lv->elevator_inthethread->current_floor = 1;
  	lv->elevator_inthethread->people = NULL;
}

// Init the global variables
void init_gv(GV gv, int num_elevators, int num_floors, double beginning_time, int gen_person_time,int elevator_speed, int simulation_time, int random_seed)
{
	gv->num_elevators = num_elevators;
	gv->num_floors = num_floors;
	gv->beginning_time = beginning_time;
	gv->gen_person_time = gen_person_time;
	gv->elevator_speed = elevator_speed;
	gv->simulation_time = simulation_time;
	gv->random_seed = random_seed;
	gv->current_max_personid = 0;
}

// Get the random integer to determine the floor number
// (1~n floor)
int rand_num(int n) 
{ 
    srand(time(NULL));
	return	rand()%n + 1;
}

// The thread to generate person
void* thread_person(void *v) 
{
	LV   lv;
	GV   gv;

	lv = (LV) v;
	gv = (GV) lv->gv;

	while(1) 
	{
		struct Semaphore_Dll * g_s_buffer = &(gv->sem_buffer);

		srand(gv->random_seed);
		//sleep(gv->gen_person_time);
		double current_time = get_cur_time();
		if(current_time - gv->beginning_time >= gv->simulation_time)
		{
			int i = 0;
			// If time is up, call all elevators to terminate
			for(i=0;i<abs(gv->num_elevators-gv->num_floors);i++)
			{
				pthread_cond_broadcast(&(g_s_buffer->cv));
			}
			break;
		}
		// Create a new person
		pthread_mutex_lock(&(g_s_buffer->mutex_lock));
		int new_person_id = gv->current_max_personid;
		gv->current_max_personid ++;
		struct person* new_person = (struct person*)malloc(sizeof(struct person));
		new_person->id = new_person_id;
		new_person->from_floor = rand_num(gv->num_floors);
		new_person->next_person = NULL;
		// Make sure that from_floor would not equal to to_floor and stays in the range of num_floors
		int temp = rand()%(gv->num_floors)+1;
		if (temp != new_person->from_floor)
		{
			new_person->to_floor = temp;	
		}else if(temp+1 <= gv->num_floors)
		{
 			new_person->to_floor = temp+1;
		}else
		{
			new_person->to_floor = temp-1;
		}
		gv->num_people_started ++;
		// Append it to then end
		add_person_to_end(g_s_buffer, new_person);
		printf("[%d] Person %d arrives on floor %d, waiting to go to floor %d\n", (int)(current_time-gv->beginning_time), new_person->id, new_person->from_floor, new_person->to_floor);
		// Increase the related number of people 
		lv->related_num_persons ++;		
		g_s_buffer->added_num_people ++;
		if(g_s_buffer->list_size > 0)
		{
			pthread_cond_broadcast(&(g_s_buffer->cv));
		}
		pthread_mutex_unlock(&(g_s_buffer->mutex_lock));
		sleep(gv->gen_person_time);
	}	
	pthread_exit((void *)lv->related_num_persons);
}

// The thread for elevator
void* thread_elevator(void *v)
{
	LV	lv;
	GV	gv;

	lv = (LV) v;
	gv = (GV) lv->gv;

	struct elevator * thread_elevator = lv->elevator_inthethread;
	while(1)
	{
		struct Semaphore_Dll * g_s_buffer = &(gv->sem_buffer);
		srand(gv->random_seed);
		double current_time = get_cur_time();
		// If time is up
		if(current_time - gv->beginning_time >= gv->simulation_time)
		{
			break;
		}
		pthread_mutex_lock(&(g_s_buffer->mutex_lock));
		// Wait for the people
		while(g_s_buffer->list_size == 0)
		{
			pthread_cond_wait(&(g_s_buffer->cv), &(g_s_buffer->mutex_lock));
			current_time = get_cur_time();
			if(current_time - gv->beginning_time >= gv->simulation_time)
			{
				pthread_mutex_unlock(&(g_s_buffer->mutex_lock));
				pthread_exit((void *)lv->related_num_persons);
			}
		}
		// Remove one person from the list
		struct person * person_in_elevator = remove_fifo(g_s_buffer);
		if(g_s_buffer->list_size > 0)
		{
			pthread_cond_broadcast(&(g_s_buffer->cv));
		}
		pthread_mutex_unlock(&(g_s_buffer->mutex_lock));

		// Move the elevator
		int distance_to_person = abs(person_in_elevator->from_floor - thread_elevator->current_floor);
		current_time = get_cur_time();
		if((int)(current_time - gv->beginning_time) < gv->simulation_time)
		{
			printf("[%d] Elevator %d starts moving from %d to %d...\n",(int)(current_time - gv->beginning_time), lv->tid, thread_elevator->current_floor, person_in_elevator->from_floor);	
		}
		else
		{
			break;
		}
		sleep(distance_to_person * gv->elevator_speed);
		thread_elevator->current_floor = person_in_elevator->from_floor;
		// Arrived, then pick up person
		current_time = get_cur_time();
		if((int)(current_time - gv->beginning_time) < gv->simulation_time)
		{
			printf("[%d] Elevator %d arrives at floor %d\n", (int)(current_time - gv->beginning_time), lv->tid, person_in_elevator->from_floor);
			printf("[%d] Elevator %d picks up Person %d\n", (int)(current_time - gv->beginning_time), lv->tid, person_in_elevator->id);
			printf("[%d] Elevator %d starts moving from %d to %d...\n",(int)(current_time - gv->beginning_time), lv->tid, person_in_elevator->from_floor, person_in_elevator->to_floor);
		}
		else
		{
			break;
		}
		// Arrived, drop person
		int distance_to_destination = abs(person_in_elevator->from_floor -  person_in_elevator->to_floor);
		sleep(distance_to_destination * gv->elevator_speed);
		current_time = get_cur_time();
		thread_elevator->current_floor = person_in_elevator->to_floor;
		if((int)(current_time - gv->beginning_time) < gv->simulation_time)
		{
			printf("[%d] Elevator %d arrives at floor %d\n", (int)(current_time - gv->beginning_time), lv->tid, person_in_elevator->to_floor);
			printf("[%d] Elevator %d drops Person %d\n", (int)(current_time - gv->beginning_time), lv->tid, person_in_elevator->id);
			gv->num_people_finished ++;
		}
		else
		{
			break;
		}

		pthread_mutex_lock(&(g_s_buffer->mutex_lock));
		current_time = get_cur_time();
		pthread_mutex_unlock(&(g_s_buffer->mutex_lock));
		lv->related_num_persons ++;
	}
	pthread_exit((void *)lv->related_num_persons);
}

int main(int argc, char* argv[]) 
{
  	int             i;
  	LV              lvs_person_gen;
  	LV		  		lvs_elevator;
  	GV              gv;
  	pthread_t       *thrds_person_gen;
  	pthread_t	  	*thrds_elevator;
  	pthread_attr_t  *attrs_person_gen;
  	pthread_attr_t  *attrs_elevator;

  	srand(time(NULL));
  	// Check number of variables
  	if(argc != 7) 
  	{
    	fprintf(stderr, "Usage: %s num_elevators num_floors person_arrival_howoften elevator_speed simulation_time random_seed\n", argv[0]);
    	exit(1);
  	}
  	// Validate the inputs
  	for(i = 1; i < 7;i++)
  	{
  		int temp = atoi(argv[i]);
  		if(temp <= 0){
			fprintf(stderr, "the input should be larger than 0 or should be integer\n");
			exit(1);
		}
  	}
  	int num_elevators = atoi(argv[1]);
  	int num_floors = atoi(argv[2]);
  	int person_arrival_howoften = atoi(argv[3]);
  	int elevator_speed = atoi(argv[4]);
  	int simulation_time = atoi(argv[5]);
  	int random_seed = atoi(argv[6]);   
  	// Global variable
  	gv = (GV) malloc(sizeof(*gv));
  	double beginning_time = get_cur_time();
  	init_gv(gv, num_elevators, num_floors, beginning_time, person_arrival_howoften, elevator_speed, simulation_time, random_seed);
  	// Global variable initialization
  	init_sem_dll(&(gv->sem_buffer));   
  	lvs_person_gen = (LV) malloc(sizeof(*lvs_person_gen));
  	lvs_elevator = (LV) malloc(sizeof(*lvs_elevator)*num_elevators);
  	thrds_person_gen = (pthread_t*) malloc(sizeof(pthread_t));
  	thrds_elevator = (pthread_t*) malloc(sizeof(pthread_t)*num_elevators);
  	// All threads' attributes
  	attrs_person_gen = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
  	attrs_elevator = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*num_elevators);
	// Create the elevatorumer threads
  	for(i = 0; i < num_elevators; i++) 
  	{
    	init_lv(lvs_elevator+i, i, gv);
    	if(pthread_attr_init(attrs_elevator+i)) perror("attr_init()");
    	if(pthread_attr_setscope(attrs_elevator+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
    	if(pthread_create(thrds_elevator+i, attrs_elevator+i, thread_elevator, lvs_elevator+i)) 
		{
      		perror("pthread_create() elevator");
      		exit(1);
    	}
  	}
  	// Create the genearation people threads
  	for(i = 0; i < 1; i++) 
  	{
    	init_lv(lvs_person_gen+i, i, gv);
    	if(pthread_attr_init(attrs_person_gen+i)) perror("attr_init()");
    	if(pthread_attr_setscope(attrs_person_gen+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
    	if(pthread_create(thrds_person_gen+i, attrs_person_gen+i, thread_person, lvs_person_gen+i)) 
		{
      		perror("pthread_create() people generation");
      		exit(1);
    	}
  	} 
 	for(i = 0; i < 1; i++) 
  	{
    	pthread_join(thrds_person_gen[i], NULL);
  	}
  	for(i = 0; i < num_elevators; i++) 
  	{
    	pthread_join(thrds_elevator[i], NULL);
  	}
 	
  	printf("[%d] Simulation ends.\n", gv->simulation_time);
  	printf("Simulation result: %d people have started, %d people have finished during %d seconds.\n", gv->num_people_started, gv->num_people_finished, gv->simulation_time);
  
  	free(lvs_person_gen);
  	free(lvs_elevator);
  	free(attrs_person_gen);
  	free(attrs_elevator);
  	free(thrds_person_gen);
  	free(thrds_elevator);

  	return 0; 
}
