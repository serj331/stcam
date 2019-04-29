#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <jpeglib.h>
#include <pthread.h>

libusb_context* lu_ctx = NULL;
libusb_device_handle * stcam_handle = NULL;

const int STCAM_FRAME_W = 208, STCAM_FRAME_H = 156; 
const int STCAM_FRAME_BSIZE = STCAM_FRAME_W * STCAM_FRAME_H * 2;
const int STCAM_TPXMAX = 16384, STCAM_TPXMIN = 2000;

volatile uint8_t* jpegframe = NULL;
volatile uint32_t jpegframe_size = 0;
volatile uint32_t jpegframe_num = 0;

volatile pthread_mutex_t jpegframe_lock;
volatile pthread_cond_t jpegframe_wait_v;

typedef enum {
         BEGIN_MEMORY_WRITE              = 82,
         COMPLETE_MEMORY_WRITE           = 81,
         GET_BIT_DATA                    = 59,
         GET_CURRENT_COMMAND_ARRAY       = 68,
         GET_DATA_PAGE                   = 65,
         GET_DEFAULT_COMMAND_ARRAY       = 71,
         GET_ERROR_CODE                  = 53,
         GET_FACTORY_SETTINGS            = 88,
         GET_FIRMWARE_INFO               = 78,
         GET_IMAGE_PROCESSING_MODE       = 63,
         GET_OPERATION_MODE              = 61,
         GET_RDAC_ARRAY                  = 77,
         GET_SHUTTER_POLARITY            = 57,
         GET_VDAC_ARRAY                  = 74,
         READ_CHIP_ID                    = 54,
         RESET_DEVICE                    = 89,
         SET_BIT_DATA_OFFSET             = 58,
         SET_CURRENT_COMMAND_ARRAY       = 67,
         SET_CURRENT_COMMAND_ARRAY_SIZE  = 66,
         SET_DATA_PAGE                   = 64,
         SET_DEFAULT_COMMAND_ARRAY       = 70,
         SET_DEFAULT_COMMAND_ARRAY_SIZE  = 69,
         SET_FACTORY_SETTINGS            = 87,
         SET_FACTORY_SETTINGS_FEATURES   = 86,
         SET_FIRMWARE_INFO_FEATURES      = 85,
         SET_IMAGE_PROCESSING_MODE       = 62,
         SET_OPERATION_MODE              = 60,
         SET_RDAC_ARRAY                  = 76,
         SET_RDAC_ARRAY_OFFSET_AND_ITEMS = 75,
         SET_SHUTTER_POLARITY            = 56,
         SET_VDAC_ARRAY                  = 73,
         SET_VDAC_ARRAY_OFFSET_AND_ITEMS = 72,
         START_GET_IMAGE_TRANSFER        = 83,
         TARGET_PLATFORM                 = 84,
         TOGGLE_SHUTTER                  = 55,
         UPLOAD_FIRMWARE_ROW_SIZE        = 79,
         WRITE_MEMORY_DATA               = 80,
} STCamCtlReq;


int stcam_devopen() {
    stcam_handle = libusb_open_device_with_vid_pid(lu_ctx, 0x289d, 0x0010);
    if(stcam_handle == NULL) {
        printf("stcam_devopen error: open_device_with_vid_pid failed.\n");
        return -1;
    }
    if(libusb_kernel_driver_active(stcam_handle, 0) == 1) {
        libusb_detach_kernel_driver(stcam_handle, 0);
    }
    libusb_claim_interface(stcam_handle, 0);
    return 0;
}

void stcam_devclose() {
    libusb_release_interface(stcam_handle, 0);
    libusb_close(stcam_handle);
}

int stcam_getframe(uint16_t* fb) {
    int bt = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, START_GET_IMAGE_TRANSFER, 0, 0, (uint8_t*)"\xC0\x7E\x00\x00", 4, 1000);
    if(bt != 4) {
        printf("stcam_readframe control xfer error: %s\n", libusb_strerror((libusb_error)bt));
        return -1;
    }
    int total_n = 0;

    uint8_t* fbU8 = (uint8_t*)fb;
    
    while(total_n < STCAM_FRAME_BSIZE) {
        int xfer_n;
        int err = libusb_bulk_transfer(stcam_handle, 0x81, &fbU8[total_n], 512, &xfer_n, 1000);
        if(err != 0) {
            printf("stcam_readframe bulk xfer error: %s\n", libusb_strerror((libusb_error)err));
            return -1;
        }
        total_n+=xfer_n;
    }
    return 0;
}

int stcam_deinit() {
    int nb;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_OPERATION_MODE, 0, 0, (uint8_t*)"\x00\x00", 2, 1000);
    if(nb!=2) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_OPERATION_MODE, 0, 0, (uint8_t*)"\x00\x00", 2, 1000);
    if(nb!=2) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_OPERATION_MODE, 0, 0, (uint8_t*)"\x00\x00", 2, 1000);
    if(nb!=2) return -1;
    return 0;
}

