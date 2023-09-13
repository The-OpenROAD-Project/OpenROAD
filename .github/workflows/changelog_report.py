with open("../../NEWS.md") as f:
    temp = f.readlines()

# divide the file into four categories
commit, name, date, msg = [], [], [], []

# regex is <commit>\t<name>\t<date>\t<msg>
for line in temp:
    line = line.split('\t')
    commit.append(line[0])
    name.append(line[1])
    date.append(line[2])
    msg.append(line[3])

# first detect the number of unique year-month combo
date_year_month = [x[:7] for x in date]
unique = sorted(list(set(date_year_month)), reverse=True)

# based on this write from the reverse order
final_lines = []

for year_month in unique:
    # first write the header
    final_lines.append(f"# {year_month}\n")
    
    # loop through and stop when the year_month is lesser 
    for idx in range(len(date)):
        if date[idx][:7] == year_month:
            l = f"- {commit[idx]} {name[idx]} {date[idx]} {msg[idx]}"
            final_lines.append(l)
  
with open('../../NEWS.md', 'w') as f:
    for l in final_lines:
        f.write(l)

