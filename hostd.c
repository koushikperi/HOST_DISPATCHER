#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "hostd.h"


int main(int argc, char** argv){

        char *filename;
        
        int i;

        if(argc !=2){
                printf("Error: usage ./hostd dispatch_list.txt\n");
                exit(1);
        }
	
	printf(" \n\n*************************Group 2 Host Dispatcher Notes *************************\n");
	printf(" PRINT OPTIONS (Set as #define in hostd.h)\n");
	if(_PRINT_MEMORY_)
		printf(" ----#define _PRINT_MEMORY_ is on:\n");
	else
		printf(" ----#define _PRINT_MEMORY_ is off:\n");
	printf("----This option prints the array that acts as our representation of main memory\n\n");
	if(_PRINT_ACTIONS_)
		printf(" ----#define _PRINT_ACTIONS_ is on:\n");
	else
		printf(" ----#define _PRINT_ACTIONS_ is off:\n");
	printf("----This option prints useful information such as how much memory is left after \n\teach allocation, what resources were allocated and other stuff\n");

	printf(" *************************Group 2 Host Dispatcher Notes *************************\n");
	sleep(2);

        filename = argv[1];
        parse_input_process_list(filename);
	zeroMemory(0, HOST_MEM-1);
        dispatcher();

        return 0;
}


/*****************************************************/
/* Koushik Peri - Start of the code I worked on here */
/*****************************************************/

void zeroMemory(int s, int e){
	int i;
	for(i = s; i<=e; i++){
		memory[i] = 0;
	}
}

//fill out the indicated bytes in memory to indicate in usage
void fillMemory(int start, int end){
	int i;
	for(i = start; i<=end; i++){
		memory[i] = 1;
	}

}

//visualizes memory onto screen
void printMemory(){
#if _PRINT_MEMORY_
	int i = 0;
	for(i = 0; i<HOST_MEM; i++){
		printf("%d ",memory[i]);                            
	}
	printf("\n");
#endif

}

//takes process struct and puts it in appropriate queues
void dispatch_to_appropriate_queue(process_t * temp){

                process_t* it = NULL, *prev = NULL, **source = NULL;
        
                if(temp->priority == 0)
		{
                        source = &rt_list;
                }
		else
		{
                        source = &usr_list;
		}

                //decide which queue to put it in
                //as well as the order to put it in -- based on arrival time
                if((*source)==NULL){
                        (*source) = temp;
                        temp->next_process = NULL;
                }else{
                        it = (*source);
                        prev = (*source);

                        while(it!=NULL){
                                if(it->arrival_time>=temp->arrival_time){
                                        temp->next_process = it;
                                        if(it!=(*source))
                                                prev->next_process = temp;
                                        else        
                                                 (*source)=temp;
                                        break;
                                }else{
                                        prev = it;
                                        it = it->next_process;
                                }
                        }

                        if(it==NULL){
                                prev->next_process = temp;
                                temp->next_process = NULL;
                        }
                }
}


void dispatcher(){

        //checks if a process is in the real time queue
        //allocates memory for it and runs it (aka create new proc for it, print info every second for 20s)
        //...
        int disp_clock = 0, rc;
        process_t* it = NULL, *tt;

	initialize_resources();

        while(rt_list!=NULL | usr_list!=NULL | usrq1_h!=NULL | usrq2_h!=NULL | usrq3_h !=NULL)
	{ 
		printMemory();
		//This is for RT processes that have arrived
		//Execute them immediately
                while(rt_list!=NULL && rt_list->arrival_time<=disp_clock){
                	it = rt_list;
			if(allocate_memory(it)){
				printMemory();
				exec_rt(it);
				disp_clock = disp_clock + it->proc_time;
				remove_from_memory(it->id);
				free(it);
				rt_list = rt_list->next_process;
				printf("\n");
			}
			else{
				printf("yeah..we have a problem\n");
				//this would never happen unless rt processess need more than 64 bytes
			}

		}
		
		//This is to put user processess into their respective priority queues once they have arrived
		//doesn't run them
		while(usr_list!=NULL && usr_list->arrival_time<=disp_clock){
			it = usr_list;
			usr_list = usr_list->next_process;
			it->next_process = NULL;
			insert_into_usr_queue(it);
		}

		//run user threads for one second based on priority
		if(usrq1_h!=NULL){
			it = usrq1_h;
			usrq1_h = usrq1_h->next_process;
			it->next_process = NULL;
			//handle any return codes if needed
			prepare_process(it, &disp_clock);
		}
		else if(usrq2_h!=NULL){
			it = usrq2_h;
			usrq2_h = usrq2_h->next_process;
			it->next_process = NULL;
			prepare_process(it, &disp_clock);

		}
		else if(usrq3_h!=NULL){
			it = usrq3_h;
			usrq3_h = usrq3_h->next_process;
			it->next_process = NULL;
			prepare_process(it, &disp_clock);
		}
		else
		{	
			printf("- ");
			fflush(stdout);
			disp_clock++;
			sleep(1);
		}
		printf("\n\n");
        }
	printMemory(); //should be empty a this point
}