int stcam_init() {
    if(stcam_deinit()!=0) {
        return -1;
    }
    int nb;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, TARGET_PLATFORM, 0, 0, (uint8_t*)"\x01", 1, 1000);
    if(nb!=1) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_OPERATION_MODE, 0, 0, (uint8_t*)"\x00\x00", 2, 1000);
    if(nb!=2) return -1;
    uint8_t data[4];
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, GET_FIRMWARE_INFO, 0, 0, data, 4, 1000);
    if(nb!=4) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_FACTORY_SETTINGS_FEATURES, 0, 0, (uint8_t*)"\x20\x00\x30\x00\x00\x00", 6, 1000);
    if(nb!=6) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_FACTORY_SETTINGS_FEATURES, 0, 0, (uint8_t*)"\x20\x00\x50\x00\x00\x00", 6, 1000);
    if(nb!=6) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_FACTORY_SETTINGS_FEATURES, 0, 0, (uint8_t*)"\x0C\x00\x70\x00\x00\x00", 6, 1000);
    if(nb!=6) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_FACTORY_SETTINGS_FEATURES, 0, 0, (uint8_t*)"\x06\x00\x08\x00\x00\x00", 6, 1000);
    if(nb!=6) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_IMAGE_PROCESSING_MODE, 0, 0, (uint8_t*)"\x08\x00", 2, 1000);
    if(nb!=2) return -1;
    nb = libusb_control_transfer(stcam_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, SET_OPERATION_MODE, 0, 0, (uint8_t*)"\x01\x00", 2, 1000); 
    if(nb!=2) return -1;
    return 0;
}

//void rgb_to_yuv(uint8_t r, uint8_t g, uint8_t b, uint8_t* y, uint8_t* u, uint8_t* v) {
//    *y = (uint8_t)(16.0 + ((65.738 * (double)r) / 256.0) + ((129.057 * (double)g) / 256.0) + ((25.064 * (double)b) / 256.0));
//    *u = (uint8_t)(128.0 + ((-37.945 * (double)r) / 256.0) - ((74.494 * (double)g) / 256.0) + ((112.439 * (double)b) / 256.0));
//    *v = (uint8_t)(128.0 + ((112.439 * (double)r) / 256.0) - ((94.154 * (double)g) / 256.0) - ((18.285 * (double)b) / 256.0));
//}
//
//static int xioctl(int fd, int request, void *arg){
//    int r;
//    do { 
//        r = ioctl(fd, request, arg);
//    } while(-1 == r && EINTR == errno);
//    return r;
//}
//
//#define VCAM_FRAME_W 206
//#define VCAM_FRAME_H 156
//#define VCAM_FRAME_BSIZE (VCAM_FRAME_W * VCAM_FRAME_H * 2)
//
//volatile int vcam_fd = -1;
//
//int vcam_devopen() {
//    vcam_fd = open("/dev/video0", O_RDWR);
//    if(vcam_fd == -1) {
//        perror("vcam_open error");
//        return -1;
//    }
//    return 0;
//}
//
//void vcam_devclose() {
//    close(vcam_fd);
//}
//
//int vcam_setfmt() {
//    struct v4l2_capability caps = {0};
//    if(xioctl(vcam_fd, VIDIOC_QUERYCAP, &caps) == -1) {
//        perror("vcam_setfmt {VIDIOC_QUERYCAP} error");
//        return -1;
//    }
//    struct v4l2_format fmt = {0};
//    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
//    fmt.fmt.pix.width = VCAM_FRAME_W;
//    fmt.fmt.pix.height = VCAM_FRAME_H;
////    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
//    fmt.fmt.pix.field = V4L2_FIELD_NONE;
////	fmt.fmt.pix.sizeimage = VCAM_FRAME_BSIZE;
////	fmt.fmt.pix.bytesperline = VCAM_FRAME_W * 2;
//    fmt.fmt.pix.sizeimage = 0;
//    fmt.fmt.pix.bytesperline = 0;
////    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
//    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
//    if(xioctl(vcam_fd, VIDIOC_S_FMT, &fmt) == -1) {
//        perror("vcam_setfmt {VIDIOC_S_FMT} error");
//        return -1;
//    }
//    return 0;
//}

extern const uint8_t iron_pallet_pixel_map[];

extern "C" {
    int HTTPStreamer_start(int port);
}

