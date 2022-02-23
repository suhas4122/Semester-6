#include<bits/stdc++.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<time.h>
#include<signal.h>
#include<pthread.h>


#define Q_SIZE 8

using namespace std;
time_t st, endt;
int runningjobs;
typedef struct job                              // struct for job 
{
    pid_t process_id;                           // all the variables as described in p.s.
    int producer_number;
    int priority;
    int compute_time;
    int job_id;
}job;

job jobqueue[Q_SIZE+1];                         // the global variables that will be shared among all the threads
int job_created;
int job_completed;
int num_of_jobs;
pthread_mutex_t mutex1;

int total_jobs;
double time_taken;
void heapifyup( int pos)                        // for inserting items in the priority queue
{
    while(1)
    {
    	if(!((pos/2)>0))
    		break;
        else if(jobqueue[pos/2].priority>jobqueue[pos].priority)
			break;
        else
      		{
	        job temp = jobqueue[pos/2];
			jobqueue[pos/2]= jobqueue[pos];
			jobqueue[pos]=temp;
      		}
        pos=pos/2;
    }
}

void heapifydown( int pos)                              // for maintaining priority queue while extracting from priority queue
{
    int right = 2*pos+1, left = 2*pos, smallest;
    int n = num_of_jobs;
    if(left <= n)
        if(jobqueue[left].priority>jobqueue[pos].priority) 
			smallest=left;
        else 
			smallest=pos;
    else 
		smallest=pos;

    if(right <= n && jobqueue[right].priority>jobqueue[smallest].priority)
		smallest=right;
    
    if(smallest != pos)
    {
        job temp=jobqueue[smallest];
	    jobqueue[smallest]=jobqueue[pos];
	    jobqueue[pos]=temp;
        heapifydown(smallest);
    }
}

int retrieve_job( job *j)                       // function to retrieve job from the priority queue
{
    if(num_of_jobs==0) return -1;
    *j = jobqueue[1];
    jobqueue[1]=jobqueue[num_of_jobs];
    num_of_jobs--;
    heapifydown(1);
    return 0;
}

int insert_job(job j)                           // function to insert job in the priority queue
{
    if(num_of_jobs+runningjobs== Q_SIZE) return -1;     //if numofjobs + runningjobs ==queuesize, return -1
    num_of_jobs+=1;
    jobqueue[num_of_jobs] = j;
    heapifyup(num_of_jobs);
    return 0;
}


void *consumer(void *id)                            // function for consumer
{
    pthread_mutex_lock(&(mutex1));                  // lock before accessing memory
    if(job_completed>=total_jobs)                     
    {
        pthread_mutex_unlock(&(mutex1));
        pthread_exit(0);
    }
    pthread_mutex_unlock(&(mutex1));    

    int *t = (int *) id;
    while(1)
    {
        pthread_mutex_lock(&(mutex1));                  // lock before accessing memory
        if(job_completed>=total_jobs)                     
        {
            pthread_mutex_unlock(&(mutex1));
            pthread_exit(0);
        }
        pthread_mutex_unlock(&(mutex1));
            // break;
        srand(time(NULL) ^ (pthread_self()));                       // random seed and sleep for random time as asked
        sleep(rand()%4);
        job temp;
        while(1)
        {
            pthread_mutex_lock(&(mutex1));          // lock before accessing memory
             if(job_completed>=total_jobs)                     
            {
                pthread_mutex_unlock(&(mutex1));
                pthread_exit(0);
            }
            if(retrieve_job(&temp)!=-1)                // if success then print the details
            {
                runningjobs++;          // job is running now so increment
                pthread_mutex_unlock(&(mutex1));        // unlock

                sleep(temp.compute_time);
                pthread_mutex_lock(&(mutex1));         
                runningjobs--;      //job completed so decrement
                if(job_completed>=total_jobs)                   //before updating jobscompleted, check is required jobs are already completed  
                {
                    pthread_mutex_unlock(&(mutex1));
                    pthread_exit(0);
                }
                pthread_mutex_unlock(&(mutex1));  
                cout<<"Consumer: "<<*t<<", Consumer PID: "<<pthread_self()<<", ";
                cout<<"Producer: "<<temp.producer_number<<", Producer PID: "<<temp.process_id<<", "<<"Priority: "<<temp.priority<<", Compute Time: "<<temp.compute_time<<", "<<"Job ID: "<<temp.job_id<<"\n";
                pthread_mutex_lock(&(mutex1));          // lock before accessing memory
                job_completed+=1;
                pthread_mutex_unlock(&(mutex1));        // unlock
              
                break;
            }
            else 
                pthread_mutex_unlock(&(mutex1));        // unlock
            usleep(10000);                          // sleep for stated time
        }
    }
    exit(0);
}

void *producer(void *id)                            // function for producer
{
    int *t = (int *)id;
    while(1)
    {
        job temp;                                       // create the job
        temp.process_id = pthread_self();
        temp.producer_number = *t;
        temp.priority = (rand()%10) + 1;
        temp.job_id = (rand()%10000) + 1;
        temp.compute_time = (rand()%4) + 1;
        sleep(rand()%4);                                // sleep for random time
        pthread_mutex_lock(&(mutex1));                  // lock before accessing memory
        if(job_completed>=total_jobs)                     
        {
            pthread_mutex_unlock(&(mutex1));
            pthread_exit(0);
        }
        else if(insert_job(temp)!=-1)           // try inserting in the queue and if success print the details
        {
            job_created+=1;
            cout<<"Producer: "<<temp.producer_number<<", Producer PID: "<<temp.process_id<<", "<<"Priority: "<<temp.priority<<", Compute Time: "<<temp.compute_time<<", "<<"Job ID: "<<temp.job_id<<"\n";
            pthread_mutex_unlock(&(mutex1));    // unlock
        }
        else
        {
            pthread_mutex_unlock(&(mutex1));    // if failed then unlock and sleep
            usleep(10000);
        }

    }
    exit(0);
}
int main()
{
    int NP,NC;                                                          // taking input
    cout<<"NP: ";
    cin>>NP;
    cout<<"NC: ";
    cin>>NC;
    cout<<"total_jobs: ";
    cin>>total_jobs;
    pthread_t producer_threads[NP], consumer_threads[NC];               // maintaining arrays for producer consumer threads
    num_of_jobs = job_created = job_completed = 0;
    runningjobs =0;
    pthread_mutex_init(&(mutex1), NULL);

    pthread_attr_t attr2;
    pthread_attr_init(&attr2);
    int a1[NC],a2[NP];
    time(&st);
    for(int i=0;i<NC;i++)
    {
        a1[i] = i+1;
        int *t = &a1[i];
        pthread_create(&consumer_threads[i],&attr2,consumer,(void*)t);      // creating consumer threads
    }
    for(int i=0;i<NP;i++)
    {
        a2 [i] = i+1;
        int *t = &a2[i];
        pthread_create(&producer_threads[i],&attr2,producer,(void*)t);      // creating producer threads
    }
    for(int i=0;i<NC;i++) pthread_join(consumer_threads[i],NULL);           // starts the threads and their functions
    for(int i=0;i<NP;i++) pthread_join(producer_threads[i],NULL);
    time_taken = 0;
    time(&endt);
    double time_taken = double(endt - st); 
    cout<<setprecision(4)<<fixed<<"TIME TAKEN TO RUN "<<total_jobs<<" JOBS: "<<time_taken<<" seconds"<<endl;  //output
    kill(-getpid(), SIGQUIT); //quits all the processes 
    return 0;
}