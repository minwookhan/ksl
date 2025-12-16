import signal, os, argparse, sys
#import cv2
import numpy as np
import vpython as vp
# import ruptures as rpt
import scipy.ndimage
from vpython.no_notebook import stop_server
from changepointDetect import *
# from matplotlib import pyplot as plt

# CHANGE_DISPLAY = False
sphere_radius = 10
head_radius = 50
RATE = 10
STOP = False

from thresholds import *
# MIN_THRESHOLD = 0.35
# MINOR_THRESHOLD = 0.05
# ADJ_LOCAL_THRESHOLD = 15
# SIGMA = 2.0

def handler(signum, frame):
    global STOP
    print("Signum:", signum)
    STOP = True

def makeTrajectory(joint, radius, seg_info):
    color_list = [vp.color.red, vp.color.orange, vp.color.yellow, vp.color.green, vp.color.blue]
    color_idx = 0
    # right handtip trajectory
    trajectory = []
    segments = [ [] ]
    for i in range(frame_len): 
        if i == 0:
            prepos = vp.vector(joint[i][0], joint[i][1], joint[i][2])
        else:
            curpos = vp.vector(joint[i][0], joint[i][1], joint[i][2])
            caxis = curpos - prepos

            if i in seg_info:
                color_idx +=1
                color_idx = 0 if color_idx >= len(color_list) else color_idx
                segments.append([])

            cyl = vp.cylinder(pos=prepos, axis=caxis, radius=radius, color=color_list[color_idx], opacity=0)
            trajectory.append(cyl)
            segments[-1].append(cyl)
            prepos = curpos

    return trajectory, segments

def highlight_cur_seg(seg_idx, segments):
    if (seg_idx >= 0) and (seg_idx < len(segments)):
        for cyl in segments[seg_idx]:
            cyl.opacity=0.7
    
    for i in (seg_idx-1, seg_idx+1):
        if (i >= 0) and (i < len(segments)):
            for cyl in segments[i]:
                cyl.opacity=0.3
    if seg_idx-2 >= 0:
        for cyl in segments[seg_idx-2]:
            cyl.opacity=0.0

def make_transparent_cur_seg(seg_idx, segments):
    if (seg_idx >= 0) and (seg_idx < len(segments)):
        for cyl in segments[seg_idx]:
            cyl.opacity=0.0

signal.signal(signal.SIGINT, handler)

# 1st data json 파일을 열고 dict type의 data에 로드
parser = argparse.ArgumentParser(description='Visualization of temporal segmentation')
# Positional arguments
parser.add_argument('json_path', help='Skeleton JSON file path')
parser.add_argument('target_hand', help='both, right, left hand for analysis')
parser.add_argument('--cap', '-c', help='Capture screenshots', action='store_true')
parser.add_argument('--gau', '-g', help='Apply Gaussian filter to the strong handtip', action='store_true')
parser.add_argument('--log', '-l', help='Log standard out and standard error to a log file', action='store_true')
parser.add_argument('--logdir', '-d', help='Directory path for a log file')
args = vars(parser.parse_args())

json_path = args['json_path']
target_hand = args['target_hand']
log = args['log']
cap = args['cap']
gau = args['gau']

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


Head, SpineChest, rHandtip, rHand, rWrist, rElbow, rShoulder, rClavicle, lHandtip, lHand, lWrist, lElbow, lShoulder, lClavicle = getJointData(json_path)
frame_len = len(Head)
print('Number of frames:', frame_len)

rHandtip_org = rHandtip
lHandtip_org = lHandtip

if gau:
    if target_hand == 'both' or target_hand == 'right':
        rHandtip_gfiltered = scipy.ndimage.gaussian_filter1d(rHandtip, SIGMA, axis=0)
        rHandtip = rHandtip_gfiltered
    elif target_hand == 'left':
        lHandtip_gfiltered = scipy.ndimage.gaussian_filter1d(lHandtip, SIGMA, axis=0)
        lHandtip = lHandtip_gfiltered


#vbox = vp.box()
balls = []
rJoint_list = [rHandtip, rHand, rWrist, rElbow, rShoulder, rClavicle]
rRods = None
for ji, joint in enumerate(rJoint_list):
    balls.append(vp.sphere(pos=vp.vector(joint[0][0], joint[0][1], joint[0][2]), radius=sphere_radius, color=vp.color.green))
    if ji==0:
        rRods = vp.curve(pos=balls[-1].pos, radius=sphere_radius/2, color=vp.color.yellow)
    else:
        rRods.append(pos=[vp.vector(joint[0][0], joint[0][1], joint[0][2])])

