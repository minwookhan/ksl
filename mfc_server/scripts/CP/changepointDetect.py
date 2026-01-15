import numpy as np
import scipy.ndimage
import json
from scipy.signal import argrelmin, argrelmax

def getJointData(json_path):
    with open(json_path) as f:
        data = json.load(f)
    # data의 type과 길이를 check
    #print(type(data)) 
    #print(len(data))

    # jp_dict 생성 
    # data 에서 joint position 관련 정보만을 frame number, i 를 key로 해서
    # {i: 해당 프레임의 joint positions } 형태로 생성

    jp_dict = {}
    for i in range(len(data['frames'])):
        jp_dict[i] = data['frames'][i]['bodies'][0]['joint_positions']

    # jp_array 생성
    # jp_dict의 value값들을 가져와서 jp_list 를 생성하고,
    # jp_list 를 array 타입으로 형변환

    jp_list=list( jp_dict.values())
    jp_array=np.array(jp_list)

    # 생성된 jp_array의 dimension을 확인
    #np.shape(jp_array)

    #frame_len = len(jp_array[:,14,0])
    #print('Number of frames:', frame_len)
    # 오른팔의 손목, 팔꿈치, 어께, 쇄골점에 대한 관절점 인덱스를 정의
    rHandtip_joint_idx = 16
    rHand_joint_idx = 15
    rWrist_joint_idx = 14
    rElbow_joint_idx = 13
    rShoulder_joint_idx = 12
    rClavicle_joint_idx = 11

    # 위 과정에 대하여 왼팔에 대해서도 동일하게 계산: l_angle_Elbow & l_angle_Shoulder
    lHandtip_joint_idx = 9
    lHand_joint_idx = 8
    lWrist_joint_idx = 7
    lElbow_joint_idx = 6
    lShoulder_joint_idx = 5
    lClavicle_joint_idx = 4

    Head_joint_idx = 26
    SpineChest_joint_idx = 2

    # 관절점 정보 json으로부터 위 각 주요 관절점에 대한 시계열정보를 획득
    rHandtip = jp_array[:,rHandtip_joint_idx,:]
    rHand = jp_array[:,rHand_joint_idx,:]
    rWrist = jp_array[:,rWrist_joint_idx,:]
    rElbow = jp_array[:,rElbow_joint_idx,:]
    rShoulder = jp_array[:,rShoulder_joint_idx,:]
    rClavicle = jp_array[:,rClavicle_joint_idx,:]

    lHandtip = jp_array[:,lHandtip_joint_idx,:]
    lHand = jp_array[:,lHand_joint_idx,:]
    lWrist = jp_array[:,lWrist_joint_idx,:]
    lElbow = jp_array[:,lElbow_joint_idx,:]
    lShoulder = jp_array[:,lShoulder_joint_idx,:]
    lClavicle = jp_array[:,lClavicle_joint_idx,:]

    Head = jp_array[:,Head_joint_idx,:]
    SpineChest = jp_array[:,SpineChest_joint_idx,:]

    #print(rWrist[0], rElbow[0])
    return Head, SpineChest, rHandtip, rHand, rWrist, rElbow, rShoulder, rClavicle, lHandtip, lHand, lWrist, lElbow, lShoulder, lClavicle

