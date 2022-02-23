#include<bits/stdc++.h>
#include<unistd.h>
#include<sys/shm.h>
#include<sys/wait.h>
#include<pthread.h>
#include<time.h>
#include<sys/ipc.h>
#include<signal.h>

#define Q_SIZE 8

using namespace std;
//structure for job containing the required variables
struct job
{
    pid_t process_id;       //process id of the job
    int producer_number;    //producer no of the job
    int priority;       //priority of job
    int compute_time;   // computing time
    int job_id;     //id of job
};
//structure for storing variables used in shared memory 
struct sharedmemory
{
    int job_created;    // to store number of jobs created
    int job_completed;  // to store number of jobs completed
    int num_of_jobs;    // to store total number of jobs
    pthread_mutex_t mutex;  //mutex lock
    job jobqueue[Q_SIZE+1]; // queue for storing various jobs. Maintained as a priority queue using heaps.
    int runningjobs;
};

void heapifyup(sharedmemory *mem, int pos)      //used for inserting the job in the priority queue(heap)
{
    while(1)        //standard procedure of heapify up
    {
    	if(!((pos/2)>0))
    		break;
        else if(mem[0].jobqueue[pos/2].priority>mem[0].jobqueue[pos].priority)
			break;
        else
      		{
	        job temp = mem[0].jobqueue[pos/2];
			mem[0].jobqueue[pos/2]= mem[0].jobqueue[pos];
			mem[0].jobqueue[pos]=temp;
      		}
        pos=pos/2;
    }
}

void heapifydown(sharedmemory *mem, int pos)    //used during retrieving the job from heap
{
    int right = 2*pos+1, left = 2*pos, smallest;    //standard procedure of heapify down
    int n = mem[0].num_of_jobs;
    if(left <= n)
        if(mem[0].jobqueue[left].priority>mem[0].jobqueue[pos].priority) 
			smallest=left;
        else 
			smallest=pos;
    else 
		smallest=pos;

    if(right <= n && mem[0].jobqueue[right].priority>mem[0].jobqueue[smallest].priority)
		smallest=right;
    
    if(smallest != pos)
    {
        job temp=mem[0].jobqueue[smallest];
	    mem[0].jobqueue[smallest]=mem[0].jobqueue[pos];
	    mem[0].jobqueue[pos]=temp;
        heapifydown(mem, smallest);
    }
}

int retrieve_job(sharedmemory *mem, job *j)    // for retrieving jobs 
{
    if(mem[0].num_of_jobs==0) return -1;        // if queue is empty, return -1
    *j = mem[0].jobqueue[1];
    mem[0].jobqueue[1]=mem[0].jobqueue[mem[0].num_of_jobs];   // made top element equal to last element, that is delete the top entry
    mem[0].num_of_jobs--;   // decrement number of jobs
    heapifydown(mem,1);    //heapifydown to maintain the priority
    return 0;
}

int insert_job(sharedmemory *mem, job j)    //for inserting job
{
    if(mem[0].num_of_jobs + mem[0].runningjobs== Q_SIZE) return -1;     // if numofjobs+runningjobs = queue size, return -1
    mem[0].num_of_jobs+=1;  //increment numofjobs counter
    mem[0].jobqueue[mem[0].num_of_jobs] = j;    //added the job to the last of queue
    heapifyup(mem,mem[0].num_of_jobs);      //heapify up to maintain the priority
    return 0;
}

