import os, argparse, sys, json
import numpy as np
from changepointDetect import *

from thresholds import *
# MIN_THRESHOLD = 0.35
# MINOR_THRESHOLD = 0.1
# MINOR_MAX_THRESHOLD = 0.1
# ADJ_LOCAL_THRESHOLD = 15
# SIGMA = 2.0

parser = argparse.ArgumentParser(description='Change point detection of sign videos')
# Positional arguments
parser.add_argument('json_path', help='Skeleton JSON file path')
parser.add_argument('target_hand', help='auto or right or left hand for analysis')
parser.add_argument('--json', '-j', help='Save changpoints as a JSON file', action='store_true')
parser.add_argument('--log', '-l', help='Log standard out and standard error to a log file', action='store_true')
parser.add_argument('--logdir', '-d', help='Directory path for a log file')
args = vars(parser.parse_args())

json_path = args['json_path']
target_hand = args['target_hand']
log = args['log']
save_json = args['json']

cap_target_hand = target_hand.capitalize()

json_filename = os.path.basename(json_path)
json_filename_prefix = json_filename.split('.json')[0]
#print(json_filename, json_filename_prefix)

if log:
    log_dir = args['logdir']
    log_file_path = os.path.join(log_dir, json_filename_prefix+ f'_{target_hand}_result.txt')
    print(f"Get-Content '{log_file_path}' -Wait")
    sys.stdout = open(log_file_path, 'w')
    sys.stderr = sys.stdout

print('json_path:', json_path)
print('target_hand:', target_hand)

print(f'SIGMA={SIGMA}, MIN_THRESHOLD={MIN_THRESHOLD}, MINOR_THRESHOLD={MINOR_THRESHOLD}, ADJ_LOCAL_THRESHOLD={ADJ_LOCAL_THRESHOLD}, MINOR_MAX_THRESHOLD={MINOR_MAX_THRESHOLD}')

rHandtip, lHandtip = getHandTipJointData(json_path)

strongHandResult = checkStrongHand(rHandtip, lHandtip)
if strongHandResult == 0:
    strongHand = rHandtip
    strongHandStr = 'Right'
else:
    strongHand = lHandtip
    strongHandStr = 'Left'

print('Strong Hand:', strongHandStr)

if target_hand == 'right':
    handTip = rHandtip
elif target_hand == 'left':
    handTip = lHandtip
elif target_hand == 'auto':
    handTip = strongHand

#print('handTip:', handTip)
strong_cp, weak_cp = changepoint_detector(handTip, min_threshold=MIN_THRESHOLD, minor_threshold=MINOR_THRESHOLD, 
    adj_local_threshold=ADJ_LOCAL_THRESHOLD, sigma=SIGMA, minor_max_threshold=MINOR_MAX_THRESHOLD)

print('Strong Changepoints:', strong_cp)
print('Weak Changepoints:', weak_cp)

cp_array = changepoint_detector_1array_confi(handTip, min_threshold=MIN_THRESHOLD, minor_threshold=MINOR_THRESHOLD, 
    adj_local_threshold=ADJ_LOCAL_THRESHOLD, sigma=SIGMA, minor_max_threshold=MINOR_MAX_THRESHOLD)
print('Changepoint Array:', cp_array)

if save_json:
    changepoints_dict = {'StrongChangepoints':strong_cp.tolist(), 'WeakChangepoints':weak_cp.tolist()}
    json_str = json.dumps(changepoints_dict)
    # print('JSON string:', json_str)
    cp_json_path = json_path.replace('.json', 'changepoints.json')
    with open(cp_json_path, "w") as json_file:
        json.dump(changepoints_dict, json_file)