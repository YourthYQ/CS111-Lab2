#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  bool isDone; // will be initialized to false later
  bool isStarted; // will be initialized to false later
  bool isInserted; // will be initialized to false later
  u32 remaining_time;
  u32 end_time;
    
  u32 waiting_time;
  u32 response_time;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

// Used to qsort the Processes' List by arrival_time
int compare_by_arrival_time(const void *a, const void *b) {
    struct process *processA = (struct process *)a;
    struct process *processB = (struct process *)b;
    return processA->arrival_time - processB->arrival_time;
}

int main(int argc, char *argv[]) {
    
    // If the quantum length is 0, then all average_time is 0 and return directly
    if (atoi(argv[2]) == 0) {
        u32 total_waiting_time = 0;
        u32 total_response_time = 0;
        
        printf("Average waiting time: %.2f\n", (float)total_waiting_time);
        printf("Average response time: %.2f\n", (float)total_response_time);
        
        return 0;
    }
    
    
    if (argc != 3) {
      return EINVAL;
    }
    struct process *data;
    u32 size;
    init_processes(argv[1], &data, &size);

    u32 quantum_length = next_int_from_c_str(argv[2]);

    struct process_list list;
    TAILQ_INIT(&list);

    u32 total_waiting_time = 0;
    u32 total_response_time = 0;
    
    
    /* Your code here */
    
    // Sorting the data array by arrival time
    qsort(data, size, sizeof(struct process), compare_by_arrival_time);
    
    /* Debug: sorting
    printf("Sorted Process List:\n");
    printf("Process ID | Arrival Time | Burst Time\n");
    for (int i = 0; i < size; i++) {
        printf("%10d | %12d | %10d\n", data[i].pid, data[i].arrival_time, data[i].burst_time);
    }
     */

    // Insert the first process into the Tail_Queue
    TAILQ_INSERT_TAIL(&list, &data[0], pointers);

    // Log the current time
    u32 current_time = 0;

    // Initializes all process
    for (int i = 0; i < size; i++) {
        data[i].isDone = false;
        data[i].isStarted = false;
        data[i].remaining_time = data[i].burst_time;
        data[i].end_time = 0;
        
        if (i == 0) {
            data[i].isInserted = true;
        } else {
            data[i].isInserted = false;
        }
    }

    // Main logic starts here
    bool isFinish = false;
    while(!isFinish) {

        // Extract the first pointer in the list as `current_process`
        struct process *current_process = TAILQ_FIRST(&list);
        
        // Check if the current_process is started (whether first time executing)
        if (!current_process->isStarted) {
            current_process->isStarted = true;
            current_process->response_time = current_time - current_process->arrival_time;
            total_response_time += current_process->response_time;
        }
        
        // Compare the value of `remaining_time of current_process` and `quantum_length`
        if (current_process->remaining_time >= quantum_length) {
                        
            // Find the min(current_process->remaining_time, quantum_length)
            u32 insert_time = 0;
            if (current_process->remaining_time >= quantum_length) {
                insert_time = quantum_length;
            } else {
                insert_time = current_process->remaining_time;
            }
            
            for (int i = 0; i < insert_time; i++) {
                current_time++;
                for (int j = 0; j < size; j++) {
                    if (!data[j].isInserted && data[j].arrival_time == current_time) {
                        TAILQ_INSERT_TAIL(&list, &data[j], pointers);
                        data[j].isInserted = true;
                        break;
                    }
                }
            }
            
        } else {
            
            for (int i = 0; i < current_process->remaining_time; i++) {
                for (int j = 0; j < size; j++) {
                    if (!data[j].isInserted && data[j].arrival_time == current_time) {
                        TAILQ_INSERT_TAIL(&list, &data[j], pointers);
                        data[j].isInserted = true;
                        break;
                    }
                }
                current_time++;
            }
                        
        }
        
        // Check if the current_process is done
        unsigned int difference = current_process->remaining_time - quantum_length;
        if (difference == 0 || difference > current_process->remaining_time) {
            current_process->remaining_time = 0;
            current_process->isDone = true;
            
            // While current_process is done, then calculate its waiting time
            current_process->end_time = current_time;
            current_process->waiting_time = current_process->end_time - current_process->arrival_time - current_process->burst_time;
            total_waiting_time += current_process->waiting_time;
        } else {
            current_process->remaining_time = difference;
        }

        // Remove the current_process in the list
        TAILQ_REMOVE(&list, current_process, pointers);

        // Used to avoid double insert the same process
        bool isInTheStack = false;
        // Check if all process is done
        for (int i = 0; i < size; i++) {
            if (data[i].isDone) {
                isFinish = true;
            } else {
                isFinish = false;
                
                // Re-insert the `current_process` if no additional process in the list
                if (TAILQ_EMPTY(&list)) {
                    TAILQ_INSERT_TAIL(&list, current_process, pointers);
                    isInTheStack = true;
                }
                break;
            }
        }
        
        if (!isInTheStack && !current_process->isDone) {
            TAILQ_INSERT_TAIL(&list, current_process, pointers);
        }

    }
    
    /* End of "Your code here" */

    printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
    printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

    free(data);
    return 0;
}
