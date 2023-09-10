import pandas as pd
import glob
import os
import matplotlib.pyplot as plt

# Define experiment variations
experiment_variations = {1: 'Power Level', 2: 'Crank', 3: 'Power Level', 
                         4: 'Power Level', 5: 'Power Level', 6: 'Battery', 
                         7: 'Battery',
                         10: 'Power Level'}

# Define channel columns mapping
channel_mapping = {0: 'APP_NAME', 1: 'LIGHT_SENSOR', 2: 'SLEEP_PERCENT'}

# Get a list of all csv files in the "RAW" directory
csv_files = glob.glob('RAW/*.csv')

#Get only the first 3 files
#csv_files = csv_files[:4]

# Initialize a list to hold all dataframes
df_list = []

for filename in csv_files:
    # Load the CSV file into a DataFrame
    df = pd.read_csv(filename)
    
    # Extract the experiment ID from the filename
    experiment_id = int(os.path.basename(filename).split('_')[0])
    
    # Add a new column for the experiment ID and variation
    df['experiment_id'] = experiment_id
    df['experiment_variation'] = experiment_variations[experiment_id]

    # Rename channel columns
    for key, value in channel_mapping.items():
        column_name = f'Channel{key}'
        if column_name in df.columns:
            df.rename(columns={column_name: value}, inplace=True)

        # Remove unused channel columns
    for i in range(25):
        if i not in channel_mapping.keys():
            column_name = f'Channel{i}'
            if column_name in df.columns:
                df.drop(column_name, axis=1, inplace=True)

    # Remove unused event columns
    for i in range(25):
        column_name = f'Event{i}'
        if column_name in df.columns:
            df.drop(column_name, axis=1, inplace=True)

    # Append the DataFrame to the list
    df_list.append(df)

# Combine all the dataframes into a single dataframe
all_data = pd.concat(df_list, ignore_index=True)

# Split data by experiment id
grouped = all_data.groupby('experiment_id')


def isRowBeingInteractedWith(row, includeButtons=True):
    if includeButtons and (row['B1'] < 1 or row['B2'] < 1 or row['B3'] < 1 or row['B4'] < 1 or row['B5'] < 1 or row['B6'] < 1):
        return True
    return abs(row['Joy1'] - 950) > 100 or abs(row['Joy2'] - 950) > 100 or row['CrankPower'] > 0.01

def process_experiment_group(group):
    last_activity_time = None
    active_rows = []
    oldJoy1 = 0
    oldJoy2 = 0
    oldJoy1SameSampleNum = 0
    oldJoy2SameSampleNum = 0

    group['IsActive'] = 50

    print (f'Processing experiment {group["experiment_id"].iloc[0]}')

    for i, row in group.iterrows():
        current_app = row['APP_NAME']

        if(row['Joy1'] < 100): 
            row['Joy1'] = 950
        
        if(row['Joy2'] < 100):
            row['Joy2'] = 950

        if(row['Joy1'] == oldJoy1):
            oldJoy1SameSampleNum += 1
            if oldJoy1SameSampleNum > 2:
                row['Joy1'] = 950
        else:
            oldJoy1SameSampleNum = 0

        oldJoy1 = row['Joy1']

        if(row['Joy2'] == oldJoy2):
            oldJoy2SameSampleNum += 1
            if oldJoy2SameSampleNum > 2:
                row['Joy2'] = 950
        else:
            oldJoy2SameSampleNum = 0

        oldJoy2 = row['Joy2']

       

        if(current_app != 68 and current_app != 71): # 68 = "Doom", 71 = "Tetris"
            last_activity_time = None
            continue

        # Check if there's significant joystick activity
        if isRowBeingInteractedWith(row):
            last_activity_time = row['Time']

        # Check if the app has changed
        if last_activity_time is not None and last_activity_time + 15e3  > row['Time']:   
            group.loc[i, 'IsActive'] = 100        
            active_rows.append(i) 
        


    print (f'Active rows: {len(active_rows)} / {len(group)} ({len(active_rows) / len(group) * 100:.2f}%))')

    return group.loc[active_rows]

