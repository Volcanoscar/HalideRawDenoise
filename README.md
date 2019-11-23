# AI Bayer Noise Reduction with Halide

## System data flow:
    sensor -> CSID -> IFE   -> [preview]
                   |
                   ->  RDI  -> [][][][][][] (6 frames in ZSL buffer)
                                    |
                                    V           <- SNAPSHOT request! 
                                AI denoise   
                                    |
                                    V
                             processed raw buffer
                                    |
          JPEG <-  IPE   <-  BPS   <-


## Build and run on Linux machine

1. pre-requisite:
  In the linux machine, install halide lib at /etc/halide.

1.build project using makefile under project folder:
>> make

1. how to run
  build.sh contains path to process the image
>> source build.sh
>> abnr run linux hand

