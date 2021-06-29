#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h> 

// The maximum number of patients threads.
#define MAX_PATIENTS 48 
// The number of healthcare staff threads.
#define nurseCount 8
int remainingPatientCount = 0;//used for checking the how many patients left
int currentTestUnitCount=0;//unitID, shows which unit is currently in
void *patient(void *num);//used at creating patient threads
void *nurse(void *);//used at creating nurse threads
void timePassed(int secs);//used for the passage of time
void visual(int currentTestUnitCount);//used for visualizing the current status of the test unit

//Define the semaphores.
// testUnit is to track the number of patient at a test unit
sem_t testUnit[8];
// ventilate is used to allow the nurse to ventilate the room until a patient arrives.
sem_t ventilate[8];// 0 means nurse is ventilating the room and 1 means not
// testStart is used to make the patients to wait until the nurse is start testing.
sem_t testStart[8];//1 means nurse is testing and 0 means not 
//lock is used to prevent patients from entering at the same time
sem_t lock;//0 means dont bring the patients in, 1 means bring

int main(int argc, char *argv[])
{
	//Define the threads.
	pthread_t nurseID[nurseCount];
	pthread_t patientID[MAX_PATIENTS];

	int i, input, numPatients;
	int patientNumber[MAX_PATIENTS]; int nurseNumber[nurseCount];
	//taking number of patients from user
	printf("Maximum number of customers can only be 48.Please try numbers in multiples of 3. Enter number of customers.\n");
	scanf("%d",&input);
	numPatients = input;

	remainingPatientCount= numPatients;
	//checking if the entered input is more than defined
	if (numPatients > MAX_PATIENTS) {
		printf("The maximum number of Customers is %d.\n", MAX_PATIENTS);
		system("PAUSE");
		return 0;
	}

	printf("Solution to the Covid-19 Test Unit problem using semaphores.\n\n");

	//process of assigning id to patients
	for (i = 0; i < MAX_PATIENTS; i++) {
		patientNumber[i] = i;
	}
	//process of assigning id to nurses
	for (i = 0; i < nurseCount; i++) {
		nurseNumber[i] = i;
	}

	// Initialize the semaphores with initial values
	for (i = 0; i < nurseCount; i++) {
		sem_init(&testUnit[i], 0, 3);// 3 chair for test unit
		sem_init(&ventilate[i], 0, 0);//0 means ventilate, 1 means not
		sem_init(&testStart[i], 0, 0);//1 means testing, 0 means not
		}
		sem_init(&lock,0,1);//0 means dont bring the patients in, 1 means bring

	// Create the nurse threads
	for (i = 0; i < nurseCount; i++) {
		pthread_create(&nurseID[i], NULL, nurse, (void *)&nurseNumber[i]);
		//at start all nurses ventilating the units because there is no patients
		printf("The nurse is ventilating unit %d\n",nurseNumber[i]);
	}
	// Create the patient threads
	for (i = 0; i < numPatients; i++) {
		pthread_create(&patientID[i], NULL, patient, (void *)&patientNumber[i]);
	}

	// Join each of the threads to wait for them to finish.
	for (i = 0; i < numPatients; i++) {
		pthread_join(patientID[i],NULL);
	}
	// to send all nurses home
	for (i = 0; i < nurseCount; i++) {
		sem_post(&ventilate[i]);
	}
	// Join each of the threads to wait for them to finish.
	for (i = 0; i < nurseCount; i++) {
		pthread_join(nurseID[i],NULL);
	}
	// killing all treads
	for (i = 0; i < nurseCount; i++) {
		sem_destroy(&testUnit[i]);
		sem_destroy(&ventilate[i]);
		sem_destroy(&testStart[i]);
	}
	sem_destroy(&lock);
	
	system("PAUSE");

	return 0;
}
void *patient(void *number) 
{
	int patient_id = *(int *)number;//patientID
	printf("\nPatient %d arrived at hospital.\n", patient_id);
	//since a patient entered the unit, dont let in a new patient just for now(we will post it at nurse function)
	sem_wait(&lock);
	//decrease the number of free space in the unit when patient entered the unit
	sem_wait(&testUnit[currentTestUnitCount]); 
	printf("Patient %d is at Covid-19 Test Unit waiting room %d\n", patient_id,currentTestUnitCount);
	printf("Patient %d is filling the form.\n", patient_id);
	timePassed(1);
	//since patient entered the unit, nurse stops ventilating room
	sem_post(&ventilate[currentTestUnitCount]);
	//nurse wait until the unit gets full
	sem_wait(&testStart[currentTestUnitCount]);
	printf("Patient %d leaving test unit%d.\n", patient_id,currentTestUnitCount);
}
void *nurse(void *number)
{
	int nurse_id = *(int *)number;//nurseID
	int currentTestRemain;
	while (remainingPatientCount>2) //Since there is no test when there are less than 2 patient, it continues until then
	{
		sem_wait(&ventilate[nurse_id]);
		// to get the value of current patient at unit
		sem_getvalue(&testUnit[nurse_id], &currentTestRemain);

		if(currentTestRemain == 0){//means unit is full
			//to make sure the same room doesn't work constantly
			currentTestUnitCount=currentTestUnitCount+1;
			if(currentTestUnitCount>=8){
			currentTestUnitCount=0;
			}

			printf("The status of unit %d\n",nurse_id);
			//visuals the unit
			visual(currentTestRemain);
			printf("Covid-19 Test Unit 5's medical staff announce:\nLet's start! Please, pay attention to your social distance and hygiene, use a mask.\n", currentTestRemain);
			printf("The nurse %d is testing...\n",nurse_id);
			//now a new patient can entered the room
			sem_post(&lock);
			timePassed(3);
			printf("The nurse %d has finished testing.\n",nurse_id);
			printf("The status of unit end of testing %d\n",nurse_id);
			for (int i = 0; i < 3; i++) {
				sem_post(&testUnit[nurse_id]);//The free space in the unit increases as the test done, patient will leave
				sem_getvalue(&testUnit[nurse_id], &currentTestRemain);
				visual(currentTestRemain);
				sem_post(&testStart[nurse_id]);//means test kepp going
				remainingPatientCount=remainingPatientCount-1;//when patient leave then remaining decrease
			}
			
						
			printf("The nurse is ventilating unit %d\n",nurse_id);
			
		}
		else if(currentTestRemain == 1 | currentTestRemain == 2){//means unit is not full
		printf("The status of unit %d\n",nurse_id);
		visual(currentTestRemain);
		printf("Covid-19 Test Unit 5's medical staff announce:\nThe last %d people, Let's start! Please, pay attention to your social distance and hygiene, use a mask.\n", currentTestRemain);
		//now a new patient can entered the room
		sem_post(&lock);}
		}
	printf("The nurse %d is going home for the day.\n",nurse_id);
}
void timePassed(int secs) {
	int len = 1; // Generate an arbit number
	sleep(len);
}
void visual(int currentTestRemain) {
	int i;
	for (i = 0; i < (3-currentTestRemain); i++) {
		printf("[ X ]");
	}
	for (i = 0; i < currentTestRemain; i++) {
		printf("[ ]");
	}
	printf("\n");
}
