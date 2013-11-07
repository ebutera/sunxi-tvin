
//#加了点注释

//#Rockie Cheng

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include <getopt.h>            

#include <fcntl.h>             
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>         
#include <linux/videodev2.h>
#include <time.h>
#include <linux/fb.h>

#include <linux/input.h>

//#include "./../lichee/linux-2.6.36/include/linux/drv_display_sun4i.h"//modify this
#include "/home/enrico/dev/linux-sunxi-hans/include/video/sunxi_disp_ioctl.h"

#define DISPLAY
#define LCD_WIDTH	1280
#define LCD_HEIGHT	720

#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {
        void *                  start;
        size_t                  length;
};

struct size{
	int width;
	int height;
};

static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;

FILE *file_fd;
static unsigned long file_length;
static unsigned char *file_name;

int disphd;
unsigned int hlay;
int sel = 0;//which screen 0/1
__disp_layer_info_t layer_para;
__disp_video_fb_t video_fb;
__u32 arg[4];

struct size disp_size;
int  format;
__disp_pixel_fmt_t  disp_format;
__disp_pixel_mod_t  disp_mode;
__disp_pixel_seq_t	disp_seq;

static int quit = 0;

void handle_int(int n) {
	quit = 1;
}

//////////////////////////////////////////////////////
//获取一帧数据
//////////////////////////////////////////////////////
static int read_frame (void)//FIXME
{
	struct v4l2_buffer buf;
	unsigned int i;
	
	CLEAR (buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	ioctl (fd, VIDIOC_DQBUF, &buf); //出列采集的帧缓冲
	        
	assert (buf.index < n_buffers);
	disp_set_addr(disp_size.width, disp_size.height,&buf.m.offset);
	
	ioctl (fd, VIDIOC_QBUF, &buf); //再将其入列

	return 1;
}

int disp_init(int w,int h)
{
	if((disphd = open("/dev/disp",O_RDWR)) == -1)
	{
		printf("open file /dev/disp fail. \n");
		return 0;
	}

    arg[0] = 0;
    ioctl(disphd, DISP_CMD_HDMI_ON, (void*)arg);

    //layer0
    arg[0] = 0;
    arg[1] = DISP_LAYER_WORK_MODE_SCALER;
    hlay = ioctl(disphd, DISP_CMD_LAYER_REQUEST, (void*)arg);
    if(hlay == 0)
    {
        printf("request layer0 fail\n");
        return 0;
    }

    layer_para.mode = DISP_LAYER_WORK_MODE_SCALER; 
    layer_para.pipe = 0; 
    layer_para.fb.addr[0]       = 0;//your Y address,modify this 
    layer_para.fb.addr[1]       = 0; //your C address,modify this 
    layer_para.fb.addr[2]       = 0; 
    layer_para.fb.size.width    = w;
    layer_para.fb.size.height   = h;
    layer_para.fb.mode          = disp_mode;///DISP_MOD_INTERLEAVED;//DISP_MOD_NON_MB_PLANAR;//DISP_MOD_NON_MB_PLANAR;//DISP_MOD_NON_MB_UV_COMBINED;
    layer_para.fb.format        = disp_format;//DISP_FORMAT_YUV420;//DISP_FORMAT_YUV422;//DISP_FORMAT_YUV420;
    layer_para.fb.br_swap       = 0;
    layer_para.fb.seq           = disp_seq;//DISP_SEQ_UVUV;//DISP_SEQ_YUYV;//DISP_SEQ_YVYU;//DISP_SEQ_UYVY;//DISP_SEQ_VYUY//DISP_SEQ_UVUV
    layer_para.ck_enable        = 0;
    layer_para.alpha_en         = 1; 
    layer_para.alpha_val        = 0xff;
    layer_para.src_win.x        = 0;
    layer_para.src_win.y        = 0;
    layer_para.src_win.width    = w;
    layer_para.src_win.height   = h;
    layer_para.scn_win.x        = 0;
    layer_para.scn_win.y        = 0;
    layer_para.scn_win.width    = LCD_WIDTH;//800;
    layer_para.scn_win.height   = LCD_HEIGHT;//480;
	arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&layer_para;
    ioctl(disphd,DISP_CMD_LAYER_SET_PARA,(void*)arg);
#if 0
    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd,DISP_CMD_LAYER_TOP,(void*)arg);
#endif
    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd,DISP_CMD_LAYER_OPEN,(void*)arg);

