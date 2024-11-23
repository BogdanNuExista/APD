How to run:
- Be in the sports_analyzer folder and check if in the data folder you have 3 other folders (football, basketball, tennis)
  in which you unzipped every zip file.
- Run : chmod +x build.sh
- Run : ./build.sh
- To view the graphs, run python3 script.py (make sure you have python installed and matplotlib)
- If you want to run the serial version, change dir to serial and then run chmod +x build.sh and ./build.sh



Workflow:

P.S. I am using almost the same input data for football and tennis because I couldn't find football
     data anywhere. Data data differes by a few extra files so that the outputs are different.

- Initialization:
  The main function initializes the shared buffer and profiler.
  Producer and consumer threads are created.

- Producer Thread:
Phase 1: Football Processing:
  Reads football player data from a CSV file and adds it to the shared buffer.
  Searches for additional football CSV files and processes them.
  Signals the end of football data processing.
  Waits for consumers to finish processing football data.
  Generates a report for football data.
Phase 2: Tennis Processing:
  Reads tennis player data from a CSV file and adds it to the shared buffer.
  Searches for additional tennis CSV files and processes them.
  Signals the end of tennis data processing.
  Waits for consumers to finish processing tennis data.
  Generates a report for tennis data.
  Signals the completion of all data processing.

- Consumer Threads:
Each consumer thread processes data from the shared buffer according to the current phase (football or tennis).
Football Phase:
  Consumer 0 calculates PPA for football players.
  Consumer 1 calculates max points for football players.
Tennis Phase:
  Consumer 0 calculates PPA for tennis players.
  Consumer 1 calculates max points for tennis players.
Consumers wait for new data or phase changes and signal completion when done.

- Profiling Thread:
Periodically samples and logs CPU and memory usage.
Tracks and logs hotspots (sections of code that are frequently executed or take a long time).

- Synchronization:
Mutexes and condition variables are used to synchronize access to the shared buffer and coordinate phase changes.
The producer and consumers use condition variables to wait for data availability and phase changes.



Conclusions: 
- Bottlenecks: 
This is a sample from the serial version:
CPU Usage: 99.90%
Memory Usage: 2596996 KB
Wall Clock Time: 196.896879 seconds
Hotspots:
  football_ppa_calculation: Total Time=10.033165, Calls=186780, Avg Time=0.000054
  football_points_calculation: Total Time=86.821916, Calls=3235639, Avg Time=0.000027
  tennis_ppa_calculation: Total Time=10.151809, Calls=186782, Avg Time=0.000054
  tennis_points_calculation: Total Time=86.496379, Calls=3235639, Avg Time=0.000027

This is a sample from parallel version:
CPU Usage: 1.70%
Memory Usage: 2600752 KB
Wall Clock Time: 350.005301 seconds
Hotspots:
  football_phase: Total Time=169.773106, Calls=1, Avg Time=169.773106
  csv_file_processing: Total Time=344.359332, Calls=334, Avg Time=1.031016
  football_ppa_calculation: Total Time=13.986267, Calls=186780, Avg Time=0.000075
  football_points_calculation: Total Time=131.485787, Calls=3235639, Avg Time=0.000041
  tennis_phase: Total Time=175.501626, Calls=1, Avg Time=175.501626
  tennis_ppa_calculation: Total Time=14.633180, Calls=186782, Avg Time=0.000078
  tennis_points_calculation: Total Time=136.887035, Calls=3235639, Avg Time=0.000042

Problems:
- As we can see, the programs, especially the parallel one spends a lot of time calculating max points.
This problem is due to Synchronization Overhead because the use of mutexes to protect shared data can introduce significant overhead, especially if the critical sections are long or if there is high contention among threads
- I should have done the program in C++ and maybe have used a hashmap and that way, inside the critical sections,
the program would've runed faster and therefore I wouldn't got this much overhead.
- The functions that calculate max ppa and max points are extremely similar, I used different functions
  for different sports in case the csv files and data differ.


- Speedup:
Tparalel = 350.005301 seconds, Tserial = 196.896879 seconds
Speedup = Tseral/Tparalel = 196.896879/350.005301 = 0.56
Efficiency = Speedup / p = 0.56 / 3 = 0.18 
                                  |
                            2 cons and 1 prov