int prepare_process(process_t* it, int *disp_clock){

	//if we have memory for the process, run it
	//otherwise lower priority and re-submit to queue
	int memSuccess = allocate_memory(it);
	int resourceSuccess = assign_resource(it);
	//printf("resourceSuccess = %d\n", resourceSuccess);

	if(memSuccess && resourceSuccess){ //check to see if memory is free and resources are free
		exec_up(it);
		(*disp_clock)++;
	} else { 	//if memory and ALL resources aren't free, free the resources that have been allocated in the process
		if (memSuccess != 1){
			free_resource(it);  //free allocated resources
			printf("Out of Memory for [%d]\n", it->id);
		}
		if (resourceSuccess == 0){
			remove_from_memory(it->id);
			free_resource(it);  //free allocated resources
			printf("Resources unavailable for [%d]\n", it->id);
		}

		if(it->priority==1 || it->priority == 2){
			it->priority = it->priority+1;
		}
		insert_into_usr_queue(it);
		printf("P[%d] - Lowering priority and re-inserting into queue\n",it->id);
		return 0;
	}
			
	//if finished, free memory and resources and move on	
	if(it->state==terminated){
		remove_from_memory(it->id);
		free(it);
		free_resource(it); 
	}
	else{
		//downgrade priority and move to lower priority queue
		if(it->priority==1 || it->priority == 2){
			it->priority = it->priority+1;
		}
		insert_into_usr_queue(it);
	}
}