lJoint_list = [lHandtip, lHand, lWrist, lElbow, lShoulder, lClavicle]
lRods = None
for ji, joint in enumerate(lJoint_list):
    balls.append(vp.sphere(pos=vp.vector(joint[0][0], joint[0][1], joint[0][2]), radius=sphere_radius, color=vp.color.blue))
    if ji==0:
        lRods = vp.curve(pos=balls[-1].pos, radius=sphere_radius/2, color=vp.color.orange)
    else:
        lRods.append(pos=[vp.vector(joint[0][0], joint[0][1], joint[0][2])])

headball = vp.sphere(pos=vp.vector(Head[0][0], Head[0][1], Head[0][2]), radius=head_radius, color=vp.color.white)

vp.scene.width = 1000
vp.scene.height = 500
#vp.scene.background = vp.color.white
vp.scene.center = vp.vector(SpineChest[0][0], SpineChest[0][1], SpineChest[0][2])
vp.scene.up = vp.vector(Head[0][0], Head[0][1], Head[0][2]) - vp.scene.center

SpineChest2rShoulder = vp.vector(rShoulder[0][0], rShoulder[0][1], rShoulder[0][2]) - vp.scene.center

sceneForwVec = vp.cross(SpineChest2rShoulder, vp.scene.up)

vp.scene.forward = sceneForwVec
vp.scene.fov = vp.scene.fov*1.2/3

#dlight = vp.distant_light(direction=sceneForwVec, color=vp.color.gray(2))
for dl in vp.scene.lights:
    #print(dl.direction, dl.color)
    dl.direction = -dl.direction
    #print(dl.direction, dl.color)

gd = vp.graph( width=1000, height=400,
      title='<b>Velocity Graph</b>',
      xtitle='<i>Frame #</i>', ytitle='<i>Velocity</i>', fast=False)

anggd = vp.graph( width=1000, height=400,
      title='<b>Angular Acceleration Graph</b>',
      xtitle='<i>Frame #</i>', ytitle='<i>Angle</i>', fast=False)

rHandtipDisp = np.array(calculateDisplacement(rHandtip))
lHandtipDisp = np.array(calculateDisplacement(lHandtip))

rHandtipDispVector = calculateDisplacementVector(rHandtip)
lHandtipDispVector = calculateDisplacementVector(lHandtip)

# totalDisp = rWristDisp + rHandtipDisp + lWristDisp + lHandtipDisp
if target_hand == 'both':
    handtipDisp = rHandtipDisp + lHandtipDisp
    handTip = rHandtip_org
    handtipDispVector = rHandtipDispVector
elif target_hand == 'right':
    handtipDisp = rHandtipDisp
    handTip = rHandtip_org
    handtipDispVector = rHandtipDispVector
elif target_hand == 'left':
    handtipDisp = lHandtipDisp
    handTip = lHandtip_org
    handtipDispVector = lHandtipDispVector

handtipDispGaussFiltered = scipy.ndimage.gaussian_filter1d(handtipDisp, SIGMA)

angAcc = calculateAngularAcc(handtipDispVector)
angAcc_org = angAcc
angAcc = scipy.ndimage.gaussian_filter1d(angAcc_org, SIGMA)

print('For Handtip Displacement:')

strong_cp, weak_cp = changepoint_detector(handTip, min_threshold=MIN_THRESHOLD, minor_threshold=MINOR_THRESHOLD, 
    adj_local_threshold=ADJ_LOCAL_THRESHOLD, sigma=SIGMA, minor_max_threshold=MINOR_MAX_THRESHOLD)

print('Strong Changepoints:', strong_cp)
print('Weak Changepoints:', weak_cp)

sys.stdout.flush()

fhandtipDispFiltered = vp.gcurve(graph = gd, color=vp.color.blue, label=f"{cap_target_hand} Handtip Displacement Gaussian Filtered")
angAccGraph = vp.gcurve(graph = anggd, color=vp.color.blue, label=f"{cap_target_hand} Handtip Angular Acceleration")

fdhandtipDispFilteredMin = vp.gdots(graph = gd, color=vp.color.red)

for i in range(len(handtipDisp)):
    fhandtipDispFiltered.plot(i, handtipDispGaussFiltered[i])
    angAccGraph.plot(i, angAcc[i])

fdStrongCP_AngAcc = vp.gdots(graph = anggd, color=vp.color.red)

for x in strong_cp:
    fdhandtipDispFilteredMin.plot(x, handtipDispGaussFiltered[x])
    fdStrongCP_AngAcc.plot(x, angAcc[x])

