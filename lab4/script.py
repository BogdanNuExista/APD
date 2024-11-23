import matplotlib.pyplot as plt

# Data for the performance counts
programs = ['performancecount1', 'performancecount2', 'performancecount3', 'performancecount4']
time_parallel = [0.195539, 0.011052, 0.011645, 0.005701]
time_serial = [0.019484, 0.019520, 0.019743, 0.020019]

# Plotting the data
plt.figure(figsize=(10, 5))
plt.plot(programs, time_parallel, marker='o', label='Parallel Time', color='b')
plt.plot(programs, time_serial, marker='o', label='Serial Time', color='r')

# Adding labels and title
plt.xlabel('Programs')
plt.ylabel('Time (seconds)')
plt.title('Execution Time: Parallel vs. Serial')
plt.legend()
plt.grid(True)

# Show the plot
plt.tight_layout()
plt.show()