int allocate_memory(process_t *it){
	memory_b* mb = NULL;
	int startIndex = 0, endIndex = 0;

	printMemoryList();
	if(check_if_process_is_in_mem(it->id)){
		return TRUE;
	} else {
		//process not in memory, so create a memory block to store information
		//and find a place in memory for this process.
		//If memory cannot be found, return false and let caller deal with the repercussions 
		mb = malloc(sizeof(memory_b));	
		if(it->type == user_proc)
		{
			if(it->priority == 1){
				if(searchForLargeEnoughMemoryBlock(0,it->m_bytes, &startIndex, &endIndex, 1) > 0){
					createMemoryBlock(it, startIndex, endIndex);
					printf("Allocated %d bytes of memory for p[%d] at indexes[%d,%d]:\n", it->m_bytes, it->id, getLowestMemoryIndex(it->id), getLowestMemoryIndex(it->id)+it->m_bytes);
					return TRUE;
				} else {
					//opted with an algorithm that just waits until space is availble
					return FALSE; 
				}
			}else if(it->priority == 2){
				if(searchForLargeEnoughMemoryBlock(0,it->m_bytes, &startIndex, &endIndex, 2) > 0){
					createMemoryBlock(it, startIndex, endIndex);
					printf("Allocated %d bytes of memory for p[%d] at indexes[%d,%d]:\n", it->m_bytes, it->id, getLowestMemoryIndex(it->id), getLowestMemoryIndex(it->id)+it->m_bytes);
					return TRUE;
				} else {
					
					return FALSE; 

				}
			}if(it->priority == 3){
				if(searchForLargeEnoughMemoryBlock(0,it->m_bytes, &startIndex, &endIndex, 3) > 0){
					createMemoryBlock(it, startIndex, endIndex);
					printf("Allocated %d bytes of memory for p[%d] at indexes[%d,%d]:\n", it->m_bytes, it->id, startIndex, endIndex);
					return TRUE;
				} else {
					
					return FALSE;

				}
			}

		}
		else{
			if(searchForLargeEnoughMemoryBlock(0,it->m_bytes, &startIndex, &endIndex, 0) > 0){
					createMemoryBlock(it, startIndex, endIndex);
					printf("Allocated %d bytes of memory for p[%d] at indexes[%d,%d]:\n", it->m_bytes, it->id, getLowestMemoryIndex(it->id), getLowestMemoryIndex(it->id)+it->m_bytes);
					return TRUE;
				} else {
					return FALSE; 

				}


		}
			
	}
}
int searchForLargeEnoughMemoryBlock(int startI, int size, int* start, int* end, int priority){
	int i = 0, s = 0, e = 0;
	int count = 0;
	//ignore first 64 if user process
	if(priority!=0){
		i = 64;
		while(i<HOST_MEM){
			count = 0;
			while(memory[i] == 1) //these are occupied so ignore
				i++;
			s = i;

			while(i<HOST_MEM&&memory[i]==0){
				count++;
				i++;
			}
			if(count>=size){
				e = s+size-1;
				(*start) = s;
				(*end) = e;
				fillMemory(s, e);
				free_mem = free_mem - size;
				return 1;
			}
		}
	}else {
		while(i<=63){
			count = 0;
			while(memory[i] == 1) //these are occupied so ignore
				i++;
			s = i;

			while(i<=63&&memory[i]==0){
				count++;
				i++;
			}
			if(count>=size){
				e = s+size-1;
				(*start) = s;
				(*end) = e;
				fillMemory(s, e);
				free_mem = free_mem - size;
				return 1;
			}
		}
	}
	return 0;

}

int createMemoryBlock(process_t* it, int s, int e){
	memory_b* mb = malloc(sizeof(memory_b));			
	mb->id = it->id;
	mb->m_bytes = it->m_bytes;
	mb->startIndex = s;
	mb->endIndex = e;
	mb->next_block = NULL;
	
	add_to_memory(mb);
}

void add_to_memory(memory_b* mb){
	if(memory_h == NULL){
		memory_h = mb;
		memory_t = mb;
			
	} else {
		memory_t->next_block = mb;
		memory_t = mb;
	}
	memory_t->next_block = NULL;
	fillMemory(mb->startIndex, mb->endIndex);
}
void printMemoryList(){
#if _PRINT_ACTIONS_
	memory_b* it;
	it = memory_h;
	printf("Memory blocks list allocated [%d] free: ", free_mem);
	while(it!=NULL){
		printf("p[%d] %d bytes allocated from [%d,%d]; ", it->id, it->m_bytes, it->startIndex, it->endIndex);
		it = it->next_block;
	}
	printf("\n");
#endif
}

void printProcessList(){        
#if PROC_DEBUG
 	process_t* temp = rt_list;
        int i = 0;
        printf("\nReal_time list: \n");
        while(temp!=NULL){
                printf("[%d] priority %d, proc_time %d, m_bytes %d\n",
                                temp->id, temp->priority, temp->proc_time, temp->m_bytes);
                i++;
                temp = temp->next_process;
        }
        
        temp = usr_list;
        i = 0;
        printf("\nUser Process List: \n");
        while(temp!=NULL){

                printf("[%d] priority %d, proc_time %d, m_bytes %d\n",
                                temp->id, temp->priority, temp->proc_time, temp->m_bytes);
                i++;
                temp = temp->next_process;
                
        }
#endif

}


