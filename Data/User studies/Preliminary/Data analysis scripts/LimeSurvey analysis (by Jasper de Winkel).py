import pandas as pd
import sys
import matplotlib.pyplot as plt
 
path = sys.argv[1]
orderString = 'Research2. Which order are the participants following? '
orders = ['(1) Battery - DOOM - Tetris',
          '(2) Battery - Tetris - DOOM',
          '(3) Power Level - DOOM - Tetris',
          '(4) Power Level - Tetris - DOOM',
          '(5)  Connected - DOOM - Tetris',
          '(6)  Connected - Tetris - DOOM'
         ]

df = pd.read_csv(path)

# Mon merged 
grouped = df.replace(r'\D', '', regex=True) # remove everything that is not a number
for col in grouped.columns:
    try:
      grouped[col] = grouped[col].astype(float)
    except:
      print(col + ' failed converting to int')

grouped[orderString] = grouped[orderString].replace({1:orders[0],2:orders[1],3:orders[2],4:orders[3],5:orders[4],6:orders[5]})


# Merging the alternate options
#grouped = df.replace([orders[1]],orders[0])
#grouped = grouped.replace([orders[3]],orders[2])

# Grouping by the option 1 or 3 or 5
grouped = grouped.groupby([orderString]).mean()
xlKeys = ['Tetris', 'DOOMA', 'DOOMB']



# -------------------------------------------

plt.figure(0)
xlQuest = '[How mentally demanding was the task?|Very low|Very high]'
xlSequence = '8[SQ001].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace('|Very low|Very high]',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('1.png')

plt.figure(1)
xlQuest = '[How physically demanding was the task?|Very low|Very high]'
xlSequence = '8[SQ002].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace('|Very low|Very high]',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('2.png')

plt.figure(2)
xlQuest = '[How hurried or rushed was the pace of the task?|Very low|Very high]'
xlSequence = '8[SQ003].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace('|Very low|Very high]',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('3.png')
 
plt.figure(3)
xlQuest = '[How successful were you in accomplishing what you were asked to do?|Perfect|Failure]'
xlSequence = '8[SQ004].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace('|Perfect|Failure]',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('4.png')

plt.figure(4)
xlQuest = '[How hard did you have to work to accomplish your level of performance?|Very low|Very high]'
xlSequence = '8[SQ005].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace('|Very low|Very high]',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('5.png')

plt.figure(5)
xlQuest = '[How insecure, discouraged, irritated, stressed, and annoyed were you?|Very low|Very high]'
xlSequence = '8[SQ006].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace('|Very low|Very high]',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('6.png')

# -------------------------------------------

plt.figure(21)
xlQuest = '[To what extent did you feel that the game was something you were experiencing, rather than something you were just doing?   ]'
xlSequence = '2[SQ001].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace('   ]',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('21.png')

plt.figure(22)
xlQuest = '[To what extent did you feel emotionally attached to the game?]'
xlSequence = '3[SQ001].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace(']',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('22.png')

plt.figure(23)
xlQuest = '[To what extent were you interested in seeing how the game’s events would progress?]'
xlSequence = '4[SQ001].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace(']',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('23.png')

plt.figure(24)
xlQuest = '[At any point did you find yourself become so involved that you wanted to speak to the game directly?]'
xlSequence = '5[SQ001].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace(']',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('24.png')

plt.figure(25)
xlQuest = '[When interrupted, were you disappointed that the game was over?]'
xlSequence = '6[SQ001].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace(']',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('25.png')

plt.figure(26)
xlQuest = '[Would you like to play the game again?]'
xlSequence = '7[SQ001].  ' #be sure to include the . and two spaces
plot0 = grouped[[xlKeys[0]+xlSequence+xlQuest, 
                 xlKeys[1]+xlSequence+xlQuest,
                 xlKeys[2]+xlSequence+xlQuest]].T.plot(kind="bar", title=xlQuest.replace('[','').replace(']',''))
plot0.set_xticklabels( ('Tetris', 'DoomA','DoomB'))
plt.tight_layout()
plt.savefig('26.png')


# -------------------------------------------

plt.figure(10)
xlQuest = 'Background1. On average, how frequently do you play video games? '
plotdata = df[[xlQuest]].replace(r'\D', '', regex=True).groupby(xlQuest).size().rename(index={'1': 'Daily','2': 'A few times a week', '3':'A few times a month', '4':'Rarely', '5':"Never"})
plot0 = plotdata.plot.pie(y=xlQuest,autopct='%1.1f%%', title=xlQuest.replace('Background1. ',''))
plt.tight_layout()
plt.savefig('10.png')

plt.figure(11)
xlQuest = 'Background2. Have you previously played any first-person shooter style games (on a phone, pc, tv, game console, etc.)? '
plot0 = df[[xlQuest]].replace(r'\D', '', regex=True).groupby(xlQuest).size().plot.pie(y=xlQuest,labels=['Yes','No'],autopct='%1.1f%%', title=xlQuest.replace('Background2. ',''))
plt.tight_layout()
plt.savefig('11.png')

plt.figure(12)
xlQuest = 'Background3. Have you previously played any puzzle video games (on a phone, pc, tv, game console, etc.)? '
plot0 = df[[xlQuest]].replace(r'\D', '', regex=True).groupby(xlQuest).size().plot.pie(y=xlQuest,labels=['Yes','No'],autopct='%1.1f%%', title=xlQuest.replace('Background3. ',''))
plt.tight_layout()
plt.savefig('12.png')

plt.figure(13)
xlQuest = 'Background4. Have you previously played the first-person shooter game DOOM (on a phone, pc, tv, game console, etc.)?'
plot0 = df[[xlQuest]].replace(r'\D', '', regex=True).groupby(xlQuest).size().plot.pie(y=xlQuest,labels=['Yes','No'],autopct='%1.1f%%', title=xlQuest.replace('Background4. ',''))
plt.tight_layout()
plt.savefig('13.png')

plt.figure(14)
xlQuest = 'Background5.  Have you previously played the video game Tetris (on a phone, pc, tv, game console, etc.)?'
plot0 = df[[xlQuest]].replace(r'\D', '', regex=True).groupby(xlQuest).size().plot.pie(y=xlQuest,labels=['Yes','No'],autopct='%1.1f%%', title=xlQuest.replace('Background5. ',''))
plt.tight_layout()
plt.savefig('14.png')

plt.show()