def getHandTipJointData(json_path):
    with open(json_path) as f:
        data = json.load(f)
    # data의 type과 길이를 check
    #print(type(data)) 
    #print(len(data))

    # jp_dict 생성 
    # data 에서 joint position 관련 정보만을 frame number, i 를 key로 해서
    # {i: 해당 프레임의 joint positions } 형태로 생성

    jp_dict = {}
    for i in range(len(data['frames'])):
        jp_dict[i] = data['frames'][i]['bodies'][0]['joint_positions']

    # jp_array 생성
    # jp_dict의 value값들을 가져와서 jp_list 를 생성하고,
    # jp_list 를 array 타입으로 형변환

    jp_list=list( jp_dict.values())
    jp_array=np.array(jp_list)

    # 생성된 jp_array의 dimension을 확인
    #np.shape(jp_array)

    #frame_len = len(jp_array[:,14,0])
    #print('Number of frames:', frame_len)
    # 오른팔의 손목, 팔꿈치, 어께, 쇄골점에 대한 관절점 인덱스를 정의
    rHandtip_joint_idx = 16
    rHand_joint_idx = 15
    rWrist_joint_idx = 14
    rElbow_joint_idx = 13
    rShoulder_joint_idx = 12
    rClavicle_joint_idx = 11

    # 위 과정에 대하여 왼팔에 대해서도 동일하게 계산: l_angle_Elbow & l_angle_Shoulder
    lHandtip_joint_idx = 9
    lHand_joint_idx = 8
    lWrist_joint_idx = 7
    lElbow_joint_idx = 6
    lShoulder_joint_idx = 5
    lClavicle_joint_idx = 4

    Head_joint_idx = 26
    SpineChest_joint_idx = 2

    # 관절점 정보 json으로부터 위 각 주요 관절점에 대한 시계열정보를 획득
    rHandtip = jp_array[:,rHandtip_joint_idx,:]
    # rHand = jp_array[:,rHand_joint_idx,:]
    # rWrist = jp_array[:,rWrist_joint_idx,:]
    # rElbow = jp_array[:,rElbow_joint_idx,:]
    # rShoulder = jp_array[:,rShoulder_joint_idx,:]
    # rClavicle = jp_array[:,rClavicle_joint_idx,:]

    lHandtip = jp_array[:,lHandtip_joint_idx,:]
    # lHand = jp_array[:,lHand_joint_idx,:]
    # lWrist = jp_array[:,lWrist_joint_idx,:]
    # lElbow = jp_array[:,lElbow_joint_idx,:]
    # lShoulder = jp_array[:,lShoulder_joint_idx,:]
    # lClavicle = jp_array[:,lClavicle_joint_idx,:]

    # Head = jp_array[:,Head_joint_idx,:]
    # SpineChest = jp_array[:,SpineChest_joint_idx,:]

    #print(rWrist[0], rElbow[0])
    return rHandtip, lHandtip

def unit_vector(vector):
    """ Returns the unit vector of the vector.  """
    return vector / np.linalg.norm(vector)

def angle_between(v1, v2):
    """ Returns the angle in radians between vectors 'v1' and 'v2'::

            >>> angle_between((1, 0, 0), (0, 1, 0))
            1.5707963267948966
            >>> angle_between((1, 0, 0), (1, 0, 0))
            0.0
            >>> angle_between((1, 0, 0), (-1, 0, 0))
            3.141592653589793
    """
    v1_u = unit_vector(v1)
    v2_u = unit_vector(v2)
    return np.arccos(np.clip(np.dot(v1_u, v2_u), -1.0, 1.0))

def calculateAngularAcc(vecarr): # Angular acceleration
    angAcc = []
    for i in range(len(vecarr)):
        if i==0 or np.array_equal(vecarr[i], (0,0,0)) or np.array_equal(vecarr[i-1], (0,0,0)):
            angAcc.append(0)
        else:
            ang = angle_between(vecarr[i], vecarr[i-1])
            angAcc.append(ang)
    return angAcc

def calculateDisplacementVector(jarr):
    dispVec = []
    for i in range(len(jarr)):
        if i==0:
            dispVec.append(np.array([0, 0, 0]))
        else:
            disp_vector = np.array([ jarr[i][0]-jarr[i-1][0], jarr[i][1]-jarr[i-1][1], jarr[i][2]-jarr[i-1][2] ])
            dispVec.append(disp_vector)
    return dispVec


def calculateDisplacement(jarr):
    disp = []
    for i in range(len(jarr)):
        if i==0:
            disp.append(0)
        else:
            disp_vector = np.array([ jarr[i][0]-jarr[i-1][0], jarr[i][1]-jarr[i-1][1], jarr[i][2]-jarr[i-1][2] ])
            disp.append(np.linalg.norm(disp_vector))
    return disp


