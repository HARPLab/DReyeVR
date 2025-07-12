import matplotlib.pyplot as plt
import pandas as pd

plt.style.use('default')

columns =  ['transformX', 'transformY', 'transformZ',
            'LocationX', 'LocationY',  'LocationZ',
            'transformRoll', 'transformPitch', 'transformYaw',
            'RotationRoll', 'RotationPitch', 'RotationYaw',
            'VelocityX', 'VelocityY', 'VelocityZ',
            'AngularVelocityX', 'AngularVelocityY', 'AngularVelocityZ',
            'brake_input', 'throttle_input', 'steering_input',
           ]

def plot_data_with_timestamp(df):
    plt.style.use('default')
    plt.figure(figsize=(10, 10))
    cols = 3
    rows = int(len(columns) // cols)
    for i in range(len(columns)):
        plt.subplot(rows, cols, i+1)
        plt.plot(df['timestamp_carla'], df[columns[i]])
        plt.xlabel('timestamp_carla')
        plt.ylabel(columns[i])
        plt.grid()

    plt.tight_layout()
    plt.show()
    plt.close()

if __name__ == "__main__":
    from pathlib import Path
    __path__ = Path(__file__).resolve().parent
    df = pd.read_excel(f'{__path__}/trials/trial12.xlsx')
    plot_data_with_timestamp(df)
