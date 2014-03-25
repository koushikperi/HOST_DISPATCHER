
/*MACROS*/
#define DEBUG 1
#define MEM_DEBUG 1
#define PROC_DEBUG 1
#define _PRINT_MEMORY_ 0
#define _PRINT_ACTIONS_ 0
#define FALSE 0
#define TRUE 1
#define HOST_MEM 1024
#define NUM_PRINT_CD 2


typedef enum {
	real_time,
	user_proc
} process_type;

typedef enum {
	init,
	ready, 
	waiting, 
	terminated
} process_lifecycle;

typedef struct process_t process_t;
typedef struct resource resource;

struct resource{
	int available; //0 if available, 1 if taken
	int takenBy;  //process id that is using the resource - has a value of -1 if not taken by anyone
};

struct process_t{
	int arrival_time;
	int priority;
	int proc_time;
	int m_bytes;
	int printers;
	int scanners;
	int modems;
	int cds;
	process_type type;
	process_lifecycle state;
	process_t* next_process;
	pid_t pid;
	int id;
	int color;
};


typedef struct memory_b memory_b;

struct memory_b{
	memory_b* next_block;
	int id;
	int m_bytes;
	int startIndex;
	int endIndex;
};

/*GLOBAL VARIABLES*/

/*Mutlilevel dispatcher queue initialization*/
process_t* rt_list = NULL; 
process_t* usr_list = NULL; 
process_t* usrq1_h = NULL;
process_t* usrq1_t= NULL; 
process_t* usrq2_h= NULL; 
process_t* usrq2_t= NULL; 
process_t* usrq3_h=NULL;
process_t* usrq3_t=NULL;
memory_b* memory_h = NULL; //linked list of the processes in memory
memory_b* memory_t = NULL;
int memory[HOST_MEM];
int free_mem = HOST_MEM - 64;



resource PRINTERS[2];
resource SCANNER;
resource MODEM;
resource CDDRIVE[2];

/*FUNCTION PROTOTYPES*/
void parse_input_process_list(char *filename);
void dispatch_to_appropriate_queue();
void dispatcher();
void exec_rt(process_t* it);
void exec_up(process_t* it);
void insert_into_usr_queue(process_t* it);
void remove_from_usr_queue(process_t* it);
int allocate_memory(process_t *it);
void add_to_memory(memory_b* it);
void remove_from_memory(int id);
int check_if_process_is_in_mem(int id);
int createMemoryBlock(process_t* it, int startIndex, int endIndex);
int assign_resource(process_t *it);
void free_resource(process_t* it);
void initialize_resources();
int searchForLargeEnoughMemoryBlock(int startI, int size, int* s, int* e, int priority);
void zeroMemory(int s, int e);
void fillMemory(int s, int e);
void printMemory();
int getLowestMemoryIndex(int id);
void printMemoryList();
void printProcessList();
void printUserMessage(process_t* it);