def checkStrongHand(handTipData0, handTipData1):
    handtipDisp0 = calculateDisplacement(handTipData0)
    handtipDisp1 = calculateDisplacement(handTipData1)
    handtipSum0 = sum(handtipDisp0)
    handtipSum1 = sum(handtipDisp1)
    if handtipSum0 >= handtipSum1:
        result = 0
    else:
        result = 1
    return result

def findminmax(vals, mms, thres): # mms: mins and maxs
    totalMaxValue = np.max(vals)
    last_min = None
    for (frame, kind) in mms:
        if kind =='min':
            last_min = frame
            continue
        if (kind=='max') and (vals[frame] > totalMaxValue*thres) and last_min:
            #print(frame, kind)
            break
    return last_min

def findzero(vals, startFrame):
    for i, v in enumerate(vals[startFrame:]):
        # print(f'{i+startFrame},{v}', end=' ')
        if v == 0:
            return i+startFrame

def fixmaxmax(vals, mms):
    lastframe = None
    lastkind = None
    for (frame, kind) in mms:
        if lastkind=='max' and kind=='max':
            # print('Max max,', frame)
            newMinFrame = findzero(vals, lastframe)
            # print('newMinFrame', newMinFrame)
            if newMinFrame:
                mms.append( (newMinFrame, 'min') )
                # print('new min apppended', mms)
        lastframe = frame
        lastkind = kind
    mms.sort(key=lambda element : element[0])
    return mms

def findBoundary(vals, thres=0.3):
    localMin = argrelmin(vals)[0]
    localMax = argrelmax(vals)[0]
    mins = [(i, 'min') for i in localMin]
    maxs = [(i, 'max') for i in localMax]
    mms = mins + maxs
    mms.sort(key=lambda element : element[0])
    mms = fixmaxmax(vals, mms)

    head = findminmax(vals, mms, thres)

    mms.sort(reverse=True, key=lambda element : element[0])
    tail = findminmax(vals, mms, thres)

    return head, tail

def calConfidence(cp_list, displacement_array, verbose=False):
    maxValue = np.max(displacement_array)
    cp_with_conf_list = []

    for cp_frame_num in cp_list:
        disp_val = displacement_array[int(cp_frame_num)]
        cp_with_conf_list.append( (int(cp_frame_num), 1 - (disp_val/maxValue)) )

    return cp_with_conf_list

def calFilteredLocalMin(displacement_array, MIN_THRESHOLD, MINOR_THRESHOLD, ADJ_LOCAL_THRESHOLD, MINOR_MAX_THRESHOLD, verbose=False):
    # verbose=True
    displacementLocalMin = argrelmin(displacement_array)[0]
    displacementLocalMax = argrelmax(displacement_array)[0]

    maxValue = np.max(displacement_array)
    if verbose: print('Maximum displacement:', maxValue)

    if verbose: print('LocalMin:', displacementLocalMin)
    displacementLocalMin = removeBoundary(displacementLocalMin, len(displacement_array))
    if verbose: print('LocalMin after removeBoundary:', displacementLocalMin)

    displacementLocalMin_minor_max_filtered = filterMinorMax(displacementLocalMin, displacementLocalMax, displacement_array, MINOR_MAX_THRESHOLD)
    if verbose: print('Filtered at filterMinorMax:', np.setdiff1d(displacementLocalMin, displacementLocalMin_minor_max_filtered, assume_unique=True))
    if verbose: print('LocalMin after filterMinorMax:', displacementLocalMin_minor_max_filtered)

    if len(displacementLocalMin_minor_max_filtered) < 1:
        displacementLocalMin_minor_max_filtered = displacementLocalMin
    min_filter = displacement_array[displacementLocalMin_minor_max_filtered] <= maxValue*MIN_THRESHOLD
    displacementLocalMin_filtered = displacementLocalMin_minor_max_filtered[min_filter]
    if verbose: print('LocalMin after min_filter', displacementLocalMin_filtered)
    
    displacementLocalMin_filtered = filterMinorMin(displacementLocalMin_filtered, displacementLocalMax, displacement_array, MINOR_THRESHOLD, ADJ_LOCAL_THRESHOLD)
    if verbose: print('LocalMin after filterMinorMin', displacementLocalMin_filtered)

    weak_cp = np.setdiff1d(displacementLocalMin_minor_max_filtered, displacementLocalMin_filtered, assume_unique=True)
    return displacementLocalMin_filtered, weak_cp


