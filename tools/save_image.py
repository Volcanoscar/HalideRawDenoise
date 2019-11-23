import sys, getopt
import matplotlib.pyplot as plt
import raw_reader as raw_reader 

def main(argv):
    inputfile = ''
    output_bayer = ''
    width = 0
    height = 0
    try:
        opts, args = getopt.getopt(argv,'i:w:h:v:',["ifile=","width=","height="])
    except getopt.GetoptError:
        print 'save_image.py -i <inputfile> -w <width> -h <height>'
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-v':
            print 'show_image.py -i <inputfile> -o <output_bayer>'
            sys.exit()
        elif opt in ("-i", "--ifile"):
            inputfile = arg
        elif opt in ("-w", "--width"):
            width = int(arg)
        elif opt in ("-h", "--height"):
            height = int(arg)

    output_bayer = inputfile[0:inputfile.find('.raw')] + '.png'
    # output_g = inputfile[0:inputfile.find('.raw')] + '_g_interp.png'

    print 'Input file is ', inputfile
    print 'Width is ', width
    print 'Height is ', height
    print 'Output PNG is ', output_bayer

    ref_origin = raw_reader.ReadBayerRaw(inputfile, width, height)
    
    raw_reader.BayerRawToPng(ref_origin, output_bayer);
    # raw_reader.BayerRawToGInterpPng(ref_origin, output_g);

if __name__ == "__main__":
    main(sys.argv[1:])