void remove_from_memory(int id){

	memory_b **source, **tail, *prev, *it;
	source = &memory_h;
	tail = &memory_t;

	it = (*source);
	prev = (*source);

	//for only one block	
	if(id == (*source)->id){
		it = memory_h;
		memory_h = memory_h->next_block;
		zeroMemory(it->startIndex, it->endIndex);
		free_mem = free_mem + it->m_bytes;
		if(_PRINT_ACTIONS_)printf("Process[%d]: Freed %d bytes from memory. [%d] remaining\n",id, it->m_bytes, free_mem);
		free(it);
		return;
	}

	//for several block list
	while(it!=NULL){
		if(id == it->id){
			prev->next_block = it->next_block;
			if(memory_t->id == id){
				//if we remove the last block, we need to reset the tail otherwise
				//insert breaks and we lose memory
				memory_t = prev;
			}
			zeroMemory(it->startIndex, it->endIndex);
			free_mem = free_mem + it->m_bytes;
			if(_PRINT_ACTIONS_)printf("Process[%d]: Freed %d bytes from memory. [%d] remaining\n",id, it->m_bytes, free_mem);
			free(it);
			break;
		} 
		prev = it;
		it = it->next_block;
	}
	

}

int check_if_process_is_in_mem(int id){
	
	memory_b* it;
	it = memory_h;

	if(it == NULL){
		return FALSE;
	}
	
	while(it!=NULL){
		if(id == it->id){
			if(_PRINT_ACTIONS_)printf("Process[%d]-Already in mem\n", id);
			return TRUE;
		}
		it = it->next_block;
	}
	return FALSE;
}


void remove_from_usr_queue(process_t* it){

	if(it->priority == 1){
		usrq1_h=usrq1_h->next_process;
		it->next_process = NULL;
	}
	else if(it->priority == 2){
		usrq2_h=usrq2_h->next_process;
		it->next_process = NULL;
	}
	else if(it->priority == 3){
		usrq3_h=usrq3_h->next_process;
		it->next_process = NULL;
	}
}

void insert_into_usr_queue(process_t* it){

	if(it->priority == 1){
		if(usrq1_h==NULL){
			usrq1_h = it;
			usrq1_t = it;
		}else{
			usrq1_t->next_process = it;
			usrq1_t = it;
		}
		usrq1_t->next_process = NULL;
	}
	else if(it->priority == 2){
		if(usrq2_h==NULL){
			usrq2_h = it;
			usrq2_t = it;
		}else{
			usrq2_t->next_process = it;
			usrq2_t = it;
		}
		usrq2_t->next_process = NULL;
	}
	else if(it->priority == 3){
		if(usrq3_h==NULL){
			usrq3_h = it;
			usrq3_t = it;
		}else{
			usrq3_t->next_process = it;
			usrq3_t = it;
		}
		usrq3_t->next_process = NULL;
	}
}

int getLowestMemoryIndex(int id){
	memory_b* it = memory_h;
	while(it!=NULL){
		if(it->id == id){
			return it->startIndex;
		}
		it = it->next_block;
	}
}