def findAdjacentMaxIdxs(mini, maxarr):
    for i in range(len(maxarr)-1):
        if (maxarr[i] < mini) and (mini < maxarr[i+1]):
            return (maxarr[i], maxarr[i+1])
    return None

def removeBoundary(minarr, data_length):
    bound = np.array([0, data_length-1])
    newmin = np.setdiff1d(minarr, bound)

    return newmin

def filterMinorMax(minarr, maxarr, valarr, minor_max_threshold, verbose=False):
    totalMaxValue = np.max(valarr)
    thredhold_value = totalMaxValue*minor_max_threshold

    new_minarr = []
    for idx, mi in enumerate(minarr):
        maxes = findAdjacentMaxIdxs(mi, maxarr)
        if (maxes and (valarr[maxes[0]] < thredhold_value) and (valarr[maxes[1]] < thredhold_value)) \
            or (idx==0 and maxes == None) or (idx==len(minarr)-1 and maxes == None):
            adjmaxstr = ''
            if maxes and verbose: adjmaxstr = f', Adjacent Max Values:{valarr[maxes[0]]}, {valarr[maxes[1]]}'
            if verbose:print(f'filterMinorMax Min index:{mi}'+adjmaxstr)
        else:
            new_minarr.append(mi)

    if new_minarr:
        first_adjMaxIdxs = findAdjacentMaxIdxs(new_minarr[0], maxarr)
        if (first_adjMaxIdxs == None) or (valarr[first_adjMaxIdxs[0]] < thredhold_value):
            new_minarr.pop(0)

    if new_minarr:
        last_adjMaxIdxs = findAdjacentMaxIdxs(new_minarr[-1], maxarr)
        if (last_adjMaxIdxs == None) or (valarr[last_adjMaxIdxs[1]] < thredhold_value):
            new_minarr.pop()

    return np.array(new_minarr)

def filterMinorMin(minarr, maxarr, valarr, threshold, adj_local_threshold, verbose=False):
    totalMaxValue = np.max(valarr)
    thredhold_value = totalMaxValue*threshold
    if verbose: print(f'Thredhold_value:{thredhold_value}')
    new_minarr = []
    for idx, mi in enumerate(minarr):
        minval = valarr[mi]
        maxes = findAdjacentMaxIdxs(mi, maxarr)
        left_min = None
        left_minval = None
        if (idx > 0):
            left_min = minarr[idx-1]
            left_minval = valarr[left_min]
        
        right_min = None
        right_minval = None
        if (idx < len(minarr)-1):
            right_min = minarr[idx+1]
            right_minval = valarr[right_min]

        # if maxes:
        #     print(f'Min index:{mi}, Min Value:{minval}, Adjacent Max Values:{valarr[maxes[0]]}, {valarr[maxes[1]]}, Difference:{valarr[maxes[0]]-minval}, {valarr[maxes[1]]-minval}')
        if maxes and ( ((valarr[maxes[0]]-minval < thredhold_value) and (left_min) and (left_minval<=minval) and (mi-left_min < adj_local_threshold))
            or ((valarr[maxes[1]]-minval< thredhold_value) and (right_min) and (right_minval<=minval) and (right_min-mi < adj_local_threshold)) ):
            if verbose: print(f'filterMinorMin Min index:{mi}, Min Value:{minval}, Adjacent Max Values:{valarr[maxes[0]]}, {valarr[maxes[1]]}, Difference:{valarr[maxes[0]]-minval}, {valarr[maxes[1]]-minval}')
            if left_min and verbose:
                print(f'filterMinorMin left_min:{left_min}, left_minval:{left_minval}, {left_minval<=minval}, {(mi-left_min < adj_local_threshold)}')
            if right_min and verbose:
                print(f'filterMinorMin right_min:{right_min}, right_minval:{right_minval}, {right_minval<=minval}, {(right_min-mi < adj_local_threshold)}')
            continue
        else:
            new_minarr.append(mi)

    return np.array(new_minarr)

