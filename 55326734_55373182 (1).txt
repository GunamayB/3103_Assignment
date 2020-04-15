#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>


using namespace std;

double* generate_frame_vector(int l);

void *camera(void* arg);
void *quantizer(void* arg);

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; // Mutex 1 to protect Front
static pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER; // Mutex 2 to protect Rear


struct frames
{
	int frame_num;
	double* fr;
};

static struct frames cache[6]; //Implemented circular queue and therefore we need size 6 for imitating a queue of size 5
static int front = 0;
static int rear = 0;


int main(int argc, char* argv[])
{
	
	int interval = 0;
	if (argc == 2) //check for correct input
	{
		interval = atoi(argv[1]);
		int c1 = 0;
		int q1 = 0;
		pthread_t cam;
		pthread_t quant;
		c1 = pthread_create(&cam, NULL, camera, (void*)&interval);
		if (c1)
		{
			cout << "Error when creating thread!" << endl;
			exit(-1);
		}
		q1 = pthread_create(&quant, NULL, quantizer, NULL);
		if (q1)
		{
			cout << "Error when creating thread!" << endl;
			exit(-1);
		}
		c1 = pthread_join(cam, NULL); //wait for camera thread to end
		q1 = pthread_join(quant, NULL);//wait for quantiser thread to end
		pthread_exit(NULL);

	}


}

void *camera(void* arg)
{
	int interval = *(int *)arg;
	int a = 0;
	int b = 0;
	double* f = generate_frame_vector(8); //generate a frame
	while (true)
	{
			if (((rear + 1) % 6) == front) //check if cache is full and only then try to lock mtx : this will avoid redundant waiting of camera 
			{
				a = pthread_mutex_lock(&mtx); //after quantiser eliminates a frame , camera will have updated value of front (only quantiser updates value of front, and only camera updates value of rear)
				while (((rear + 1) % 6) == front)
				{
					
					a = pthread_mutex_unlock(&mtx); //if cache full , camera will unlock mtx and allow quantiser to quantise frames from full cache and give updated value of front
					sleep(interval);
					a = pthread_mutex_lock(&mtx); //camera receives updated value of front
					
				}
				a = pthread_mutex_unlock(&mtx);
			}
			
			b = pthread_mutex_lock(&mtx2); //Camera enters critical section and adds a new frame, value of rear is updated here which is required by quantiser to check if cache is empty or not
			cache[rear].fr = f;
			rear++;
			rear = rear % 6;
			b = pthread_mutex_unlock(&mtx2); //unlock mtx2 and letquantiser have updated value of rear for checking
			f = generate_frame_vector(8); //generate new frame
			if (f == NULL)
			{
				break; 
			}
			sleep(interval); //check after interval seconds 
	}
	
	pthread_exit(NULL); //camera thread ended 
}

void *quantizer(void* arg)
{

	int done = 0;
	int a = 0;
	int b = 0;
	while (!done)
	{
		int tries = 0;
		if (front == rear) // if quantiser sees that cache is empty it will try to lock mtx2 and get updated value of rear , as rear is only updated by camera , (front stays unaffected as quantiser itself updates front)
		{
			b = pthread_mutex_lock(&mtx2);
			while (front == rear)
			{
				b = pthread_mutex_unlock(&mtx2); //if cache is empty then unlock mtx2 and allow camera to add frames in the cache 
				sleep(1);
				tries++;
				if (tries == 3)
				{
					done = 1;
					break;
				}
				b = pthread_mutex_lock(&mtx2); //lock mtx2 so updated value of rear can be received after camera has added frames
			}
			b = pthread_mutex_unlock(&mtx2);
		}
		

		if (done == 0)
		{
			a = pthread_mutex_lock(&mtx); // enter critical section to quantise frames and give updated values of front to help camera to check if cache is full or not 
			for (int i = 0; i < 8; i++)
			{
				if (cache[front].fr[i] <= 0.5)
				{
					cache[front].fr[i] = 0.0; //quantising values within the cache
				}
				else
				{
					cache[front].fr[i] = 1.0;
				}
			}
			for (int i = 0; i < 8; i++)
			{
				cout << cache[front].fr[i] << ".0 "; //displaying quantised values 
			}
			cache[front].fr = NULL;
			front++;
			front = front % 6;
			sleep(3);
			a = pthread_mutex_unlock(&mtx); //give updated value of front to camera for further checking 
			cout  << endl; 
		}

	}
	pthread_exit(NULL); //exit quantiser 
}