void exec_rt(process_t* it){
	int status, lowIndex=0;
	pid_t pid;
	pid = fork();
	if(!pid)
	{
		lowIndex =  getLowestMemoryIndex(it->id);
		//exec w.e
		printf("\e[0;30mExecuting Real Time Process id[%d].\n Arrival_time[%d], priority[%d], time_remaining[%d].\n Memory_size[%d] allocated from indexes[%d,%d] of main memory: \e[1;0m",it->id,it->arrival_time, it->priority, it->proc_time, it->m_bytes, lowIndex, lowIndex+it->m_bytes );
		while(1)
		{
			printf("\e[0;45m. \e[0;0m");
			fflush(stdout);
			sleep(1);
		}
	}
	if(pid>0)
	{
	  sleep(it->proc_time);
	  kill(pid,SIGINT);
	  waitpid(pid, &status, WUNTRACED);
	}
	printf("\n");
}
void printUserMessage(process_t* it){
	int lowIndex =  getLowestMemoryIndex(it->id);
		if(it->color == 0){
			printf("\e[0;31mExecuting User Process id[%d].\n Arrival_time[%d], priority[%d], time_remaining[%d].\n Memory_size[%d] allocated from indexes[%d,%d] of main memory: \e[1;0m",it->id,it->arrival_time, it->priority, it->proc_time, it->m_bytes, lowIndex, lowIndex+it->m_bytes );
		}else if(it->color ==1){
			printf("\e[0;32mExecuting User Process id[%d].\n Arrival_time[%d], priority[%d], time_remaining[%d].\n Memory_size[%d] allocated from indexes[%d,%d] of main memory: \e[1;0m",it->id,it->arrival_time, it->priority, it->proc_time, it->m_bytes, lowIndex, lowIndex+it->m_bytes );
		}else if(it->color==2){
			printf("\e[0;34mExecuting User Process id[%d].\n Arrival_time[%d], priority[%d], time_remaining[%d].\n Memory_size[%d] allocated from indexes[%d,%d] of main memory: \e[1;0m",it->id,it->arrival_time, it->priority, it->proc_time, it->m_bytes, lowIndex, lowIndex+it->m_bytes );
		}else if(it->color==3){
			printf("\e[0;35mExecuting User Process id[%d].\n Arrival_time[%d], priority[%d], time_remaining[%d].\n Memory_size[%d] allocated from indexes[%d,%d] of main memory: \e[1;0m",it->id,it->arrival_time, it->priority, it->proc_time, it->m_bytes, lowIndex, lowIndex+it->m_bytes );
		}else if(it->color==4){
			printf("\e[0;33mExecuting User Process id[%d].\n Arrival_time[%d], priority[%d], time_remaining[%d].\n Memory_size[%d] allocated from indexes[%d,%d] of main memory: \e[1;0m",it->id,it->arrival_time, it->priority, it->proc_time, it->m_bytes, lowIndex, lowIndex+it->m_bytes );
		}


			fflush(stdout);
}

void exec_up(process_t* it){

	int revive = FALSE, lowIndex = 0;
	pid_t pid;
	int status;
	if(it->pid!=-1){
		pid = it->pid;	
		revive = TRUE;
	}	
	else{
		pid = fork();
		fflush(stdout);

	}
	
	if(!pid)
	{ 
		//exec w.e
		printUserMessage(it);
		
		while(1)
		{
			printf("\e[0;45m. \e[0;0m");
			fflush(stdout);
			usleep(1000900); //this is just a random number greater than 1 second
		}
	}
	if(pid>0)
	{
	
		it->pid = pid;
		if(revive){
			printUserMessage(it);
			kill(pid, SIGCONT);
		}
		sleep(1); 
		if((--(it->proc_time))==0){
			kill(pid,SIGINT);
			waitpid(pid, &status, WUNTRACED);
			it->state = terminated;
			printf("\nProcess[%d]: has terminated ",it->id);
		}else{
			kill(pid, SIGTSTP);
			waitpid(pid, &status, WUNTRACED);
			it->state = waiting;
		}
		fflush(stdout);

	}
	printf("\n");
}
/*****************************************************/
/* Koushik Peri - End of the code that I worked on   */
/*****************************************************/

//Parse in the dispatch file and save into array of proc structs
void parse_input_process_list(char *filename){

        FILE *fp;
        char t[100];
        char *token;
        int i, j, id = 0;
        int array[8];
        process_t *temp;

        fp = fopen(filename, "r");

        if(fp == 0){
                printf("Error: could not open %s\n", filename);
                exit(1);
        }

        i = 0;
        j = 0;

        while (fgets(t, 100, fp) != NULL) {

                if(strcmp(t, "\n") == 0){
                        break;
                }
                token = strtok(t, ", ");
                while(token != NULL) {
                        array[j] = atoi(token);
                        j+=1;
                        token = strtok(NULL, ",");
                }

                
                temp = malloc(sizeof(process_t));

                //populate one process_t struct
                temp->arrival_time = array[0];
                temp->priority = array[1];
                temp->proc_time = array[2];
                temp->m_bytes = array[3];
                temp->printers = array[4];
                temp->scanners = array[5];
                temp->modems = array[6];
                temp->cds = array[7];
                temp->next_process = NULL;
		temp->pid = -1;
		temp->id = id++;
		temp->color = id%5;

                //set the type of process it is
                if((temp->priority==0)){
                        temp->type = real_time;
                } else {
                        temp->type = user_proc;
                }


                //decide which queue to put it in
                //as well as the order to put it in
                dispatch_to_appropriate_queue(temp);

                i+=1;
                j=0;
                
        }
        fclose(fp);        
        

	#if DEBUG
        
        temp = rt_list;
        i = 0;
        printf("\nReal_time list: \n");
        while(temp!=NULL){
                printf("[%d] arrival_time: %d, priority %d, proc_time %d, m_bytes %d, printers %d, scanners %d, modems %d, cds %d\n",
                                temp->id, temp->arrival_time, temp->priority, temp->proc_time, temp->m_bytes, temp->printers, temp->scanners, temp->modems,temp->cds);
                i++;
                temp = temp->next_process;
        }
        
        temp = usr_list;
        i = 0;
        printf("\nUser Process List: \n");
        while(temp!=NULL){

                printf("[%d] arrival_time: %d, priority %d, proc_time %d, m_bytes %d, printers %d, scanners %d, modems %d, cds %d\n",
                                temp->id, temp->arrival_time, temp->priority, temp->proc_time, temp->m_bytes, temp->printers, temp->scanners, temp->modems,temp->cds);
                i++;
                temp = temp->next_process;
                
        }


	#endif
        
}