#if 1
	int fb_fd;
	unsigned long fb_layer;
	void *addr = NULL;
	fb_fd = open("/dev/fb0", O_RDWR);
	if (ioctl(fb_fd, FBIOGET_LAYER_HDL_0, &fb_layer) == -1) {
		printf("get fb layer handel\n");	
	}
	
	close(fb_fd);

	arg[0] = 0;
	arg[1] = fb_layer;
	ioctl(disphd, DISP_CMD_LAYER_BOTTOM, (void *)arg);
#endif
}

void disp_start(void)
{
	arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_VIDEO_START,  (void*)arg);
}

void disp_stop(void)
{
	arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_VIDEO_STOP,  (void*)arg);
}

int disp_on()
{
	arg[0] = 0;
    ioctl(disphd, DISP_CMD_HDMI_ON, (void*)arg);
}

int disp_set_addr(int w,int h,int *addr)
{
#if 0	
	layer_para.fb.addr[0]       = *addr;//your Y address,modify this 
    layer_para.fb.addr[1]       = *addr+w*h; //your C address,modify this 
    layer_para.fb.addr[2]       = *addr+w*h*3/2; 
    
    arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&layer_para;
    ioctl(disphd,DISP_CMD_LAYER_SET_PARA,(void*)arg);
#endif
	__disp_video_fb_t  fb_addr;	
	memset(&fb_addr, 0, sizeof(__disp_video_fb_t));

	fb_addr.interlace       = 1;///////////////////////////////////////////////////////
	fb_addr.top_field_first = 0;
	fb_addr.frame_rate      = 25;
	fb_addr.addr[0] = *addr;
//	fb_addr.addr[1] = *addr + w * h;
//	fb_addr.addr[2] = *addr + w*h*3/2;
	
	
	switch(format){
		case V4L2_PIX_FMT_YUV422P:
	    case V4L2_PIX_FMT_YUYV:
	    case V4L2_PIX_FMT_YVYU:
	    case V4L2_PIX_FMT_UYVY:
	    case V4L2_PIX_FMT_VYUY:		
	    	fb_addr.addr[1]       = *addr+w*h; //your C address,modify this 
	    	fb_addr.addr[2]       = *addr+w*h*3/2; 
	    	break;
	    case V4L2_PIX_FMT_YUV420:
	    	fb_addr.addr[1]       = *addr+w*h; //your C address,modify this 
	    	fb_addr.addr[2]       = *addr+w*h*5/4;
	    	break;
	    case V4L2_PIX_FMT_NV16:
	    case V4L2_PIX_FMT_NV12:	
	    case V4L2_PIX_FMT_HM12:	
	    	fb_addr.addr[1]       = *addr+w*h; //your C address,modify this 
	    	fb_addr.addr[2]       = layer_para.fb.addr[1];
	    	break;
	    
	    default:
	    	printf("format is not found!\n");
	    	break;
    
  	}
  	
  	fb_addr.id = 0;  //TODO
  	
    arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&fb_addr;
    ioctl(disphd, DISP_CMD_VIDEO_SET_FB, (void*)arg);
}

int disp_exit()
{
	__u32 arg[4];
	arg[0] = 0;
    ioctl(disphd, DISP_CMD_HDMI_OFF, (void*)arg);

    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_LAYER_CLOSE,  (void*)arg);

    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_LAYER_RELEASE,  (void*)arg);
    close (disphd);
}