# Apply the function to each experiment group
filtered_data = all_data.groupby('experiment_id').apply(process_experiment_group).reset_index(drop=True)


# Initialize a dictionary to hold interaction percentages for each experiment variation
interaction_percentages = {}
interaction_percentagesRAW = []

for name, group in filtered_data.groupby('experiment_id'):

    # Calculate the percentage of rows where isRowBeingInteractedWith returns true
    print (f'Experiment {name}: {len(group[group.apply(isRowBeingInteractedWith, axis=1)]) / len(group) * 100:.2f}%')

    # Calculate the percentage of rows where isRowBeingInteractedWith returns true
    interaction_percentage = len(group[group.apply(isRowBeingInteractedWith, axis=1)]) / len(group) * 100
    # Store the interaction percentage for this experiment variation
    experiment_variation = group['experiment_variation'].iloc[0]
    if experiment_variation not in interaction_percentages:
        interaction_percentages[experiment_variation] = []
    interaction_percentages[experiment_variation].append(interaction_percentage)

    
    # Store the interaction percentage for this experiment
    experiment_id = group['experiment_id'].iloc[0]
    experiment_variation = group['experiment_variation'].iloc[0]
    interaction_percentagesRAW.append((experiment_id, experiment_variation, interaction_percentage))



    #fig, ax1 = plt.subplots(figsize=(10,5))    
    #
    ## Plot Joy1 and Joy2 for each experiment
    #ax1.plot(group['Joy1'], label='Joy1', alpha=0.7)
    #ax1.plot(group['Joy2'], label='Joy2', alpha=0.7)
#
    ## Plot App Name on a secondary y-axis
    #ax2 = ax1.twinx()
    #ax2.plot(group['APP_NAME'], label='APP_NAME', alpha=0.7, linestyle='dotted')
    #ax2.plot(group['IsActive'], label='IsActive', alpha=0.7, linestyle='dashed')
    #
    #fig.suptitle(f'Experiment {name}')
    #ax1.set_xlabel('Time')
    #ax1.set_ylabel('Joystick value')
    #ax2.set_ylabel('App')
    #fig.legend(loc="upper right")
#
    #plt.show()

# After processing all data, plot interaction percentages by experiment variation
# After processing all data, plot interaction percentages by experiment variation
fig, ax = plt.subplots()

# Convert interaction percentages into a DataFrame for easier plotting
percentages_df = pd.DataFrame([(key, var) for key, values in interaction_percentages.items() for var in values], columns=['Experiment Variation', 'Interaction Percentage'])

# Plot a boxplot
boxplot = percentages_df.boxplot(by='Experiment Variation', column='Interaction Percentage', ax=ax, grid=False)

# Set plot properties
ax.set_xlabel('Experiment type')
ax.set_ylabel('Interaction Percentage')
plt.suptitle('')
#ax.set_title('Interaction Percentages by Experiment Variation')
plt.show()





# After processing all data, plot interaction percentages by experiment variation
interaction_df = pd.DataFrame(interaction_percentagesRAW, columns=['Experiment ID', 'Experiment Variation', 'Interaction Percentage'])
interaction_df.sort_values(by=['Experiment Variation', 'Experiment ID'], inplace=True)

fig, ax = plt.subplots(figsize=(10,5))

# Iterate over each experiment variation and plot a bar for each experiment
for i, (variation, group) in enumerate(interaction_df.groupby('Experiment Variation')):
    ax.bar([str(x) + ' ' + variation for x in group['Experiment ID']], group['Interaction Percentage'], label=variation)

ax.set_xlabel('Experiment ID and Variation')
ax.set_ylabel('Interaction Percentage')
ax.legend()
plt.xticks(rotation=90)
plt.tight_layout()
plt.show()