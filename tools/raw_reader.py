import numpy as np
import matplotlib.pyplot as plt
import scipy.misc
from PIL import Image
from PIL import ImageEnhance

def ReadBayerRaw(src_fname, in_width, in_height):
    in_length = in_width * in_height
    raw_buffer = np.fromfile(src_fname, dtype=np.uint16, count=in_length)
    return np.reshape(raw_buffer, (in_height, in_width))

def ImShow(img, t, block=False):
    plt.figure()
    plt.imshow(img, cmap='gray')
    plt.title(t)
    plt.show(block)

def ImCompare(img1, t1, img2, t2):
    fig = plt.figure()

    ax1 = fig.add_subplot(1, 2, 1)
    ax1.imshow(img1, cmap='gray')
    plt.title('Reference image')

    ax2 = fig.add_subplot(1, 2, 2)
    ax2.imshow(img2, cmap='gray')
    plt.title('Compare image')

    plt.show(block=False)

def InterpolateGChannel(bayer_raw):
    (in_height,in_width) = bayer_raw.shape
    g_interleaved = bayer_raw

    g_interleaved[0:in_height:2, 1:in_width:2] = 0

    g_interleaved[2:in_height-2:2, 3:in_width-2:2] =  \
        (g_interleaved[1:in_height-3:2, 3:in_width-2:2] + 
        g_interleaved[3:in_height-1:2, 3:in_width-2:2] +
        g_interleaved[2:in_height-2:2, 2:in_width-3:2] +
        g_interleaved[2:in_height-2:2, 4:in_width-1:2]) / 4 

    g_interleaved[1:in_height:2, 0:in_width:2] = 0

    g_interleaved[3:in_height-2:2, 2:in_width-2:2] = \
        (g_interleaved[2:in_height-3:2, 2:in_width-2:2] +
        g_interleaved[4:in_height-1:2, 2:in_width-2:2] +
        g_interleaved[3:in_height-2:2, 1:in_width-3:2] +
        g_interleaved[3:in_height-2:2, 3:in_width-1:2]) / 4 
    return g_interleaved

def EnhanceBrightness(img, ratio):
    enhancer = ImageEnhance.Brightness(img)
    return enhancer.enhance(ratio)

def EnhanceContrast(img, ratio):
    enhancer = ImageEnhance.Contrast(img)
    return enhancer.enhance(ratio) 

def BayerRawToPng(bayer_raw, dst_fname, brightness_ratio = 3.0, contrast_ratio = 1.5, max_val = 1024):
    # scipy.misc.toimage(bayer_raw, cmin=0.0, cmax=1023.0).save(dst_fname)    
    out_raw = bayer_raw / (max_val/255);
    out_8bit = out_raw.astype(np.uint8);
    img = Image.fromarray(out_8bit);

    img_save = EnhanceContrast(EnhanceBrightness(img, brightness_ratio), contrast_ratio)
    img_save.save(dst_fname);

def BayerRawToGInterpPng(bayer_raw, dst_fname):
    g_raw = InterpolateGChannel(bayer_raw);
    BayerRawToPng(g_raw, dst_fname)