int main(int argc, char* argv[])
{
	int i;
    int key_fd = -1;
	struct input_event data;
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(sigact)); 

	sigact.sa_handler = handle_int;
	
	//format=V4L2_PIX_FMT_NV12; //V4L2_PIX_FMT_NV16
	format = V4L2_PIX_FMT_NV12;
	disp_format=DISP_FORMAT_YUV420; //DISP_FORMAT_YUV422
	disp_seq=DISP_SEQ_UVUV;
	struct v4l2_format fmt;
	struct v4l2_format fmt_priv;
	
    printf("*********************\n");
    printf("TVD demo start!\n");
    printf("Press KEY_ESC for exit\n");
    printf("*********************\n");


	/*if((key_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK)) < 0){//key: linux=event0, android=event2
    	printf("open event fail!\n");
		return -1;
	}*/
		
	fd = open ("/dev/video1", O_RDWR /* required */ | O_NONBLOCK, 0);
	
	
	int ret = -1;

	CLEAR (fmt_priv);
	fmt_priv.type                = V4L2_BUF_TYPE_PRIVATE;
	fmt_priv.fmt.raw_data[0] =0;//interface
	fmt_priv.fmt.raw_data[1] =0;//system
	fmt_priv.fmt.raw_data[2] =0;//format 1=mb, for test only
		
	fmt_priv.fmt.raw_data[8] =2;//row
	fmt_priv.fmt.raw_data[9] =2;//column
	
	fmt_priv.fmt.raw_data[10] =1;//channel_index
	fmt_priv.fmt.raw_data[11] =2;//channel_index
	fmt_priv.fmt.raw_data[12] =3;//channel_index
	fmt_priv.fmt.raw_data[13] =4;//channel_index
	if (-1 == ioctl (fd, VIDIOC_S_FMT, &fmt_priv)) //设置自定义
	{
		printf("VIDIOC_S_FMT error!  a\n");
		ret = -1;
		return ret; 
	}
	
//	CLEAR (fmt);
//	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//	fmt.fmt.pix.width       = 720*fmt_priv.fmt.raw_data[8];//720; 
//	fmt.fmt.pix.height      = 480*fmt_priv.fmt.raw_data[9];//576;//480;
//	fmt.fmt.pix.pixelformat = format;//V4L2_PIX_FMT_YUV422P;//V4L2_PIX_FMT_NV12;//V4L2_PIX_FMT_YUYV;
//	fmt.fmt.pix.field       = V4L2_FIELD_NONE;
//	if (-1 == ioctl (fd, VIDIOC_S_FMT, &fmt)) //设置图像格式
//	{
//		printf("VIDIOC_S_FMT error! b\n");
//		ret = -1;
//		return ret;
//	}
	
	disp_mode=fmt_priv.fmt.raw_data[2]?DISP_MOD_MB_UV_COMBINED:DISP_MOD_NON_MB_UV_COMBINED;//DISP_MOD_NON_MB_UV_COMBINED DISP_MOD_MB_UV_COMBINED
	disp_size.width = fmt_priv.fmt.raw_data[8]*(fmt_priv.fmt.raw_data[2]?704:720);//width
	disp_size.height = fmt_priv.fmt.raw_data[9]*(fmt_priv.fmt.raw_data[1]?576:480);//height
	printf("disp_size.width=%d\n", disp_size.width);
	printf("disp_size.height=%d\n", disp_size.height);
	
	usleep(100000);//delay 100ms if you want to check the status after set fmt
	
	CLEAR (fmt_priv);
	fmt_priv.type                = V4L2_BUF_TYPE_PRIVATE;
	if (-1 == ioctl (fd, VIDIOC_G_FMT, &fmt_priv)) //设置自定义
	{
		printf("VIDIOC_G_FMT error!  a\n");
		ret = -1;
		return ret; 
	}
	printf("interface=%d\n", fmt_priv.fmt.raw_data[0]);
	printf("system=%d\n", fmt_priv.fmt.raw_data[1]);
	printf("format=%d\n", fmt_priv.fmt.raw_data[2]);
	printf("row=%d\n", fmt_priv.fmt.raw_data[8]);
	printf("column=%d\n", fmt_priv.fmt.raw_data[9]);
	printf("channel_index[0]=%d\n", fmt_priv.fmt.raw_data[10]);
	printf("channel_index[1]=%d\n", fmt_priv.fmt.raw_data[11]);
	printf("channel_index[2]=%d\n", fmt_priv.fmt.raw_data[12]);
	printf("channel_index[3]=%d\n", fmt_priv.fmt.raw_data[13]);
	printf("status[0]=%d\n", fmt_priv.fmt.raw_data[16]);
	printf("status[1]=%d\n", fmt_priv.fmt.raw_data[17]);
	printf("status[2]=%d\n", fmt_priv.fmt.raw_data[18]);
	printf("status[3]=%d\n", fmt_priv.fmt.raw_data[19]);

	struct v4l2_requestbuffers req;
	CLEAR (req);
	req.count               = 5;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;
		
	ioctl (fd, VIDIOC_REQBUFS, &req); //申请缓冲，count是申请的数量，注意，释放缓冲实际在VIDIOC_STREAMOFF内完成了。

	buffers = calloc (req.count, sizeof (*buffers));//内存中建立对应空间

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) 
	{
	   struct v4l2_buffer buf;   //驱动中的一帧
	   CLEAR (buf);
	   buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	   buf.memory      = V4L2_MEMORY_MMAP;
	   buf.index       = n_buffers;

	   if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf)) //映射用户空间
			printf ("VIDIOC_QUERYBUF error\n");

	   buffers[n_buffers].length = buf.length;
	   buffers[n_buffers].start  = mmap (NULL /* start anywhere */,    //通过mmap建立映射关系
								         buf.length,
								         PROT_READ | PROT_WRITE /* required */,
								         MAP_SHARED /* recommended */,
								         fd, buf.m.offset);
	   printf("MMAP: %p OFF: %p\n", buffers[n_buffers].start, buf.m.offset);

	   if (MAP_FAILED == buffers[n_buffers].start)
			printf ("mmap failed\n");
	}
	
	for (i = 0; i < n_buffers; ++i) 
	{
		struct v4l2_buffer buf;
		
		CLEAR (buf);		
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;

		if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))//申请到的缓冲进入列队
			printf ("VIDIOC_QBUF failed\n");
	}
					