int assign_resource(process_t* node){
	//returns 0 if unsucessful or returns 1 if successful signalling whether process should be placed back on job queue or not
	
	//check if printers available for node->printers needed
	if (node->printers == 1 ){
		//check if process needs 1 printer
		if(PRINTERS[0].available == 0) {
			//if process needs 1 printer , check if first printer is free, if so, assign to the process
			PRINTERS[0].available = 1;
			PRINTERS[0].takenBy = node->id;
			printf("Printer 0 allocated to [%d]\n", node->id);
		}
		else if (PRINTERS[1].available == 0 ){
			//if first printer is not free, and second printer is free, assign to the process
			PRINTERS[1].available = 1;
			PRINTERS[1].takenBy = node->id;
			printf("Printer 1 allocated to [%d]\n", node->id);
		}	
		else{
			//if no printers are free, return a 0 to place the process back on the job queue
			printf("No printers available for [%d]\n", node->id);
			//PUT BACK ON JOB QUEUE
			return 0;
		}
	}
	if (node->printers == 2) {
		//check if process needs 2 cds, if so and if both cd drives are available, assign to the process
		if (PRINTERS[0].available == 0 && PRINTERS[1].available == 0){
			PRINTERS[0].available = 1;
			PRINTERS[0].takenBy = node->id;
			PRINTERS[1].available = 1;
			PRINTERS[1].takenBy = node->id;
			printf("Printers 0 and 1 are allocated to [%d]\n", node->id);	
		}
		else {
		//otherwise, no printers are allocated to the process
			printf("No printers available for [%d]\n",node->id);
			//PUT BACK ON JOB QUEUE
			return 0;
		}
	}

	//check if scanners available for node->scanners needed
	if (node->scanners == 1){
		//if process needs a scanner, check if scanner is free, if it is, assign it to the process
		if (SCANNER.available == 0 ){
			SCANNER.available = 1;
			SCANNER.takenBy = node->id;
			printf("Scanner allocated to[%d]\n", node->id);
		}
		else{
			//otherwise, return 0 to signal to place the process back on the job queue
			printf("No scanner available for [%d]\n", node->id);
			//PUT BACK ON JOB QUEUE
			return 0;
		}
	}
	
	// check if modems available for node->modems needed
	if (node->modems == 1){	
		//if process needs a modem, check if modem is free, if it is, assign it to the process
		if (MODEM.available == 0 ){
			MODEM.available = 1;
			MODEM.takenBy = node->id; 
			printf("Modem allocated to[%d]\n", node->id);
		}
		else{
			//otherwise, return 0 to signal to place the process back on the job queue
			printf("No modem available for [%d]\n", node->id);
			//PUT BACK ON JOB QUEUE
			return 0;
		}
	}

	//check if CDs available for node->cds needed
	if (node->cds == 1 ){
		//check if process needs 1 cd
		if(CDDRIVE[0].available == 0) {
			//if process needs 1 cd drive, check if first cd drive is free, if so, assign to the process
			CDDRIVE[0].available == 1;
			CDDRIVE[0].takenBy = node->id;
			printf("CD drive 0 allocated to [%d]\n", node->id);
		}
		else if (CDDRIVE[1].available ==0 ){
			//if first cd drive is not free, and second cd drive is free, assign to the process
			CDDRIVE[1].available == 1;
			CDDRIVE[1].takenBy = node->id;
			printf("CD drive 1 allocated to [%d]\n", node->id);
		}	
		else{
			//if no cd drives are free, return a 0 to place the process back on the job queue
			printf("No CD drives available for [%d]\n", node->id);
			//PUT BACK ON JOB QUEUE
			return 0;
		}
	}

	if (node->cds == 2) {
		//check if process needs 2 cds, if so and if both cd drives are available, assign to the process
		if (CDDRIVE[0].available == 0 && CDDRIVE[1].available == 0){
			CDDRIVE[0].available = 1;
			CDDRIVE[1].available = 1;
			CDDRIVE[0].takenBy = node->id;
			CDDRIVE[1].takenBy = node->id;
			printf("CD drives 0 and 1 are allocated to [%d]\n", node->id);	
		}
		else {
		//otherwise, no cd drives are allocated to the process
			printf("No CD drives available for [%d]\n", node->id);
			//PUT BACK ON JOB QUEUE
			return 0;
		}
	}
	
	return 1;
}