int main(int argc, char **argv)
{
    struct sigaction sa = {SIG_IGN};
    sigaction(SIGPIPE, &sa, NULL);

    pthread_mutex_init((pthread_mutex_t*)&jpegframe_lock, NULL);

    int err = libusb_init(&lu_ctx);
    if(err != 0) {
        printf("libusb_init error: %s", libusb_strerror((libusb_error)err));
        return EXIT_FAILURE;
    }
    
//    if(vcam_devopen() == -1) {
//        return EXIT_FAILURE;
//    }
//    
//    printf("v4l2loopback opened.\n");
//    
//    if(vcam_setfmt() == -1) {
//        return EXIT_FAILURE;
//    }

    if(stcam_devopen() != 0) {
        return EXIT_FAILURE;
    }
    
    if(stcam_init() != 0) {
        return EXIT_FAILURE;
    }
    
    printf("seek thermal started..\n");
    
    jpegframe_size = 1024000;
    jpegframe = (uint8_t*)malloc(jpegframe_size);

    if(jpegframe == NULL) {
        perror("cannot alocate mem for jpegframe buf");
        return EXIT_FAILURE;
    }
    
    if(HTTPStreamer_start(80) != 0) {
        puts("failed start mjpg stream.");
        return EXIT_FAILURE;
    }
    
    uint16_t stframebuf[STCAM_FRAME_W * STCAM_FRAME_H];
    uint16_t stcalframe[STCAM_FRAME_W * STCAM_FRAME_H];
    double stgaincal[STCAM_FRAME_W * STCAM_FRAME_H];

    while(1) {
        if(stcam_getframe(stframebuf) != 0) {
            continue; //?
//            return EXIT_FAILURE;
        }
        
        uint8_t frameid = stframebuf[10] & 0xFF;
        
        if(frameid == 4) {
            uint16_t* stgainframe = stframebuf;

            uint16_t tmpfb[STCAM_FRAME_W * STCAM_FRAME_H];
            memcpy(tmpfb, stgainframe, STCAM_FRAME_BSIZE);
            
            std::sort(&tmpfb[0], &tmpfb[(STCAM_FRAME_W * STCAM_FRAME_H) - 1], std::less<uint16_t>());
            double med = tmpfb[(STCAM_FRAME_W * STCAM_FRAME_H) / 2];

            for(int y = 0; y < STCAM_FRAME_H; y++) {
                for(int x = 0; x < (STCAM_FRAME_W - 2); x++) {
                    int i = y * STCAM_FRAME_W + x;
                    if(stgainframe[i] >= STCAM_TPXMIN && stgainframe[i] <= STCAM_TPXMAX) {
                        stgaincal[i] =  med / (double)stgainframe[i];
                    } else {
                        stgaincal[i] = 1.0;
                    }
                }
            }
            
            printf("ID4 med=%d\n", (int)med);
        } else if(frameid == 1) {
            
            memcpy(stcalframe, stframebuf, STCAM_FRAME_BSIZE);
            
        } else if(frameid == 3) {
            uint16_t* thermframe = stframebuf;
            
            for(int y = 0; y < STCAM_FRAME_H; y++) {
                for(int x = 0; x < (STCAM_FRAME_W - 2); x++) {
                    int i = y * STCAM_FRAME_W + x;
                    if(thermframe[i] >= STCAM_TPXMIN && thermframe[i] <= STCAM_TPXMAX) {
                        thermframe[i] = (uint16_t)((double)(thermframe[i] - stcalframe[i]) * stgaincal[i] + (STCAM_TPXMAX / 2));
                    } else {
                        thermframe[i] = 0;
                    }
                }
            }
            
            thermframe[20] = 0;
            thermframe[40] = 0;

            //fix bad pxels
            for(int y = 0; y < STCAM_FRAME_H; y++) {
                for(int x = 0; x < (STCAM_FRAME_W - 2); x++) {
                    int i = y * STCAM_FRAME_W + x; 
                    if(thermframe[i] == 0) {  //fix bad pixel
                        int sum = 0, n = 0;
                        uint16_t tpxv = 0;
                        if(x > 0) {
                            tpxv = thermframe[(y) * STCAM_FRAME_W + (x - 1)]; //left
                            if(tpxv != 0) {
                                sum += tpxv;
                                n++;
                            }
                        }
                        if(y > 0) {
                            tpxv = thermframe[(y - 1) * STCAM_FRAME_W + (x)]; //up
                            if(tpxv != 0) {
                                sum += tpxv;
                                n++;
                            }
                        }
                        if(x < (STCAM_FRAME_W - 2) - 1) {
                            tpxv = thermframe[(y) * STCAM_FRAME_W + (x + 1)]; //right
                            if(tpxv != 0) {
                                sum += tpxv;
                                n++;
                            }
                        }
                        if(y < STCAM_FRAME_H - 1) {
                            tpxv = thermframe[(y + 1) * STCAM_FRAME_W + (x)]; //down
                            if(tpxv != 0) {
                                sum += tpxv;
                                n++;
                            }
                        }
                        if(n > 0) {
                            thermframe[i] = sum / n;
                        }
                    }
                }  
            }

            cv::Mat m0raw(STCAM_FRAME_H, STCAM_FRAME_W - 2, CV_16UC1, thermframe, STCAM_FRAME_W * 2);
            
            cv::Mat m0;
            
            cv::medianBlur(m0raw, m0, 3);

            double tpx_max = 0, tpx_min = 0;
            
            cv::minMaxLoc(m0, &tpx_min, &tpx_max);
            
            tpx_max+=1;

            cv::Mat m1(STCAM_FRAME_H, STCAM_FRAME_W - 2, CV_8UC3);

            for(int y = 0; y < STCAM_FRAME_H; y++) {
                uint16_t* m0rp = (uint16_t*)m0.ptr(y);
                uint8_t* m1rp = (uint8_t*)m1.ptr(y);
                for(int x = 0; x < STCAM_FRAME_W - 2; x++) {
                    int pal_i = ((m0rp[x] - (int)tpx_min) * 1000) / ((int)tpx_max - (int)tpx_min);
                    m1rp[x * 3 + 0] = iron_pallet_pixel_map[pal_i * 4 + 2]; //r
                    m1rp[x * 3 + 1] = iron_pallet_pixel_map[pal_i * 4 + 1]; //g
                    m1rp[x * 3 + 2] = iron_pallet_pixel_map[pal_i * 4 + 0]; //b
                }
            }
            
            cv::Mat m2;
            
            cv::bilateralFilter(m1, m2, 5, 10, 10 );
            
//            cv::Mat m3(VCAM_FRAME_H, VCAM_FRAME_W, CV_8UC3);
//            
//            cv::resize(m2, m3, m3.size(), 0, 0, cv::INTER_CUBIC);

//            cv::Mat m4;
            
//            cv::cvtColor(m3, m4, CV_BGR2YUV);
//            cv::cvtColor(m2, m4, CV_BGR2YUV);

            pthread_mutex_lock((pthread_mutex_t*)&jpegframe_lock);
            
            struct jpeg_compress_struct cinfo;
            struct jpeg_error_mgr jerr;
            
            JSAMPROW row_pointer[1];
            
            cinfo.err = jpeg_std_error(&jerr);
            jpeg_create_compress(&cinfo);

            jpegframe_size = 1024000;
            jpeg_mem_dest(&cinfo, (uint8_t**)&jpegframe, (long unsigned int*)&jpegframe_size);
            
            cinfo.image_width = STCAM_FRAME_W - 2;
            cinfo.image_height = STCAM_FRAME_H;
            cinfo.input_components = 3;	
            cinfo.in_color_space = JCS_RGB; 
    
            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, 100, TRUE);

            jpeg_start_compress(&cinfo, TRUE);
            
            while(cinfo.next_scanline < cinfo.image_height) {
                uint8_t* m2rp = (uint8_t*)m2.ptr(cinfo.next_scanline);
                row_pointer[0] = m2rp;
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }
            
            jpeg_finish_compress(&cinfo);
            jpeg_destroy_compress(&cinfo);

            jpegframe_num++;
            pthread_cond_broadcast((pthread_cond_t*)&jpegframe_wait_v); //here ???
            pthread_mutex_unlock((pthread_mutex_t*)&jpegframe_lock);
            
            
            
            

//            int n = write(vcam_fd, imgjpgc, imgjpgc_size);
//            if(n != (int)imgjpgc_size) {
//                perror("vcam write error");
//                return EXIT_FAILURE;
//            }

            //--------------------------------------------------------------------------------
    
//            for(int y = 0; y < m4.rows; y++) {
//                uint8_t* m4_rp = m4.ptr(y);
//                for(int x = 0; x < m4.cols; x+=2) {
//                    uint8_t cy0 = m4_rp[(x + 0) * 3 + 0]; //y0 -
//                    uint8_t cu0 = m4_rp[(x + 0) * 3 + 1]; //u0 -
//                    uint8_t cv0 = m4_rp[(x + 0) * 3 + 2]; //v0 -
//                    uint8_t cy1 = m4_rp[(x + 1) * 3 + 0]; //y1 -
//                    vcfbuf[(y * VCAM_FRAME_W * 2) + (x * 2) + 0] = cy0; //y
//                    vcfbuf[(y * VCAM_FRAME_W * 2) + (x * 2) + 1] = cu0; //u
//                    vcfbuf[(y * VCAM_FRAME_W * 2) + (x * 2) + 2] = cy1; //y
//                    vcfbuf[(y * VCAM_FRAME_W * 2) + (x * 2) + 3] = cv0; //v                    
//                }
//            }
//            
//            int n = write(vcam_fd, vcfbuf, VCAM_FRAME_BSIZE);
//            if(n != VCAM_FRAME_BSIZE) {
//                perror("vcam write error");
//                return EXIT_FAILURE;
//            }
        }
    }  
	return 0;
}
