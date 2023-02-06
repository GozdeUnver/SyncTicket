// Name Surname: Gözde Ünver
// Project: Cmpe 322.01 Sync-Ticket Project
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <iostream> 
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>

using namespace std;

pthread_mutex_t mtx3;
pthread_mutex_t mtx2;
pthread_mutex_t mtx;
sem_t * sem;
sem_t * sem2;
sem_t * sem3;
ifstream inputFile;
ofstream outputFile;
vector<int> seats;
int headClient[4];
string availableTeller;
bool availability[]={true,true,true};
bool cont=true;
int times[3]={0,0,0};
int clientLeft=INT32_MAX;
int go=0;

// Client thread
void * client(void *parameters){

    // Attributes of a client are stored in these variables.
    int *values=(int *)parameters;
    int clientName=*values;
    int arrivalTime=*(values+1);
    int serviceTime=*(values+2);
    int seatNo=*(values+3);

    // Each client sleeps for its arrival time before going into the client queue
    usleep(arrivalTime*1000);

    // The first client that finishes sleeping goes into the client queue to be served. This structure is done by mutex. Mutex locks the 
    // below critical section code for other clients until the critical section finishes. In this section, the attributes of the client 
    // that is at the head of the hypothetical client queue are stored in the headClient array. An available teller reads these attrivutes
    // and when the reading is done the mutex is unlocked so that the next client can be at the head of the client queue.  
    pthread_mutex_lock(&mtx);

    headClient[0]=clientName;
    headClient[1]=arrivalTime;
    headClient[2]=serviceTime;
    headClient[3]=seatNo;

    // This semaphore makes the available teller wait for a client to get the head of the queue. When the attributes of the head cient
    // are written into the headClient array then the value of the sem is incremented by one.
    sem_post(sem);

    // This semaphore makes the head client wait for teller to read its attributes before unlocking the mutex to enable the next client
    // to become the head client.
    sem_wait(sem2);

    pthread_mutex_unlock(&mtx);

    // Client leaves the system after sleeping for its service time.
    usleep(serviceTime*1000);
    pthread_exit(NULL);
}

