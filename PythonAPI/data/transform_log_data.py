# transform data/log to xlsx
from pathlib import Path
from pprint import pprint

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

"""Frame log template
{'AngularVelocity': array([-4.37428025e-06,  6.01823886e-05, -7.62434183e-06]),
 'BL_Wheel_Angle': 0.0,
 'BR_Wheel_Angle': 0.0,
 'FL_Wheel_Angle': 0.0,
 'FR_Wheel_Angle': 0.0,
 'Location': array([ -2.13139057, -13.58492756,   0.16977863]),
 'Rotation': array([ 2.82769813e-03, -8.76779175e+01,  6.53911904e-02]),
 'Velocity': array([5.51639096e-06, 9.15236285e-07, 5.89568799e-06]),
 'brake_input': 0.0,
 'camera_location': array([-2.73157787, 17.28938675, -1.66063035]),
 'camera_rotation': array([-14.0348835 ,  -0.6602667 ,  -0.08262103]),
 'current_gear_input': False,
 'focus_actor_dist': 687.36474609375,
 'focus_actor_name': 'Road_Grass_Town05_123',
 'focus_actor_pt': array([ -199.36732483, -2041.00537109,    17.01990509]),
 'frame': 17268,
 'frame_number': 17268,
 'framesequence': 66642,
 'gaze_dir': array([0.99712372, 0.02189636, 0.05830383]),
 'gaze_origin': array([-3.93994713, -0.01803055, -0.53388059]),
 'gaze_valid': True,
 'gaze_vergence': 0.7961492538452148,
 'handbrake_input': False,
 'left_eye_openness': 1.0,
 'left_eye_openness_valid': True,
 'left_gaze_dir': array([0.99966431, 0.02079773, 0.01531982]),
 'left_gaze_origin': array([-3.71361256,  2.82757282, -0.55886388]),
 'left_gaze_valid': True,
 'left_pupil_diam': 3.4825439453125,
 'left_pupil_posn': array([ 0.31178343, -0.64217329]),
 'left_pupil_posn_valid': True,
 'right_eye_openness': 1.0,
 'right_eye_openness_valid': True,
 'right_gaze_dir': array([0.99458313, 0.022995  , 0.10128784]),
 'right_gaze_origin': array([-4.16628122, -2.86363387, -0.50889742]),
 'right_gaze_valid': True,
 'right_pupil_diam': 3.1923370361328125,
 'right_pupil_posn': array([-0.23540282, -0.80398464]),
 'right_pupil_posn_valid': True,
 'steering_input': 0.0003295999194961041,
 'throttle_input': 0.0,
 'timestamp': 564.725004479289,
 'timestamp_carla': 564726,
 'timestamp_device': 3718156,
 'timestamp_stream': 564.725004479289,
 'transform': [array([2.11354113e+00, 1.51294756e+01, 5.36903366e-03]),
               array([-0.06390325, -7.67797327,  0.01413985])]}
"""

# necessary data
tab = {
    'timestamp_carla': [],
    'timestamp_device': [],
    'frame_number': [],
    'brake_input': [],
    'steering_input': [],
    'throttle_input': [],
    'LocationX': [],
    'LocationY': [],
    'LocationZ': [],
    'RotationRoll': [],
    'RotationPitch': [],
    'RotationYaw': [],
    'AngularVelocityX': [],
    'AngularVelocityY': [],
    'AngularVelocityZ': [],
    'VelocityX': [],
    'VelocityY': [],
    'VelocityZ': [],
    'transformX': [],
    'transformY': [],
    'transformZ': [],
    'transformRoll': [],
    'transformPitch': [],
    'transformYaw': [],  
    'FL_Wheel_Angle': [],
    'FR_Wheel_Angle': [],
    'BL_Wheel_Angle': [],
    'BR_Wheel_Angle': [],  
}

# txt
__path__ = Path(__file__).resolve().parent
log_path = f'{__path__}/trials/trial_v_equal_vx.txt'

with open(log_path) as rf:
    lines = rf.readlines()
    
    for line in lines:
        if 'brake_input' in line and 'handbrake_input' not in line:
            tab['brake_input'].append(float(line.split(': ')[1].split(',')[0]))
        if 'steering_input' in line:
            tab['steering_input'].append(float(line.split(': ')[1].split(',')[0]))
        if 'throttle_input' in line:
            tab['throttle_input'].append(float(line.split(': ')[1].split(',')[0]))
        if 'timestamp_carla' in line:
            tab['timestamp_carla'].append(float(line.split(': ')[1].split(',')[0]))
        if 'timestamp_device' in line:
            tab['timestamp_device'].append(float(line.split(': ')[1].split(',')[0]))
        if 'frame_number' in line:
            tab['frame_number'].append(int(line.split(': ')[1].split(',')[0]))
        if 'transform' in line:
            # print(line.split('[')[2])
            transform_data = line.split('[')[2].split(']')[0].split(',')
            tab['transformX'].append(float(transform_data[0]))
            tab['transformY'].append(float(transform_data[1]))
            tab['transformZ'].append(float(transform_data[2]))
            
        if '}' in line:
            rpy_data = line.split('[')[1].split(']')[0].split(',')
            tab['transformPitch'].append(float(rpy_data[0]))
            tab['transformYaw'].append(float(rpy_data[1]))
            tab['transformRoll'].append(float(rpy_data[2]))
            
        if 'Location' in line:
            location_data = line.split('[')[1].split(']')[0].split(',')
            tab['LocationX'].append(float(location_data[0]))
            tab['LocationY'].append(float(location_data[1]))
            tab['LocationZ'].append(float(location_data[2]))
            
        if 'Rotation' in line:
            rpy_data = line.split('[')[1].split(']')[0].split(',')
            tab['RotationPitch'].append(float(rpy_data[0]))
            tab['RotationYaw'].append(float(rpy_data[1]))
            tab['RotationRoll'].append(float(rpy_data[2]))
        
        if 'Velocity' in line and 'AngularVelocity' not in line:
            v_data = line.split('[')[1].split(']')[0].split(',')
            tab['VelocityX'].append(float(v_data[0]))
            tab['VelocityY'].append(float(v_data[1]))
            tab['VelocityZ'].append(float(v_data[2]))
            
        if 'AngularVelocity' in line:
            av_data = line.split('[')[1].split(']')[0].split(',')
            tab['AngularVelocityX'].append(float(av_data[0]))
            tab['AngularVelocityY'].append(float(av_data[1]))
            tab['AngularVelocityZ'].append(float(av_data[2]))      
            
        if 'FL_Wheel_Angle' in line:
            tab['FL_Wheel_Angle'].append(float(line.split(': ')[1].split(',')[0]))
        if 'FR_Wheel_Angle' in line:
            tab['FR_Wheel_Angle'].append(float(line.split(': ')[1].split(',')[0]))
        if 'BL_Wheel_Angle' in line:
            tab['BL_Wheel_Angle'].append(float(line.split(': ')[1].split(',')[0]))
        if 'BR_Wheel_Angle' in line:
            tab['BR_Wheel_Angle'].append(float(line.split(': ')[1].split(',')[0]))

df = pd.DataFrame().from_dict(tab)
df.to_excel(log_path.replace('.txt', '.xlsx'), index=False)
print(df.head())


     
