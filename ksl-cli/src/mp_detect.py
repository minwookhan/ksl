import cv2
import numpy as np
import mediapipe as mp

# 이미지 단위 전용: static_image_mode=True (추적 비사용)
mp_hands = mp.solutions.hands
hands_image = mp_hands.Hands(
    static_image_mode=True,      # ← 이미지 한 장씩 처리
    max_num_hands=2,
    model_complexity=1,
    min_detection_confidence=0.5
)

import numpy as np
import cv2


def mediapipe_hand_func(img):
    """
    입력: BGR 이미지 (np.ndarray, HxWx3)
    출력: np.array([all_x, all_y, all_z, all_vis], dtype=float32)
          - 정규화 좌표(0..1)
          - 한 손 마다 21개 포인트 감지, 최대 2손 (오른손+왼손): 42 포인트 길이 numpy 배열 리턴
    데이터 정렬 순서
     - 데이터는 항상 **오른손(Right)** 데이터가 먼저 오고, 그 뒤에 **왼손(Left)** 데이터가 붙는 순서로 정렬됩니다 (`Right → Left`).
     - 각 손마다 **21개의 랜드마크** 포인트가 순서대로 나열됩니다.
    """
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB) if (img.ndim == 3 and img.shape[2] == 3) else img
    results = hands_image.process(rgb)

    all_x, all_y, all_z, all_side = [], [], [], []

    if results.multi_hand_landmarks:
        packs = []
        for i, hlm in enumerate(results.multi_hand_landmarks):
            side, score = "Unknown", 0.0
            if results.multi_handedness and i < len(results.multi_handedness):
                cls = results.multi_handedness[i].classification[0]
                side, score = cls.label, float(cls.score)  # "Right" / "Left"
            packs.append((side, score, hlm.landmark))

        # 항상 Right → Left, 같은 손이 여러 개면 score 높은 것 우선
        priority = {"Right": 1, "Left": 0}
        packs.sort(key=lambda t: (priority.get(t[0], 2), -t[1]))

        for side, score, lm_list in packs:
            if side not in ("Right", "Left"):
                continue
            for lm in lm_list:  # 21개
                all_x.append(float(lm.x))
                all_y.append(float(lm.y))
                all_z.append(float(lm.z))
                if side=="Right":
                    all_side.append(1.0)
                else :
                    all_side.append(0.0)

    return np.array([all_x, all_y, all_z, all_side], dtype=np.float32)



mp_pose = mp.solutions.pose
pose = mp_pose.Pose(static_image_mode=False, model_complexity=1)

def mediapipe_pose_func(img):
 #   image = cv2.flip(img, 1) 
    rgb_frame = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
#    rgb_frame = mp.Image(image_format=mp.ImageFormat.SRGB, data=img)
    # STEP 4: Detect pose landmarks from the input image.    
    results = pose.process(rgb_frame)
    #print('******************detection_result',detection_result)
    all_x, all_y, all_z, all_vis = [], [], [], []

    if results.pose_landmarks:
        for lm in results.pose_landmarks.landmark:
            all_x.append(lm.x)
            all_y.append(lm.y)
            all_z.append(lm.z)
            all_vis.append(lm.visibility)

    return np.array([all_x, all_y, all_z, all_vis])
