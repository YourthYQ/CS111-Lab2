# You Spin Me Round Robin

A simple C program that can schedule processes sharing single-core CPU with Round-Robin as its virtualization mechanism.

With such mechanism, total run time of each process will be divided into a lot of tiny slots (scheduling quantum) and then CPU will switch to the next job in the run queue in a determined order.

## Building

To build the program, navigate to the directory containing the source code and run the following command in your terminal: 

```shell
make
```

This will compile the code using the Makefile provided in the directory.

## Running

To execute the program, run the following command in your terminal: 
```shell
./rr <input_file> <quantum_length>
```

## Results

### Example 1:

With the input command `./rr processes.txt 3`, the following result will be printed:

```shell
Average waiting time: 7.00
Average response time: 2.75
```

### Example 2:

With the input command `./rr processes.txt 0`, the following result will be printed:

```shell
Average waiting time: 0.00
Average response time: 0.00
```

## Cleaning up

Run the following command to clean up your working directory:

```shell
make clean
```

This will remove any compiled binaries and other temporary files created during the build process.

## Testing

```python
python -m unittest
```

### Expected Results

The test cases should pass without any errors, indicating the program is correctly running. After testing, the keyword **OK** will be printed which represents the success.