fdweakhandtipDispFilteredMin = vp.gdots(graph = gd, color=vp.color.green)
fdWeakCP_AngAcc = vp.gdots(graph = anggd, color=vp.color.green)
for x in weak_cp:
    fdweakhandtipDispFilteredMin.plot(x, handtipDispGaussFiltered[x])
    fdWeakCP_AngAcc.plot(x, angAcc[x])

# for x in wristDispGaFtedLocalMin_filtered:
#     fdwristDispFilteredMin.plot(x, wristDispGaussFiltered[x])


fdhandtipDispFiltered = vp.gdots(graph = gd, color=vp.color.blue)
fdAngAccGraph = vp.gdots(graph = anggd, color=vp.color.blue)


# right handtip trajectory
rHandtipTraj, rHandtipTrajSegments = makeTrajectory(rHandtip, sphere_radius/3, strong_cp)
# left handtip trajectory
lHandtipTraj, lHandtipTrajSegments = makeTrajectory(lHandtip, sphere_radius/3, strong_cp)

# for seg in rHandtipTrajSegments:
#     print(len(seg))

# fdrWristDisp = vp.gdots(graph = gd, color=vp.color.blue)
# fdlWristDisp = vp.gdots(graph = gd, color=vp.color.orange)

# fdrElbow = vp.gdots(graph = gd, color=vp.color.blue)
# fdrShoulder = vp.gdots(graph = gd, color=vp.color.orange)
# fdlElbow = vp.gdots(graph = gd, color=vp.color.green)
# fdlShoulder = vp.gdots(graph = gd, color=vp.color.red)

saved_frame_set = set()
idx = 1
seg_idx = 0 # segment index
highlight_cur_seg(seg_idx, rHandtipTrajSegments)
# highlight_cur_seg(seg_idx, lHandtipTrajSegments)
while(True):
    #print(idx)
    if idx < strong_cp[0]-15 or idx > strong_cp[-1]+5:
        vp.rate(RATE*3)
    else:
        vp.rate(RATE)
    # ret, frame = vcap.read()
    # if ret:
    #     cv2.imshow('Frame',frame)

    for ji, joint in enumerate(rJoint_list):
        balls[ji].pos = vp.vector(joint[idx][0], joint[idx][1], joint[idx][2])
        rRods.modify(ji, pos=balls[ji].pos)

    for ji, joint in enumerate(lJoint_list):
        balls[len(rJoint_list)+ji].pos = vp.vector(joint[idx][0], joint[idx][1], joint[idx][2])
        lRods.modify(ji, pos=balls[len(rJoint_list)+ji].pos)

    headball.pos = vp.vector(Head[idx][0], Head[idx][1], Head[idx][2])

    fdhandtipDispFiltered.data = [ [idx, handtipDispGaussFiltered[idx]] ]
    fdAngAccGraph.data = [ [idx, angAcc[idx]] ]

    if (idx == 0) or (idx in strong_cp):
        seg_idx += 1
        seg_idx = 0 if seg_idx >= len(rHandtipTrajSegments) else seg_idx
        highlight_cur_seg(seg_idx, rHandtipTrajSegments)
        # highlight_cur_seg(seg_idx, lHandtipTrajSegments)

        if cap and (idx != 0) and (idx not in saved_frame_set):
            vp.scene.capture(f'{json_filename_prefix}_{target_hand}_trj_{idx}')
            saved_frame_set.add(idx)
    
    # fdrWristDisp.data=[ [idx, rWristDisp[idx]] ]
    # fdlWristDisp.data=[ [idx, lWristDisp[idx]] ]

    # fdrElbow.data=[ [idx, filtered_rAngle_Elbow[idx]] ]
    # fdrShoulder.data=[ [idx, filtered_rAngle_Shoulder[idx]] ]
    # fdlElbow.data=[ [idx, filtered_lAngle_Elbow[idx]] ]
    # fdlShoulder.data=[ [idx, filtered_lAngle_Shoulder[idx]] ]

    idx += 1
    if idx >= frame_len:
        idx = 0
        for si in (seg_idx-1, seg_idx, seg_idx+1):
            make_transparent_cur_seg(si, rHandtipTrajSegments)
            # make_transparent_cur_seg(si, lHandtipTrajSegments)
        seg_idx = -1

    if STOP:
        print('STOP!')
        #stop_server()
        break
    else:
        pass

# vcap.release()
# cv2.destroyAllWindows()

os.kill(os.getpid(), signal.SIGINT)
# vp.no_notebook.__server.shutdown()
# vp.no_notebook.__interact_loop.stop()
# print('Before sys.exit')
# sys.exit()