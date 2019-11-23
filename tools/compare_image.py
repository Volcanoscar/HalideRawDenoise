import sys, getopt
import matplotlib.pyplot as plt
import raw_reader as raw_reader 

def main(argv):
    inputfile = ''
    outputfile = ''
    width = 0
    height = 0
    try:
        opts, args = getopt.getopt(argv,'i:w:h:v:x:o:y:p:g:c:',["ifile=","width=","height="])
    except getopt.GetoptError:
        print 'test.py -i <inputfile> -w <width> -h <height>'
        sys.exit(2)
    xstart = 0
    xend = 0
    ystart = 0
    yend = 0
    gain = 0.0
    for opt, arg in opts:
        if opt == '-v':
            print 'show_image.py -i <inputfile> -o <outputfile>'
            sys.exit()
        elif opt in ("-i", "--ifile"):
            inputfile = arg
        elif opt in ("-w", "--width"):
            width = int(arg)
        elif opt in ("-h", "--height"):
            height = int(arg)
        elif opt in ("-x", "--xstart"):
            xstart = int(arg)
        elif opt in ("-o", "--xend"):
            xend = int(arg)
        elif opt in ("-y", "--ystart"):
            ystart = int(arg)
        elif opt in ("-p", "--yend"):
            yend = int(arg)
        elif opt in ("-g", "--gain"):
            gain = float(arg.strip().strip("'"))
        elif opt in ("-c", "--compare"):
            comparefile = arg

    print 'Reference file is ', inputfile
    print 'compare file is ', comparefile
    print 'Width is ', width
    print 'Height is ', height
    print 'xstart is ', xstart
    print 'xend is ', xend
    print 'ystart is ', ystart
    print 'yend is ', yend 
    print 'gain is ', arg 

    ref_origin = raw_reader.ReadBayerRaw(inputfile, width, height)
    comp_origin = raw_reader.ReadBayerRaw(comparefile, width, height)

    if xstart == 0:
        ref_image = ref_origin
        comp_image = comp_origin
    else:
        ref_image = ref_origin[ystart:yend, xstart:xend]
        comp_image = comp_origin[ystart:yend, xstart:xend]
    
    raw_reader.ImCompare(ref_image, "Reference Image", comp_image, "Compare Image")

    plt.figure()
    plt.show(block=True)

if __name__ == "__main__":
    main(sys.argv[1:])

