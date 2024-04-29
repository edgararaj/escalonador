#!/bin/bash

# Specify the path to your text file
file_path="$1"

# Read each line from the file and extract the time in seconds
total_time=0
line_count=0

while IFS= read -r line; do
    time_seconds=$(echo "$line" | awk '{print $NF}' | tr -d '()' | cut -d's' -f1)
    total_time=$(echo "$total_time + $time_seconds" | bc)
    line_count=$((line_count + 1))
done < "$file_path"

# Calculate the mean time
mean_time=$(echo "scale=3; $total_time / $line_count" | bc)

echo "Tempo médio de espera: $mean_time seconds"