void initialize_resources(){
	//initialize printers and scanners availability and takenBy variable to non-possible process_t id
	int num;
	for(num = 0; num < NUM_PRINT_CD ; num++){
		PRINTERS[num].available = 0;
		CDDRIVE[num].available = 0;
		PRINTERS[num].takenBy = -1;
		CDDRIVE[num].takenBy = -1;
	}

	//initialize scanner
	SCANNER.available = 0;
	SCANNER.takenBy = -1;

	//initialize modem
	MODEM.available = 0;
	MODEM.takenBy = -1;
	
	return;
	
}

void free_resource(process_t* node){
	
	//identify if node needed any printers, if so, look through printers to see which is allocated to node
	if (node->printers != 0){		
		if (PRINTERS[0].available == 1 && PRINTERS[0].takenBy == node->id){
			PRINTERS[0].available = 0;
			PRINTERS[0].takenBy = -1;
			if(_PRINT_ACTIONS_)printf("Printer 0 now available.\n");
		}
		if (PRINTERS[1].available == 1 && PRINTERS[1].takenBy == node->id){
			PRINTERS[1].available = 0;
			PRINTERS[1].takenBy = -1;
			if(_PRINT_ACTIONS_)printf("Printer 1 now available.\n");
		}
	}

	//identify if node needed any scanners, if so, look through printers to see which is allocated to node
	if (node->scanners == 1){
		if ( SCANNER.available == 1 && SCANNER.takenBy == node->id){
			SCANNER.available = 0;
			SCANNER.takenBy = -1;
			if(_PRINT_ACTIONS_)printf("Scanner now available.\n");
		}
	}
	
	//identify if node needed any modems, if so, look through printers to see which is allocated to node
	if (node->modems == 1){
		if ( MODEM.available == 1 && MODEM.takenBy == node->id){
			MODEM.available = 0;
			MODEM.takenBy = -1;
			if(_PRINT_ACTIONS_)printf("Modem now available.\n");
		}
	}

	//identify if node needed any cd drives, if so, look through printers to see which is allocated to node
	if (node->cds != 0){		
		if (CDDRIVE[0].available == 1 && CDDRIVE[0].takenBy == node->id){
			CDDRIVE[0].available = 0;
			CDDRIVE[0].takenBy = -1;
			if(_PRINT_ACTIONS_)printf("CD Drive 0 now available.\n");
		}
		if (CDDRIVE[1].available == 1 && CDDRIVE[1].takenBy == node->id){
			CDDRIVE[1].available = 0;
			CDDRIVE[1].takenBy = -1;
			if(_PRINT_ACTIONS_)printf("CD Drive 1 now available.\n");
		}
	}

	return;
}