def findSignificantMaxs(minarr, maxarr, valarr):
    mins = [(i, 'min') for i in minarr]
    maxs = [(i, 'max') for i in maxarr]
    mms = mins + maxs
    mms.sort(key=lambda element : element[0])
    new_maxarr = []

    sigMax = None
    sigMaxValue = 0
    for (frame, kind) in mms:
        if kind =='min':
            if sigMax:
                new_maxarr.append(sigMax)
            sigMax = None
            sigMaxValue = 0
        else:
            if sigMaxValue <= valarr[frame]:
                sigMax = frame
                sigMaxValue = valarr[frame]
    new_maxarr.append(sigMax)
    return new_maxarr


def find_closest(val, alist):
    diff = 10000
    closest = None
    for aaf in alist:
        new_diff = abs(val-aaf)
        if new_diff>diff:
            break
        else:
            closest = aaf
            diff = new_diff
    return closest

def changepoint_detector(handTip, min_threshold=0.35, minor_threshold=0.1, adj_local_threshold=15, sigma=2.0, 
                         minor_max_threshold=0.2, angular_acc_threshold = 2):
    frame_len = len(handTip)
    #print('Number of frames:', frame_len)
    
    Handtip_gfiltered = scipy.ndimage.gaussian_filter1d(handTip, sigma, axis=0)
    HandtipDisp = np.array(calculateDisplacement(Handtip_gfiltered))
    handtipDispGaussFiltered = scipy.ndimage.gaussian_filter1d(HandtipDisp, sigma)
    strong_cp, weak_cp = calFilteredLocalMin(handtipDispGaussFiltered, min_threshold, minor_threshold, adj_local_threshold, minor_max_threshold, False)

    if True:
        HandtipDispVector = calculateDisplacementVector(Handtip_gfiltered)
        angAcc_org = calculateAngularAcc(HandtipDispVector)    
        angAcc = scipy.ndimage.gaussian_filter1d(angAcc_org, sigma)
        angAccLocalMax = argrelmax(angAcc)[0]
        # print(f'angAccLocalMax:{angAccLocalMax}')

        weak2strong = []
        for weak in weak_cp:
            closeAngAccLocalMax = find_closest(weak, angAccLocalMax)
            if abs(weak - closeAngAccLocalMax) <= angular_acc_threshold:
                weak2strong.append(weak)
        # print(f'weak2strong:{weak2strong}')

        if weak2strong:
            strong_cp = np.concatenate( (strong_cp, weak2strong), axis=None )
            strong_cp.sort()
            weak_cp = np.setdiff1d(weak_cp, weak2strong, assume_unique=True)

    #print('Strong Changepoints:', strong_cp)
    #print('Weak Changepoints:', weak_cp)
    #TODO output= np.array([frame_num, strength])
    return strong_cp, weak_cp