// Teller thread
void * teller(void * parameter){

    // Stores the name of the teller
    string nameOfTeller=*(string *)parameter;

    // Prints that the teller has arrived.
    if(nameOfTeller=="A")
    outputFile<<"Teller A has arrived."<<endl;
    else if(nameOfTeller=="B")
    outputFile<<"Teller B has arrived."<<endl;
    else
    outputFile<<"Teller C has arrived."<<endl;
    

    // Teller names are enumareted by using the num variable. Teller A gets 0, B gets 1 and C gets 2. 
    int num=0;
    if(nameOfTeller=="B")
    num=1;
    else if(nameOfTeller=="C")
    num=2;

    // Information about the service to a client by the teller is sotred in this string and it is written into the output file.
    string result="";

    // Loops continues running until all the clients have left the client queue. This is checked by the cont boolean variable.
    // cont is initially true and it is updated to false when all the client threads are terminated.
    while(cont){
        
        // When the teller is not servicing it is available.
        availability[num]=true;

        // Initially the available teller is the same as this teller
        availableTeller=nameOfTeller;

        // This section determines the available teller which should accept the new client. It is the teller that comes first in terms of 
        // alphabetical order and also not busy with servicing another client at the moment.
        if((num==1)&&availability[0]){
            availableTeller="A";
        }else if((num==2)&&availability[0]){
            availableTeller="A";
        }else if((num==2)&&availability[1]){
            availableTeller="B";
        }

        // If the current thread is the chosen available thread then it goes in here.
        if((nameOfTeller==availableTeller)){

            // Available thread locks the below critical section for other threads because this thread reads the head client and 
            // when it does that this thread becomes unavailable and no other teller should be able to read that client.
            pthread_mutex_lock(&mtx2);

            // Although this thread has reached this part of the code, it doesn't mean that there are still clients waiting in the queue.
            // It may be because the last teller that serviced the last client unlocked the mutex and thus this thread is able to go into 
            // this section of the code. If it is that so, it leaves the critical section.
            if(clientLeft==0){
            pthread_mutex_unlock(&mtx2);
            break;
            }
            
            // If the head of the client hasn't been waited by a teller (the new head client is not assigned yet, go==0) then it is waited. 
            // If the head client has already been assigned but since the available teller is corrected, 
            // no need to wait for the head client again (when go==1).
            if(go==0)
            sem_wait(sem);

            go=1;

            // Checks whether this teller should serv to the client because there may be another teller with high alphabetical order and
            // has finished its task but due to the structure of mutexes, this thread was the first one to go into the critical section.
            // If there is another available thread with higher priority than this thread than the mutex is unlocked so that other thread can 
            // go into the critical section and serv the client.
            if(((num==1)&&availability[0])||((num==2)&&availability[0])||((num==2)&&availability[1])){
                //availability[num]=true;
                pthread_mutex_unlock(&mtx2);
                continue;
            }

            // This thread is the correct one to serve the client so it should not be available for other clients.
            availability[num]=false;
            
            // Since the head client is served, the number of clients that aren't served yet should decrease.
            clientLeft--;

            // Information string about servicing the client will be stored in here.
            result="";

            // It is reset to zero so that next head client can be waited.
            go=0;

            result+="Client"+to_string(headClient[0])+" requests seat "+to_string(headClient[3])+", reserves ";

            // If client requests a seat that is out of the limits of the theater than it is initialized to 1. If seat 1 is full than
            // the next seat with the smallest number will be assigned to the client.
            if(seats.size()-1<headClient[3]){
                headClient[3]=1;
            }

            // If the seat that the client requests is available then it is assigned to the client.
            if(seats[headClient[3]]==0){
                seats[headClient[3]]=1;
                result+="seat "+to_string(headClient[3])+". Signed by Teller "+nameOfTeller+".";
            }
            // The seat that the client requests is not available so it checks for an available seat with smallest number.
            else{
                int i=1;
                for(i=1;i<seats.size();i++){
                    if(seats[i]==0){
                        seats[i]=1;
                        result+="seat "+to_string(i)+". Signed by Teller "+nameOfTeller+".";
                        break;
                    }
                }

                // If no available seat is found, then the client doesn't get any seat.
                if(i==seats.size()){
                    result+="None. Signed by Teller "+nameOfTeller+".";
                }
            }
            
            // Service time of the client is stored in the times array at the teller thread index. This is done 
            // because right after this thread releases the current head client, the next client will change the attributes of the head 
            // client so the time will be lost.
            times[num]=headClient[2];

            // Causes the next client to become the head client by letting the current client unlock its mutex.
            sem_post(sem2);

            // This teller thread unlocks the mutex so that other threads can read the next client.
            pthread_mutex_unlock(&mtx2);
            
            // This threas sleeps for the service time of the client.
            usleep(times[num]*1000);

            // Information about the serviced client is written into the output file. The line about the client whose service is completed earlier
            // than the other clients, appears before. In order to prevent threads to write lines into the file simultaneously,
            // mutex is used.
            pthread_mutex_lock(&mtx3);
            outputFile<<result<<endl;
            pthread_mutex_unlock(&mtx3);
            
        }  
    }
    // When all clients left the queue, teller threads exit.
    pthread_exit(NULL);
}
int main(int argc, char *argv[]) {
    
    // Input file is opened
    inputFile.open(argv[1]);

    // Output file is opened/created. If there already exists a file with the same name, its contents are erased before any write operation.
    outputFile.open(argv[2],ios::trunc);

    // When the program starts this line is written into the output file.
    outputFile<<"Welcome to the Sync-Ticket!"<<endl;

    int numOfClients, numOfSeats;
    string nameOfTheater, temp, token;

    getline(inputFile,nameOfTheater);
    
    // Capacity of the theater is determined by the initial character of the theaters. OdaTiyatrosu has 60, UskudarStudyoSahne has 80
    // and KucukSahne has 200 seats.
    if(nameOfTheater[0]=='O'){
    numOfSeats=60;
    }
    else if(nameOfTheater[0]=='U'){
    numOfSeats=80;
    }
    else{
    numOfSeats=200;
    }

    // Number of clients is read from the file
    getline(inputFile,temp);
    numOfClients=stoi(temp);
    
    // This array stores the 4 attributes of each client
    int clientAttributes[numOfClients][4];

    // Seats vector has a size of number of seats and intially filled with zeros. When a client reserves a seat, the seat values becomes 1.
    seats.assign(numOfSeats+1,0);

    // Number of clients left unserved is stored in clientLeft and it is initially equal to the number of clients.
    clientLeft=numOfClients;

    // Stores the client thread IDs
    pthread_t clientIds[numOfClients]; 

    // Stores teller thread IDs
    pthread_t tellerIds[3];

    // Mutexes are initialized
    pthread_mutex_init(&mtx, NULL);
    pthread_mutex_init(&mtx2, NULL);
    pthread_mutex_init(&mtx3, NULL);

    const char* name1="s1";
    const char* name2="s2";
    const char* name3="s3";

    int val1=0;
    int val2=0;
    int val3=0;

    // Semaphores are initialized
    sem = sem_open(name1, O_CREAT, 0666, val1);
    sem2 = sem_open(name2, O_CREAT, 0666, val2);
    sem3 = sem_open(name3, O_CREAT, 0666, val3);

    // Each teller has a name
    string tellerNameA="A";
    string tellerNameB="B";
    string tellerNameC="C";

    // Teller threads are initialized. First A then B then C tellers are initialized and this is done via sleeping the main thread for 10 ms.
    pthread_create(&tellerIds[0], NULL, &teller, &tellerNameA);
    usleep(10000);
    pthread_create(&tellerIds[1], NULL, &teller, &tellerNameB);
    usleep(10000);
    pthread_create(&tellerIds[2], NULL, &teller, &tellerNameC);
    usleep(10000);

    // Stores the index of attributes in a row for client in the clientAttributes array
    int j=0;

    // Stores the index of the client
    int k=0;

    // Reads the attributes of the clients
    while(getline(inputFile,temp)){

        // Only the numerical part of the string is hold
        stringstream splitLine(temp.substr(6,temp.length()-6));

        j=0;

        // Splits the line from the commas and each token is an attribute for a client
        while(getline(splitLine,token, ',')){
            clientAttributes[k][j]=stoi(token);
            j++;
        }

        // Client thread is created with these attributes
        pthread_create(&clientIds[k], NULL, &client, &clientAttributes[k]);
        k++;
    }
    
    // Main thread waits for the termination of all clients
    for(int i=0;i<numOfClients;i++)
    pthread_join(clientIds[i], NULL);

    // After all the client threads are terminated, the boolean variable cont is set to false so that teller threads can go out of their while loops
    cont=false;

    // Main thread waits for the termination of all teller threads
    for(int i=0;i<3;i++)
    pthread_join(tellerIds[i], NULL);

    // After all the threads are terminated, finish line is written into the output file
    outputFile<<"All clients received service."<<endl;

    // Files, mutexes and semaphores are destroyed
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&mtx2);
    pthread_mutex_destroy(&mtx3);
    sem_close(sem);
    sem_unlink(name1);
    sem_close(sem2);
    sem_unlink(name2);
    sem_close(sem3);
    sem_unlink(name3);
	pthread_exit(NULL);
    inputFile.close();
    outputFile.close();

    return 0;
}