int main()
{
    int NC,NP,total_jobs;           //variables for number of consumer,producer and totaljobs
    cout<<"NP: ";
    cin>>NP;
    cout<<"NC: ";
    cin>>NC;
    cout<<"total_jobs: ";
    cin>>total_jobs;
    int shmid = shmget(IPC_PRIVATE, sizeof(sharedmemory), 0700|IPC_CREAT);  //creater shared memory
    if(shmid<0)
    {
        cout<<"Error"<<endl;
        exit(1);
    }
    sharedmemory *mem = (sharedmemory *)shmat(shmid, NULL, 0);  //attaching the shared memory to a physical address
    mem[0].num_of_jobs = mem[0].job_created = mem[0].job_completed = mem[0].runningjobs= 0;     //initialising the structure variables
    //initialising mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(mem[0].mutex), &attr);
    clock_t st = clock(), end;

    for(int i=0;i<NC; ++i)          //loop for consumer process
    {
        pid_t consumer_id = fork(); //forked the process
        if(consumer_id==0)
        {
            pid_t id = getpid();      //get process id
            srand((1+(getpid()<<16))^time(NULL)); //seeding random generator
            while(1)
            {
                sleep(rand()%4);    //random sleep between 0 to 3 seconds
                job temp;
                while(1)    //try to retrieve every 10 ms
                {
                    pthread_mutex_lock(&(mem[0].mutex));        //lock before accessing the mem
                    if(retrieve_job(mem,&temp)!=-1)         // retrieve job by accessing mem
                    {
                        mem[0].runningjobs++;       // now this job is running so increment runningjobs
                        pthread_mutex_unlock(&(mem[0].mutex));   //unlock after the access   
                        break;          //break when a job is retrieved
                    }
                    pthread_mutex_unlock(&(mem[0].mutex));
                    usleep(10000);      //sleep for 10ms
                }
                sleep(temp.compute_time);       //sleeps for computetime
                pthread_mutex_lock(&(mem[0].mutex));  //lock before accessing the mem
                mem[0].runningjobs--;           //job completed so decrement
                if( mem[0].job_completed >=total_jobs)      // before updating jobcompleted, we check if required number is already reached
                {       
                    pthread_mutex_unlock(&(mem[0].mutex));  //unlock after the access              
                    break;
                }
                mem[0].job_completed+=1;        //accessing mem
                pthread_mutex_unlock(&(mem[0].mutex));  //unlock after the access 
                cout<<"Consumer: "<<i+1<<", Consumer PID: "<<id<<", ";
                cout<<"Producer: "<<temp.producer_number<<", Producer PID: "<<temp.process_id<<", "<<"Priority: "<<temp.priority<<", Compute Time: "<<temp.compute_time<<", "<<"Job ID: "<<temp.job_id<<endl;
            }
            exit(0);
        }
    }
    for(int i=0;i<NP;++i)       //loop for producer
    {   
        pid_t producer_id = fork();     //forked the process
        if(producer_id==0)
        {
            pid_t id = getpid();    //get process id
            srand((1+(getpid()<<16))^time(NULL));       //seed random generator
            while(mem[0].job_completed<total_jobs)   // while num of jobs is less than total jobs, run the producer
            {
                job temp;
                temp.process_id = id;           // initialising the job to be inserted
                temp.producer_number = i+1;
                temp.priority = (rand()%10) + 1;
                temp.job_id = (rand()%100000) + 1;
                temp.compute_time = (rand()%4) + 1;
                sleep(rand()%4);        //sleep for random time between 0-3 seconds
                while(mem[0].job_completed<total_jobs)
                {
                    pthread_mutex_lock(&(mem[0].mutex));    // lock before accessing mem
                    if(insert_job(mem,temp)!=-1)        //insert operation which accesses mem
                    {
                        mem[0].job_created+=1;      //mem access
                        cout<<"Producer: "<<temp.producer_number<<", Producer PID: "<<temp.process_id<<", "<<"Priority: "<<temp.priority<<", Compute Time: "<<temp.compute_time<<", "<<"Job ID: "<<temp.job_id<<endl;
                        pthread_mutex_unlock(&(mem[0].mutex));  //unlock after the access
                        break;
                    }
                    pthread_mutex_unlock(&(mem[0].mutex));  //unlock after the access
                    usleep(10000);      //sleep for 10ms
                }
                if(mem[0].job_completed>=total_jobs)        //break if jobs completed is >= total jobs
                    break;
                
            }
            exit(0);
        }
    }
	double time_taken = 0;
    while(1)    //checks completed jobs every 10ms and kills when total jobs is reached
	{
		usleep(10000); //sleep for 10ms
		time_taken += 0.01;   //increment time
        pthread_mutex_lock(&(mem[0].mutex));        // lock before accessing mem
		if(mem[0].job_completed >= total_jobs)    //check jobs completed
		{
            pthread_mutex_unlock(&(mem[0].mutex));        // unlock after accessing mem
			end = clock(); 
			time_taken += ((double)(end - st)) / CLOCKS_PER_SEC ; //calculate taken time
			cout<<setprecision(4)<<fixed<<"TIME TAKEN TO RUN "<<total_jobs<<" JOBS: "<<time_taken<<" seconds"<<endl;  //output
			shmdt(mem);  
            shmctl(shmid,IPC_RMID,NULL);
            kill(-getpid(), SIGQUIT); //quits all the processes 
		}
        pthread_mutex_unlock(&(mem[0].mutex));        // unlock after accessing mem
	}
    
}