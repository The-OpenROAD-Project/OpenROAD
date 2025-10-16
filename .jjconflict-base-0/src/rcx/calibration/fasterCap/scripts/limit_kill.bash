#!/bin/bash

if [ $# -lt 3 ]
then
        echo "Usage <job_id> <time_limit_secs> <check_interval> "
        exit
fi

script_dir="$(dirname "$0")"
# echo "$script_dir = script_dir"

# sleep 1000 &  # Start a long-running background job (example command)
# job_pid=$!     # Get the PID of the background job

# Define the time limit in seconds (e.g., 10 seconds)
job_pid=$1
time_limit=$2
check_interval=$3

# Start time in seconds
start_time=$(date +%s)

# Monitor loop
while kill -0 "$job_pid" 2>/dev/null; do
    # Calculate elapsed time
    current_time=$(date +%s)
    elapsed_time=$((current_time - start_time))

    if ((elapsed_time > time_limit)); then
        echo "             $time_limit Second Limit exceeded. Killed pid $job_pid."
        kill -9 "$job_pid"
        break
    fi

    # Sleep for a short period before checking again (e.g., 1 second)
    sleep $check_interval
done

