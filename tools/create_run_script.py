#! /usr/bin/python3
# Usage example: generate_test_file.py -i run_lightbox.txt 

import sys, getopt

def main(argv):
    inputfile = ''
    cmdfile = ''
    cfgfile = ''
    try:
        opts, args = getopt.getopt(argv,'i:',["ifile="])
     
    except getopt.GetoptError:
        print('generate_test_file.py -i <inputfile>');
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-h':
            print('generate_test_file.py -i <inputfile>');
            sys.exit()
        elif opt in ("-i", "--in"):
            inputfile = arg
         

    #print('Input file is ' + inputfile)
    
    cmdfile = inputfile[0:inputfile.find('.txt')] + '.py'
    cfgfile = inputfile[0:inputfile.find('.txt')] + '.cfg'
    fcmd = open(cmdfile,"w")
    fcfg = open(cfgfile,"w")
    fp = open(inputfile)
    line = fp.readline()
    
    # Form python header 
    command = '#! /usr/bin/python3\n'
    fcmd.write(command)
    command = 'from subprocess import check_output \n\n'
    fcmd.write(command)

    command = 'from subprocess import call\n\n'
    fcmd.write(command)
    
    # Form function to push raw image
    command = 'def push_img(src, dest, file):\n'
    fcmd.write(command)        

    command = "    temp = src + '/' + file;\n"
    fcmd.write(command)
    
    command = "    rc = check_output(['adb', 'shell', 'ls', '-l', dest, '|', 'grep', file])\n"
    fcmd.write(command)

    command = '    if rc:\n'
    fcmd.write(command)

    command = "        print('File Exists in device: ' + file) \n"
    fcmd.write(command)
    
    command = '    else:\n'
    fcmd.write(command)

    command = '        print("pushing " + file )\n'
    fcmd.write(command)
    
    command = '        call(["adb", "push", temp, dest])\n'
    fcmd.write(command)
  
    command = '    return;\n\n'
    fcmd.write(command)
    
    while line:
        fcfg.write(line)
        if line.strip() == "InputDirectoryPath" :
            src = fp.readline()
            dest = src.replace("./data","/data/abnr")
            command = 'call(["adb", "shell", "mkdir", "-p","' + dest.strip() + '"])\n'
            fcmd.write(command)
            fcfg.write(dest)
        elif line.strip() == "InputNumOfRaws":
            line = fp.readline()
            fcfg.write(line)            
            numRows = int(line.strip())
            for i in range(0, numRows):
                line = fp.readline()
                file = fp.readline()
                fcfg.write(line)
                fcfg.write(file)
                command = "push_img('" + src.strip() + "','" + dest.strip() + "','" + file.strip() + "')\n"
                fcmd.write(command)
        elif line.strip() == "OutputDirectoryPath":
            out = fp.readline()
            dest = out.replace("./data","/data/abnr")
    
            #command = 'adb shell rm -rf ' + dest
            command = "call(['adb', 'shell', 'rm', '-rf','" + dest.strip() + "'])\n"
            fcmd.write(command) 
            #command = 'adb shell mkdir -p ' + dest
            command = "call(['adb', 'shell', 'mkdir', '-p','" + dest.strip() + "'])\n"
            fcmd.write(command)
            fcfg.write(dest)
        else:
            line = fp.readline()
            fcfg.write(line)
        line = fp.readline()

    #Run app on device    
    #command = 'adb shell rm /data/abnr/*\n'
    command = "call(['adb', 'shell', 'rm', '/data/abnr/*'" + "])\n"
    fcmd.write(command) 
    #command = 'adb push  ' + cfgfile + ' /data/abnr\n'
    command = "call(['adb', 'push', " + "'" + cfgfile + "', " + "'/data/abnr'" + "])\n"
    fcmd.write(command)
    #command = 'adb push  build/nr' + ' /data/abnr\n'
    command = "call(['adb', 'push', 'build/nr', '/data/abnr'" + "])\n"
    fcmd.write(command)    
    #command = 'adb shell chmod +x /data/abnr/nr\n'
    command = "call(['adb', 'shell', 'chmod', '+x', '/data/abnr/nr'" + "])\n"
    fcmd.write(command)    
    #command = 'adb shell /data/abnr/nr' + ' /data/abnr/' + cfgfile + '\n'
    command = "call(['adb', 'shell', '/data/abnr/nr', '/data/abnr/" + cfgfile + "'" + "])\n"
    fcmd.write(command)    
    #command = 'adb pull  ' + dest.strip() + '/nr_denoised.raw ' + out;
    command = "call(['adb', 'pull', " + "'" + dest.strip() + "'," + "'" + out.strip() + "'" + "])\n"
    fcmd.write(command)   

    fp.close()
    fcmd.close()
    fcfg.close()
    print('Generated test file: ' + cmdfile)

    

if __name__ == "__main__":
    main(sys.argv[1:])
