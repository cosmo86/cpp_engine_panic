import time
import os
import signal
import sys
import time
from datetime import datetime
import glob
import json
import warnings

# Issue a warning message
print('#########################################')
print('Run this moduel in folder /py_log_handler')
print('Do Not terminate this program, leave it running')
print('#########################################')

######################
current_date = datetime.now()
current_date_str = current_date.strftime("%Y%m%d")
logTransformer_record_path = os.path.join("records", f"logTransformer_record_{current_date_str}.json")
last_position = 0

################################################
shutdown_requested = False
def termination_handler(signum, frame):
    global shutdown_requested
    print("Shutdown signal received...")
    shutdown_requested = True

def perform_cleanup():
    global logTransformer_record_path, last_position
    print("Performing cleanup...")
    my_dict = {'termination_points': []}
    if os.path.exists(logTransformer_record_path):
        with open(logTransformer_record_path, 'r') as json_file:  # Open file in read mode
            my_dict = json.load(json_file)
    else:
        os.mkdir("records")
    my_dict['termination_points'].append(last_position)
    
    with open(logTransformer_record_path, 'w') as json_file:  # Open file in write mode
        json.dump(my_dict, json_file) 

################################################

log_dir = "../Engine_log/"
#log_dir = "../server_engine_log/"
most_recent_log_files = sorted(glob.glob(os.path.join(log_dir, '*.log')), key=os.path.getmtime, reverse=True)
if most_recent_log_files:
    source_log_path = most_recent_log_files[0]
    print(f'Processing {source_log_path}')
else:
    print("No log files found.")
    sys.exit(1)

######################################
processed_log_folder_path = "../py_processed_log"
current_date_folder_path = os.path.join(processed_log_folder_path, current_date_str)

# Ensure both the base processed log folder and the current date folder exist
if not os.path.exists(current_date_folder_path):
    os.makedirs(current_date_folder_path)
    print(f"Folder '{current_date_folder_path}' was created.")
else:
    print(f"Folder '{current_date_folder_path}' already exists.")

log_porcess_state = {"Trader": None, "L2_Quoter": None}

###############################################

def read_new_log_entries():
    temp_lines = []
    global source_log_path, last_position, log_porcess_state
    with open(source_log_path, 'r', encoding='utf-8') as raw_log_file:
        raw_log_file.seek(last_position)
        for line in raw_log_file:
            temp_line = line.strip().split(',')
            if len(temp_line) > 1:  # Check if the line has at least 2 elements
                process_log_line(temp_line)
            else:
                pass
        new_position = raw_log_file.tell()
    return temp_lines, new_position

def process_log_line(temp_line):
    global log_porcess_state, current_date_folder_path
    if temp_line[1] == "T":
        target_log = "Trader"
    elif temp_line[1] == "Q":
        target_log = "L2_Quoter"
    elif temp_line[1] == "S":
        target_log = f"S{temp_line[2]}"
    else:
        return  # If the line doesn't match the expected patterns, skip it
    
    if target_log not in log_porcess_state or log_porcess_state[target_log] is None:
        log_porcess_state[target_log] = f"{target_log}_{source_log_path[-19:-4]}.log"
    
    with open(os.path.join(current_date_folder_path, log_porcess_state[target_log]), 'a', encoding='utf-8') as file:
        file.write(','.join(temp_line) + '\n')

if __name__ == "__main__":
    signal.signal(signal.SIGINT, termination_handler)  # Handle Ctrl+C
    signal.signal(signal.SIGTERM, termination_handler)  # Handle kill command

    while True:
        if shutdown_requested:
            perform_cleanup()
            break  # Exit the loop to terminate the program
        
        temp_lines, new_position = read_new_log_entries()
        if new_position != last_position:
            last_position = new_position
            # Process the lines if needed, already done in read_new_log_entries
        time.sleep(0.01)  # Wait a bit before checking for new entries again