#ifdef DISPLAY				
	disp_init(disp_size.width,disp_size.height);
	disp_start();
	disp_on();                  
#endif	
	
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == ioctl (fd, VIDIOC_STREAMON, &type)) //开始捕捉图像数据
		printf ("VIDIOC_STREAMON failed\n");

	sigaction(SIGINT, &sigact, NULL);

	while(!quit)
	{	    
		read(key_fd, &data, sizeof(data));
		if( (data.type == EV_KEY)&&
			(data.code == KEY_ESC)&&
			(data.value == 1) ){
			quit = 1;
		}


		CLEAR (fmt_priv);
		fmt_priv.type                = V4L2_BUF_TYPE_PRIVATE;
		if (-1 == ioctl (fd, VIDIOC_G_FMT, &fmt_priv)) //监视状态
		{
			printf("VIDIOC_G_FMT error!  a\n");
			ret = -1;
			return ret;
		}
		//printf("status=0x%02x\n", fmt_priv.fmt.raw_data[16]);
	
		for (;;) //这一段涉及到异步IO
		{
			fd_set fds;
			struct timeval tv;
			int r;
			
			FD_ZERO (&fds);//将指定的文件描述符集清空
			FD_SET (fd, &fds);//在文件描述符集合中增加一个新的文件描述符
			
			/* Timeout. */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select (fd + 1, &fds, NULL, NULL, &tv);//判断是否可读（即摄像头是否准备好），tv是定时
			
			if (-1 == r) {
				if (EINTR == errno)
					continue;
				printf ("select err\n");
			}
			if (0 == r) {
				fprintf (stderr, "select timeout\n");
				//exit (EXIT_FAILURE);
				//goto close;
			}
								
			if (read_frame ())//如果可读，执行read_frame ()函数，并跳出循环
				break;
		} 
	}

close:	
	if (-1 == ioctl (fd, VIDIOC_STREAMOFF, &type)) //停止捕捉图像数据，注意，此动作同时会释放VIDIOC_REQBUFS申请的缓冲
		printf ("VIDIOC_STREAMOFF failed\n");
unmap:
	for (i = 0; i < n_buffers; ++i) {
		if (-1 == munmap (buffers[i].start, buffers[i].length)) {
			printf ("munmap error");
		}
	}
	
	disp_stop();
	disp_exit();	
	close (fd);
	
	printf("TVD demo bye!\n");
	return 0;						
}
