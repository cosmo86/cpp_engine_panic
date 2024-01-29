import time
import os
import signal
import sys
import time
from datetime import datetime
import glob

################################################
def termination_handler(signum, frame):
    # Perform your cleanup tasks here
    print("Performing cleanup...")

################################################

current_date = datetime.now()
log_dir = "../Engine_log/"
most_recent_log_path_list  = glob.glob(os.path.join(log_dir, '*.log')).sort(key=os.path.getmtime, reverse=True)

source_log_path = most_recent_log_path_list[0]
last_position = 0

def read_new_log_entries():
    global source_log_path,last_position
    """Read new log entries from the file."""
    with open(source_log_path , 'r') as file:
        file.seek(last_position)
        new_data = file.read()
        new_position = file.tell()
    return new_data, new_position

def transform_log(data):
    """transform log based on their type into different files."""
    # Implement transform logic here
    pass

def start_transform():
    global last_position
    while True:
        # Check for new log entries
        new_entries, last_position = read_new_log_entries()

        if new_entries:
            transform_log(new_entries)

if __name__ == "__main__":

    signal.signal(signal.SIGINT, termination_handler)  # Handle Ctrl+C
    signal.signal(signal.SIGTERM, termination_handler) # Handle kill command

    try:
        start_transform()
        
    except KeyboardInterrupt:
        print("except block")
        pass