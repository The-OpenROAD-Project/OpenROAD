
import sys
import re
import json

def main():
    args = sys.argv[1:]
    rpt_cmp(args[0], args[1])
    print('done')

def rpt_cmp(offical_rpt, my_rpt):

    pattern = "(PAR|CAR):\s*([0-9]*\.[0-9]*)"
    rpt = open(offical_rpt, "r")
    ar_list = []
    net_dict = {}
    layer_dict = {}
    
    innovus_net = set()
    checker_net = set()
    
    net_name = ""
    count = 0
    while True:
        line = rpt.readline()
        if not line or re.match("Total", line):
            break
    
        if line == '' or line[0] == '#':
            continue
    
        try:
            if line[0] != ' ' and line[0] != '[':
                if count != 0:
                    net_dict['layers'].append(layer_dict)

                    ar_list.append(net_dict)
                    innovus_net.add(net_name)
                net_dict = {}
                net_name = line.split( )[0]
                net_dict['net'] = net_name            
                net_dict['layers'] = []
                layer_name = ''
            elif line[0] == '[':
                if layer_name != '':
                    net_dict['layers'].append(layer_dict)
                layer_name = line.split()[1]
                layer_dict['layer'] = layer_name
            else:
                parm = re.search(pattern, line)
                if parm:
                    layer_dict[parm.group(1)] = parm.group(2)
    
            count += 1
        except:
            continue
    
    rpt.close()
    
    
    pattern = ".*\s+([a-zA-Z]*):\s*([0-9]*.[0-9]*\*)"

    myrpt = open(my_rpt, "r+")
    done = 0
    my_ar_list = []
    net_dict = {}
    layer_dict = {}
    net_name = ""
    count = 0
    while True:
        line = myrpt.readline()
        if not line:
            break;
        
        count += 1
        if line == '' or line[0] == '#':
            continue
        
        try:
            if(line.find('Net') != -1):
                if len(layer_dict.keys()) > 1:
                    net_dict['layers'].append(layer_dict)
                    checker_net.add(net_name)
                
                if net_dict:
                    my_ar_list.append(net_dict)
                    net_dict = {}
                    layer_dict = {}
                
                net_name = line.split( )[2]
                net_name = net_name.replace('\\', '') 
                net_dict['net'] = net_name
                net_dict['layers'] = []
            elif(line[0] == '['):
                if len(layer_dict.keys()) > 1:
                    net_dict['layers'].append(layer_dict)
                    checker_net.add(net_name)
                    layer_dict = {}                    
                layer_dict['name'] = line.split( )[1]
            else:
                parm = re.match(pattern, line)

                if parm:
                    layer_dict[parm.group(1)] = parm.group(2)
        except:
            print(count)
            continue
    myrpt.close()

    same_nets = innovus_net.intersection(checker_net)
    in_netsonly = innovus_net - same_nets
    ch_netsonly = checker_net - same_nets

    print("innovus total: {}".format(len(innovus_net)))
    print("checker total: {}".format(len(checker_net)))

    print("correct dectections: {}".format(len(same_nets)))

    print("innovus only violations: {}".format(len(in_netsonly)))

    print("checker only violations: {}".format(len(ch_netsonly)))
    chkrslt = open("cmprslt.txt", "w+")


    for net_name in same_nets:
        chkrslt.write(net_name+'\n')
        for ar_info in ar_list:
            if ar_info['net'] == net_name:
                chkrslt.write(json.dumps(ar_info['layers']))
                chkrslt.write('\n')
        for my_ar_info in my_ar_list:
            if my_ar_info['net'] == net_name:
                chkrslt.write(json.dumps(my_ar_info['layers']))
                chkrslt.write('\n')

    chkrslt.close()

    with open("in_netsonly.txt", "w") as f:
        for net_name in in_netsonly:
            f.write(net_name+'\n')
            for ar_info in ar_list:
                if ar_info['net'] == net_name:
                    f.write(json.dumps(ar_info['layers']))
                    f.write('\n')                
    
    with open("ch_netsonly.txt", "w") as f:
        for net_name in ch_netsonly:
            f.write(net_name+'\n')
            for my_ar_info in my_ar_list:
               if my_ar_info['net'] == net_name:
                    f.write(json.dumps(my_ar_info['layers']))                
                    f.write('\n')
                        
if __name__ == "__main__":
    main()                   
            

