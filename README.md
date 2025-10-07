<h1>Parallel Computing - CSC410 @ Dakota State University</h1>

- A1 - Starting with Sequential
- - An assignment where we wrote sequential programs, sum of arrays, the Queens game, bubble and merge sort algorithms, and numerical integration. Wrote a bash script to time all programs at once. Will parallelize in the next assigment.
- A2 - Part 1: A Star Wars Process and Parallel Sums
- - An assignment focuses on practicing process creation and parallel programming in C. You will complete a program that creates four child processes to adjust a shared shield power, demonstrating how processes have separate memory. Then, you will convert a sequential array summation program into a parallel version using multiple processes and pipes for communication. Finally, you will time both sequential and parallel programs and analyze the performance benefits and challenges of multiprocessing. This assignment helps build understanding of process management, inter-process communication, and parallel computation.
- A2 - Part 2: 
- - This assignment implements parallel versions of the array sum and matrix multiplication programs from Assignment 1 using pthreads, where the array sum work is divided equally among threads and matrix multiplication threads divide work by matrix rows. Results demonstrated that parallelization overhead can outweigh benefits for simple tasks like array summation, while heavier tasks like matrix multiplication showed limited speedup but never outperformed sequential execution due to overhead and memory bottlenecks.