def changepoint_detector_confi(handTip, min_threshold=0.35, minor_threshold=0.1, adj_local_threshold=15, sigma=2.0, 
                         minor_max_threshold=0.2, angular_acc_threshold = 2):
    frame_len = len(handTip)
    #print('Number of frames:', frame_len)
    
    Handtip_gfiltered = scipy.ndimage.gaussian_filter1d(handTip, sigma, axis=0)
    HandtipDisp = np.array(calculateDisplacement(Handtip_gfiltered))
    handtipDispGaussFiltered = scipy.ndimage.gaussian_filter1d(HandtipDisp, sigma)
    strong_cp, weak_cp = calFilteredLocalMin(handtipDispGaussFiltered, min_threshold, minor_threshold, adj_local_threshold, minor_max_threshold, False)

    if True:
        HandtipDispVector = calculateDisplacementVector(Handtip_gfiltered)
        angAcc_org = calculateAngularAcc(HandtipDispVector)    
        angAcc = scipy.ndimage.gaussian_filter1d(angAcc_org, sigma)
        angAccLocalMax = argrelmax(angAcc)[0]
        # print(f'angAccLocalMax:{angAccLocalMax}')

        weak2strong = []
        for weak in weak_cp:
            closeAngAccLocalMax = find_closest(weak, angAccLocalMax)
            if abs(weak - closeAngAccLocalMax) <= angular_acc_threshold:
                weak2strong.append(weak)
        # print(f'weak2strong:{weak2strong}')

        if weak2strong:
            strong_cp = np.concatenate( (strong_cp, weak2strong), axis=None )
            strong_cp.sort()
            weak_cp = np.setdiff1d(weak_cp, weak2strong, assume_unique=True)

    strong_cp_confi = calConfidence(strong_cp, handtipDispGaussFiltered)
    weak_cp_confi = calConfidence(weak_cp, handtipDispGaussFiltered)
    # print('Strong Changepoints:', strong_cp_confi)
    # print('Weak Changepoints:', weak_cp_confi)
    return strong_cp_confi, weak_cp_confi

def changepoint_detector_1array(handTip, min_threshold=0.35, minor_threshold=0.1, adj_local_threshold=15, sigma=2.0, minor_max_threshold=0.2, angular_acc_threshold=2):
    strong_cp, weak_cp = changepoint_detector(handTip, min_threshold, minor_threshold, adj_local_threshold, sigma, minor_max_threshold, angular_acc_threshold)
    cp_list = []
    for cp in strong_cp:
        cp_list.append((cp, 1))
    for cp in weak_cp:
        cp_list.append((cp, 0))
    return np.array(cp_list)

def changepoint_detector_1array_confi(handTip, min_threshold=0.35, minor_threshold=0.1, adj_local_threshold=15, sigma=2.0, minor_max_threshold=0.2, angular_acc_threshold=2):
    strong_cp, weak_cp = changepoint_detector_confi(handTip, min_threshold, minor_threshold, adj_local_threshold, sigma, minor_max_threshold, angular_acc_threshold)
    cp_list = []
    for cp in strong_cp:
        cp_list.append((cp[0], 1, cp[1]))
    for cp in weak_cp:
        cp_list.append((cp[0], 0, cp[1]))
    return np.array(cp_list)

def changepoint_top2(handTip, min_threshold=0.35, minor_threshold=0.1, adj_local_threshold=15, sigma=2.0, minor_max_threshold=0.1):
    HandtipDisp = np.array(calculateDisplacement(handTip))
    handtipDispGaussFiltered = scipy.ndimage.gaussian_filter1d(HandtipDisp, sigma)
    strong_cp, weak_cp = calFilteredLocalMin(handtipDispGaussFiltered, min_threshold, minor_threshold, adj_local_threshold, minor_max_threshold, False)

    if len(strong_cp) == 2:
        top2 = strong_cp
    else:
        if len(strong_cp) < 2:
            strong_cp = np.concatenate((strong_cp, weak_cp))
        displacementLocalMax = argrelmax(handtipDispGaussFiltered)[0]
        valarr = handtipDispGaussFiltered
        sigMaxs = findSignificantMaxs(strong_cp, displacementLocalMax, valarr)

        diffarr = []
        for idx, mi in enumerate(strong_cp):
            minval = valarr[mi]
            maxes = findAdjacentMaxIdxs(mi, sigMaxs)
            # print(f'Maxes:{maxes}')
            diffarr.append( (mi, valarr[maxes[0]]-minval + valarr[maxes[1]]-minval) )

        if len(diffarr) == 1:
            diffarr.append( (0,0) )

        diffarr.sort(reverse=True, key= lambda element : element[1])
        # print(f'diffarr:{diffarr}')

        top2 = [diffarr[0][0], diffarr[1][0]]
        top2.sort()
    return np.array(top2)