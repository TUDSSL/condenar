# Log processing script

The script 'processLogs.py' takes any log files located in the 'RAW' subdirectory and analyzes them.

It first cleans any unsused parts of the logs, intervals of time where the user is not actively using the console. It trims not only the beggining and the end, but also parts of the experiment where the researcher is changing apps or settings.

It is also in charge of linking each log file with the experiment variation. If more experiments are performed, the reasearcher is required to add the experiment variation for each log file in the 'experiment_variations' dictionary. (Top of the file, starting at line 7)

The processed data is also read and ready to be used in further processing as a pandas dataframe.

It finally calculates the interaction percentage of each log file (which % of the time the user is actually interacting with the console) and plots it.