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
  u32 remaining_time;  // Add this line

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 waiting_time;      // Total time spent waiting in the queue
  u32 response_time;     // Time from arrival to first execution
  u32 has_started;       // Flag to check if the process has started executing
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

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <process file> <quantum>\n", argv[0]);
    return EINVAL;
  }

  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);
  u32 quantum = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  for (u32 i = 0; i < size; i++)
  {
    TAILQ_INSERT_TAIL(&list, &data[i], pointers);
  }

  u32 current_time = 0;
  u32 total_waiting_time = 0;
  u32 total_response_time = 0;
  u32 process_count = 0;

  // Main scheduling loop
  while (!TAILQ_EMPTY(&list))
  {
    struct process *current_process = TAILQ_FIRST(&list);
    TAILQ_REMOVE(&list, current_process, pointers);

    if (!current_process->has_started)
    {
      current_process->has_started = 1;
      current_process->response_time = current_time - current_process->arrival_time;
    }

    u32 time_slice = (current_process->remaining_time > quantum) ? quantum : current_process->remaining_time;
    current_process->remaining_time -= time_slice;
    current_time += time_slice;

    struct process *proc;
    TAILQ_FOREACH(proc, &list, pointers) {
        proc->waiting_time += time_slice;
    }

    if (current_process->remaining_time > 0)
    {
      TAILQ_INSERT_TAIL(&list, current_process, pointers);
    }
    else
    {
      total_waiting_time += current_process->waiting_time;
      total_response_time += current_process->response_time;
      process_count++;
    }
  }

  if (process_count > 0)
  {
    printf("Average waiting time: %.2f\n", (double)total_waiting_time / process_count);
    printf("Average response time: %.2f\n", (double)total_response_time / process_count);
  }

  free(data);
  return 0;
}
