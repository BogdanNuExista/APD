import matplotlib.pyplot as plt

# Initialize lists to store data
samples = []
cpu_usage = []
memory_usage = []
wall_clock_time = []

# Read the performance log file
with open('performance_log.txt', 'r') as file:
    lines = file.readlines()

# Parse the file and extract data
for line in lines:
    if line.startswith('Sample'):
        sample_number = int(line.split()[1][:-1])
        samples.append(sample_number)
    elif line.startswith('CPU Usage'):
        cpu_usage.append(float(line.split()[2][:-1]))
    elif line.startswith('Memory Usage'):
        memory_usage.append(int(line.split()[2]))
    elif line.startswith('Wall Clock Time'):
        wall_clock_time.append(float(line.split()[3]))

# Plot CPU Usage
plt.figure(figsize=(10, 6))
plt.plot(samples, cpu_usage, label='CPU Usage (%)', marker='o')
plt.xlabel('Sample')
plt.ylabel('CPU Usage (%)')
plt.title('CPU Usage Over Samples')
plt.legend()
plt.grid(True)
plt.show()

# Plot Memory Usage
plt.figure(figsize=(10, 6))
plt.plot(samples, memory_usage, label='Memory Usage (KB)', marker='o', color='orange')
plt.xlabel('Sample')
plt.ylabel('Memory Usage (KB)')
plt.title('Memory Usage Over Samples')
plt.legend()
plt.grid(True)
plt.show()

# Plot Wall Clock Time
plt.figure(figsize=(10, 6))
plt.plot(samples, wall_clock_time, label='Wall Clock Time (seconds)', marker='o', color='green')
plt.xlabel('Sample')
plt.ylabel('Wall Clock Time (seconds)')
plt.title('Wall Clock Time Over Samples')
plt.legend()
plt.grid(True)
plt.show()