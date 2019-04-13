#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <assert.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <pthread.h>
#include <poll.h>
#include <semaphore.h>
 
#define TimeOut 5 
 
#define CapNum 10
 
#define CapWidth 640
#define CapHeight 480
 
#define ReqButNum 3
 
#define IsRearCamera 0
 
#define  FPS 10
 
#define PIXELFMT V4L2_PIX_FMT_YUYV
 
#define CapDelay 100*1000
 
 
#define CLEAR(x)    memset(&(x), 0, sizeof(x))
 
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define CLAMP(x,l,h) ((x) < (l) ? (l) : ((x) > (h) ? (h) : (x)))
#define ERRSTR strerror(errno)
 
#define LOG(...) fprintf(stderr, __VA_ARGS__)
 
#define ERR(...) __info("Error", __FILE__, __LINE__, __VA_ARGS__)
#define ERR_ON(cond, ...) ((cond) ? ERR(__VA_ARGS__) : 0)
 
#define CRIT(...) \
	do { \
		__info("Critical", __FILE__, __LINE__, __VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while(0)
#define CRIT_ON(cond, ...) do { if (cond) CRIT(__VA_ARGS__); } while(0)
 
 
typedef struct
{
	void *start;
	int length;
	int bytesused;
}BUFTYPE;
 
 
char lcd_path[] = "/dev/fb0";
char fimc0_path[] = "/dev/video0";
char cam_path[] = "/dev/video13";
 
 
 
 
 
 
static sem_t lcd_sem;
 
BUFTYPE *fimc0_out_buf;
BUFTYPE *buffers;
static int n_buffer = 0;
void *fimc_in = NULL;
void *fimc_out = NULL;
 
int fimc0_out_buf_length;
int fimc0_cap_buf_length;
void *fimc0_out[16];
void *fimc0_cap[16];
 
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static int lcd_buf_size;
static char *fb_buf = NULL;
static int tsp_fd;
static int fimc0_fd;
static pthread_t capture_tid;
static pthread_t display_tid; 
	int lcd_fd;
	int cam_fd;
int display_x = 0;
int display_y = 0;
static int fimc0_cap_index = 0;
char *temp_buf=NULL;
int display_format(int pixelformat)
{
			printf("{pixelformat = %c%c%c%c}\n",
				pixelformat & 0xff,(pixelformat >> 8)&0xff,
				(pixelformat >> 16) & 0xff,(pixelformat >> 24)&0xff
				);
}
static inline int __info(const char *prefix, const char *file, int line,
	const char *fmt, ...)
{
	int errsv = errno;
	va_list va;
 
	va_start(va, fmt);
	fprintf(stderr, "%s(%s:%d): ", prefix, file, line);
	vfprintf(stderr, fmt, va);
	va_end(va);
	errno = errsv;
 
	return 1;
}
 
 
struct format {
	unsigned long fourcc;
	unsigned long width;
	unsigned long height;
};
void dump_format(char *str, struct v4l2_format *fmt)
{
	if (V4L2_TYPE_IS_MULTIPLANAR(fmt->type)) {
		struct v4l2_pix_format_mplane *pix = &fmt->fmt.pix_mp;
		LOG("%s: width=%u height=%u format=%.4s bpl=%u\n", str,
			pix->width, pix->height, (char*)&pix->pixelformat,
			pix->plane_fmt[0].bytesperline);
	} else {
		struct v4l2_pix_format *pix = &fmt->fmt.pix;
		LOG("%s: width=%u height=%u format=%.4s bpl=%u\n", str,
			pix->width, pix->height, (char*)&pix->pixelformat,
			pix->bytesperline);
	}
}
int open_camera_device()
{
	int fd;
 
	if((fd = open(cam_path,O_RDWR | O_NONBLOCK)) < 0)
	{
		perror("Fail to open");
		exit(EXIT_FAILURE);
	} 
	cam_fd = fd;
	if((fimc0_fd = open(fimc0_path,O_RDWR | O_NONBLOCK)) < 0)
	{
		perror("Fail to open");
		exit(EXIT_FAILURE);
	} 
	
	printf("open cam success %d\n",fd);
	return fd;
}
 
//打开摄像头设备
int open_lcd_device()
{
	int fd;
	int err;
	int ret;
	if((fd = open(lcd_path, O_RDWR | O_NONBLOCK)) < 0)
	{
		perror("Fail to open");
		exit(EXIT_FAILURE);
	} 
	printf("open lcd success %d\n",fd);
 
	if(-1 == ioctl(fd, FBIOGET_FSCREENINFO,&finfo))
	{
		perror("Fail to ioctl:FBIOGET_FSCREENINFO\n");
		exit(EXIT_FAILURE);
	}
	if (-1==ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) 
	{
		perror("Fail to ioctl:FBIOGET_VSCREENINFO\n");
		exit(EXIT_FAILURE);
	}
    lcd_buf_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;	
	printf("vinfo.xres:%d, vinfo.yres:%d, vinfo.bits_per_pixel:%d, lcd_buf_size:%d, finfo.line_length:%d\n",vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, lcd_buf_size, finfo.line_length); 
 
 
	lcd_fd = fd;
	
	vinfo.activate = FB_ACTIVATE_FORCE;
	vinfo.yres_virtual = vinfo.yres;
	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo );
	if( ret < 0 )
	{
		printf( "ioctl FBIOPUT_VSCREENINFO failed\n");
		return -1;
	}
	
    //mmap framebuffer    
    fb_buf = (char *)mmap(
	    NULL,
	    lcd_buf_size,
	    PROT_READ | PROT_WRITE,MAP_SHARED ,
	    lcd_fd, 
	    0);    
	if(NULL == fb_buf)
	{
		perror("Fail to mmap fb_buf");
		exit(EXIT_FAILURE);
	}
	ret = ioctl( lcd_fd, FBIOBLANK, FB_BLANK_UNBLANK );
	if( ret < 0 )
	{
			printf( "ioctl FBIOBLANK failed\n");
			return -1;
	}
	
	return fd;
}
int fb_wait_for_vsync(int lcd_fd)
{
	int ret;
	unsigned long temp;
 
	ret = ioctl(lcd_fd, FBIO_WAITFORVSYNC, &temp);
	if (ret < 0) {
		err("Wait for vsync failed");
		return -1;
	}
	return 0;
}
int cam_reqbufs()
{
	struct v4l2_requestbuffers req;
	int i;
	printf("%s: +\n", __func__);
	int n_buffers = 0;
	CLEAR(req);
 
	req.count  = ReqButNum;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
 
	if (-1 == ioctl(cam_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				 "user pointer i/o\n", "campture");
			exit(EXIT_FAILURE);
		} else {
			printf("VIDIOC_REQBUFS");
			exit(EXIT_FAILURE);
		}
	}
 
	buffers = calloc(ReqButNum, sizeof(*buffers));
 
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
 
	for (n_buffers = 0; n_buffers < ReqButNum; ++n_buffers) {
		buffers[n_buffers].length = fimc0_out_buf_length;
		buffers[n_buffers].start = fimc0_out_buf[n_buffers].start;
 
		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
/*
	for (i = 0; i < 3; ++i) {
		fimc0_out_buf[i].length = fimc0_out_buf_length;
		fimc0_out_buf[i].start = fimc0_out[i];
		if (!fimc0_out_buf[i].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}*/
	printf("%s: -\n", __func__);
}
 
int fimc0_reqbufs()
{
	int i = 0;
	int err;
	int ret;
	struct v4l2_control ctrl;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_requestbuffers rb;
	CLEAR(rb);
	/* enqueue the dmabuf to vivi */
	struct v4l2_buffer b;
	CLEAR(b);
 
 
	printf("%s: +\n", __func__);
		/* request buffers for FIMC0 */
	rb.count = ReqButNum;
	rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	rb.memory = V4L2_MEMORY_USERPTR;
	ret = ioctl(fimc0_fd, VIDIOC_REQBUFS, &rb);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_REQBUFS: %s\n", ERRSTR))
		return -errno;
	printf("fimc0 output_buf_num:%d\n",rb.count);
	int n;
 
	n_buffer = rb.count;
 
	fimc0_out_buf = calloc(rb.count,sizeof(BUFTYPE));
	if(fimc0_out_buf == NULL){
		fprintf(stderr,"Out of memory\n");
		exit(EXIT_FAILURE);
	}
	printf("%s, fimc0_out_buf request successfully\n",__func__);
	
		/* mmap DMABUF */
	struct v4l2_plane plane[2];
#if 1
	CLEAR(plane);
	CLEAR(b);
	b.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	b.memory = V4L2_MEMORY_USERPTR;
	b.index = 0;
	b.m.planes = plane;
	b.length = 1;
 
	fimc0_out_buf_length = 640*480*2;//for yuyv 422
	printf("%s, line:%d,fimc0_out_buf_length:%d\n",__func__,__LINE__, fimc0_out_buf_length);
	
    unsigned int page_size;   
    page_size = getpagesize();   
    fimc0_out_buf_length = (fimc0_out_buf_length + page_size - 1) & ~(page_size - 1); 
	printf("%s, line:%d,page_size:%d,fimc0_out_buf_length:%d\n",__func__, __LINE__, page_size, fimc0_out_buf_length);
	for (n = 0; n < ReqButNum; ++n) {
		
		//b.index = n;
		//printf("line:%d, buffers[n].start:0x%08x\n",buffers[n].start);
	//	void *addr = malloc(fimc0_out_buf_length + 64);
	//	addr = (void *)(((unsigned long)addr + 63)&(~63));
		fimc0_out_buf[n].start = (void *)memalign(/* boundary */(size_t)page_size, (size_t)fimc0_out_buf_length);//malloc(fimc0_out_buf_length);
		fimc0_out_buf[n].length= 640*480*2;
		if(fimc0_out_buf[n].start != NULL)
		printf("fimc0_out userptr reqbuf start:0x%08x,length:%d\n",fimc0_out_buf[n].start,fimc0_out_buf_length);
	}
#if 0
	for (n = 0; n < ReqButNum; ++n) {
		b.index = n;
		ret = ioctl(fimc0_fd, VIDIOC_QUERYBUF, &b);
		
		if (ERR_ON(ret < 0, "fimc0: VIDIOC_REQBUFS: %s\n", ERRSTR))
		exit(EXIT_FAILURE);
		
		//	printf("fimc0 querybuf:%d,%d\n", b.m.planes[0].length, b.m.planes[0].m.mem_offset);
			fimc0_out_buf[n].start = mmap(NULL,
						b.m.planes[0].length,
						PROT_READ | PROT_WRITE,
						MAP_SHARED, fimc0_fd,
						b.m.planes[0].m.mem_offset);
 
 
		
		//	fimc0_out_buf[n].start = fimc0_out[n];
			fimc0_out_buf[n].length = b.m.planes[0].length;
				if (fimc0_out[n] == MAP_FAILED) {
				printf("Failed mmap buffer %d for %d\n", n,
							fimc0_fd);
				return -1;
			}
 
		fimc0_out_buf_length = b.m.planes[0].length;
		printf("fimc0 querybuf:0x%08lx,%d,%d\n", fimc0_out_buf[n], fimc0_out_buf_length, b.m.planes[0].m.mem_offset);
		
	//	printf("fimc0 output:plane.length:%d\n",fimc0_out_buf_length);
	}
#endif
#endif
	CLEAR(plane);
	CLEAR(b);
 
	rb.count = ReqButNum;
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	rb.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fimc0_fd, VIDIOC_REQBUFS, &rb);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_REQBUFS: %s\n", ERRSTR))
		return -errno;
 
	for (n = 0; n < ReqButNum; ++n) {
	
		b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		b.memory = V4L2_MEMORY_MMAP;
		b.index = n;
		b.m.planes = plane;
		b.length = 1;
 
		b.index = n;
		ret = ioctl(fimc0_fd, VIDIOC_QUERYBUF, &b);
		if (ERR_ON(ret < 0, "fimc0: VIDIOC_QUERYBUF: %s\n", ERRSTR))
			return -errno;
 
		fimc0_cap[n] = mmap(NULL,
						b.m.planes[0].length,
						PROT_READ | PROT_WRITE,
						MAP_SHARED, fimc0_fd,
						b.m.planes[0].m.mem_offset);
			if (fimc0_cap[n] == MAP_FAILED) {
				printf("Failed mmap buffer %d for %d\n", n,
							fimc0_fd);
				return -1;
			}
 
		fimc0_cap_buf_length = b.m.planes[0].length;
		printf("fimc0 capture:plane.length:%d\n",fimc0_cap_buf_length);	
	}
 
	printf("%s -\n", __func__);
	return 0;
}
int cam_setfmt()
{
	int err;
	int ret;
	struct v4l2_fmtdesc fmt;
	struct v4l2_capability cap;
	struct v4l2_format stream_fmt;
	struct v4l2_input input;
	struct v4l2_control ctrl;
	struct v4l2_streamparm stream;
	
	memset(&fmt,0,sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while((ret = ioctl(cam_fd,VIDIOC_ENUM_FMT,&fmt)) == 0)
	{
		fmt.index ++ ;
		printf("{pixelformat = %c%c%c%c},description = '%s'\n",
				fmt.pixelformat & 0xff,(fmt.pixelformat >> 8)&0xff,
				(fmt.pixelformat >> 16) & 0xff,(fmt.pixelformat >> 24)&0xff,
				fmt.description);
	}
	ret = ioctl(cam_fd,VIDIOC_QUERYCAP,&cap);
	if(ret < 0){
		perror("FAIL to ioctl VIDIOC_QUERYCAP");
		exit(EXIT_FAILURE);
	}
 
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		printf("The Current device is not a video capture device\n");
		exit(EXIT_FAILURE);
	
	}
 
	if(!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		printf("The Current device does not support streaming i/o\n");
		exit(EXIT_FAILURE);
	}
 
	CLEAR(stream_fmt);
	stream_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	stream_fmt.fmt.pix.width = CapWidth;
	stream_fmt.fmt.pix.height = CapHeight;
	stream_fmt.fmt.pix.pixelformat = PIXELFMT;
	stream_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
 
	if(-1 == ioctl(cam_fd,VIDIOC_S_FMT,&stream_fmt))
	{
		printf("Can't set the fmt\n");
		perror("Fail to ioctl\n");
		exit(EXIT_FAILURE);
	}
	printf("VIDIOC_S_FMT successfully\n");
	
	printf("%s: -\n", __func__);
	return 0;
}
int cam_setrate()
{
	int err;
	int ret;
 
	struct v4l2_streamparm stream;
 
	CLEAR(stream);
    stream.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    stream.parm.capture.capturemode = 0;
    stream.parm.capture.timeperframe.numerator = 1;
    stream.parm.capture.timeperframe.denominator = FPS;
 
    err = ioctl(cam_fd, VIDIOC_S_PARM, &stream);
	if(err < 0)
    printf("FimcV4l2 start: error %d, VIDIOC_S_PARM", err);
 
	return 0;
 
}
int fimc0_setfmt()
{
	int err;
	int ret;
	struct v4l2_fmtdesc fmt;
	struct v4l2_capability cap;
	struct v4l2_format stream_fmt;
	struct v4l2_input input;
	struct v4l2_control ctrl;
	struct v4l2_streamparm stream;
 
	printf("%s: +\n", __func__);
	CLEAR(stream_fmt);
	stream_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	stream_fmt.fmt.pix.width = CapWidth;
	stream_fmt.fmt.pix.height = CapHeight;
	stream_fmt.fmt.pix.pixelformat = PIXELFMT;
	stream_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
		/* get format from VIVI */
	ret = ioctl(cam_fd, VIDIOC_G_FMT, &stream_fmt);
	if (ERR_ON(ret < 0, "vivi: VIDIOC_G_FMT: %s\n", ERRSTR))
		return -errno;
	dump_format("cam_fd-capture", &stream_fmt);
 
 
		/* setup format for FIMC 0 */
	/* keep copy of format for to-mplane conversion */
	
	struct v4l2_pix_format pix = stream_fmt.fmt.pix;
 
	CLEAR(stream_fmt);
	stream_fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	struct v4l2_pix_format_mplane *pix_mp = &stream_fmt.fmt.pix_mp;
 
	pix_mp->width = pix.width;
	pix_mp->height = pix.height;
	pix_mp->pixelformat = pix.pixelformat;
	pix_mp->num_planes = 1;
	pix_mp->plane_fmt[0].bytesperline = pix.bytesperline;
 
	dump_format("fimc0-output", &stream_fmt);
	ret = ioctl(fimc0_fd, VIDIOC_S_FMT, &stream_fmt);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_S_FMT: %s\n", ERRSTR))
		return -errno;
	dump_format("fimc0-output", &stream_fmt);
	
		/* set format on fimc0 capture */
	stream_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	/* try cmdline format, or use fimc0-output instead */
 
		struct v4l2_pix_format_mplane *pix_mp_f = &stream_fmt.fmt.pix_mp;
		CLEAR(*pix_mp_f);
		pix_mp_f->pixelformat = V4L2_PIX_FMT_RGB32;
		pix_mp_f->width = 800;
		pix_mp_f->height = 480;
		pix_mp_f->plane_fmt[0].bytesperline = 0;
 
 
	dump_format("pre-fimc0-capture", &stream_fmt);
	ret = ioctl(fimc0_fd, VIDIOC_S_FMT, &stream_fmt);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_S_FMT: %s\n", ERRSTR))
		return -errno;
	
	printf("%s -\n", __func__);
 
}
int init_device()
{
	cam_setfmt();
	fimc0_setfmt();
	fimc0_reqbufs();
 
	cam_reqbufs();
	cam_setrate();
 
	printf("%s -\n", __func__);
 
}
 
int start_capturing(int cam_fd)
{
	unsigned int i;
	enum v4l2_buf_type type;
		int ret;
	struct v4l2_buffer b;
	struct v4l2_plane plane;
	printf("%s +\n", __func__);
	
	for(i = 0;i < n_buffer;i ++)
	{
		struct v4l2_buffer buf;
 
		CLEAR(buf);	
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	
		buf.memory = V4L2_MEMORY_USERPTR;	
		buf.index = i;	
		buf.m.userptr = (unsigned long)buffers[i].start;
		buf.length = buffers[i].length;
		printf("cam qbuf:%d,userptr:0x%08x,length:%d\n",i, buf.m.userptr, buf.length);
		if(-1 == ioctl(cam_fd,VIDIOC_QBUF,&buf))
		{
			perror("cam Fail to ioctl 'VIDIOC_QBUF'");
			exit(EXIT_FAILURE);
		}
	}
 
	CLEAR(plane);
	CLEAR(b);
	b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	b.memory = V4L2_MEMORY_MMAP;
	b.index = 0;
	b.m.planes = &plane;
	b.length = 1;
	for(i = 0;i < n_buffer;i ++)
	{
		b.index = i;
		ret = ioctl(fimc0_fd, VIDIOC_QBUF, &b);
		if (ERR_ON(ret < 0, "fimc0: VIDIOC_QBUF: %s\n", ERRSTR))
			return -errno;	
 
	}
 
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(-1 == ioctl(cam_fd,VIDIOC_STREAMON,&type))
	{
		printf("i = %d.\n",i);
		perror("cam_fd Fail to ioctl 'VIDIOC_STREAMON'");
		exit(EXIT_FAILURE);
	}
	
			/* start processing */
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	ret = ioctl(fimc0_fd, VIDIOC_STREAMON, &type);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_STREAMON: %s\n", ERRSTR))
		return -errno;
/*
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	ret = ioctl(fimc0_fd, VIDIOC_STREAMON, &type);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_STREAMON: %s\n", ERRSTR))
		return -errno;
*/
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	ret = ioctl(fimc0_fd, VIDIOC_STREAMON, &type);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_STREAMON: %s\n", ERRSTR))
		return -errno;
	printf("%s -\n", __func__);
	return 0;
}
int cam_cap_dbuf(int *index)
{
	
	unsigned int i;
	enum v4l2_buf_type type;
	int ret;
	struct v4l2_buffer buf;
 
	bzero(&buf,sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;
	if(-1 == ioctl(cam_fd,VIDIOC_DQBUF,&buf))
	{
		perror("Fail to ioctl 'VIDIOC_DQBUF'");
		exit(EXIT_FAILURE);
	}
	buffers[buf.index].bytesused = buf.bytesused;
	printf("%s,Line:%d,bytesused:%d\n",__func__, __LINE__, buf.bytesused);
	*index = buf.index;
 
//	printf("%s -\n", __func__);
	return 0;
 
}
int cam_cap_qbuf(int index)
{
	
	struct v4l2_buffer buf;
 
		bzero(&buf,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = index;
 
		buf.m.userptr = (unsigned long)buffers[index].start;
		buf.length = buffers[index].length;
	//	printf("cam qbuf:%d,userptr:0x%08x,length:%d\n",i, buf.m.userptr, buf.length);
		if(-1 == ioctl(cam_fd,VIDIOC_QBUF,&buf))
		{
			perror("cam Fail to ioctl 'VIDIOC_QBUF'");
			exit(EXIT_FAILURE);
		}
//		printf("%s -\n", __func__);
		return 0;
}
 
int fimc0_out_qbuf(int index)
{
	unsigned int i;
	enum v4l2_buf_type type;
	int ret;
	struct v4l2_buffer b, buf;
	struct v4l2_plane plane[3];
	
	/* enqueue buffer to fimc0 output */
	CLEAR(plane);
	CLEAR(b);
	b.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	b.memory = V4L2_MEMORY_USERPTR;
	b.index = index;
	b.m.planes = plane;
	b.length = 1;
	if(b.memory == V4L2_MEMORY_USERPTR)
	{
		plane[0].m.userptr = (unsigned long)fimc0_out_buf[index].start;
		plane[0].length = (unsigned long)fimc0_out_buf[index].length;
		plane[0].bytesused = fimc0_out_buf[index].length;
	}
	else
	{
		memcpy(fimc0_out_buf[index].start, buffers[index].start, buffers[index].length);
 
	}
 
//	printf("fimc0_out_buf:0x%08lx,length:%d,byteused:%d\n",fimc0_out_buf[index].start, 	fimc0_out_buf[index].length, fimc0_out_buf[index].bytesused);
	//process_image(fimc0_out_buf[index].start,0);	
	ret = ioctl(fimc0_fd, VIDIOC_QBUF, &b);
	
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_QBUF: %s\n", ERRSTR))
		return -errno;	
//	printf("%s -\n", __func__);
}
 
int fimc0_out_dbuf(int *index)
{
 
	unsigned int i;
	enum v4l2_buf_type type;
	int ret;
	struct v4l2_buffer b, buf;
	struct v4l2_plane plane[3];
	
	/* enqueue buffer to fimc0 output */
	CLEAR(plane);
	CLEAR(b);
	b.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	b.memory = V4L2_MEMORY_USERPTR;
//	b.index = index;
	b.m.planes = plane;
	b.length = 1;
//	plane[0].m.userptr = (unsigned long)fimc0_out_buf[index].start;
//	plane[0].length = (unsigned long)fimc0_out_buf[index].length;
//	planes[0].bytesused = fimc0_out_buf[buf.index].length;
	ret = ioctl(fimc0_fd, VIDIOC_DQBUF, &b);
	*index = b.index;
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_DQBUF: %s\n", ERRSTR))
		return -errno;	
}
 
int fimc0_cap_dbuf(int *index)
{
 
	unsigned int i;
	enum v4l2_buf_type type;
	int ret;
	struct v4l2_buffer b, buf;
	struct v4l2_plane plane[3];
	static int count = 0;
	/* enqueue buffer to fimc0 output */
	CLEAR(plane);
	CLEAR(b);
	b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	b.memory = V4L2_MEMORY_MMAP;
//	b.index = index;
	b.m.planes = plane;
	b.length = 1;
//	plane[0].m.userptr = (unsigned long)fimc0_out_buf[index].start;
//	plane[0].length = (unsigned long)fimc0_out_buf[index].length;
//	planes[0].bytesused = fimc0_out_buf[buf.index].length;
	ret = ioctl(fimc0_fd, VIDIOC_DQBUF, &b);
	*index = b.index;
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_DQBUF: %s\n", ERRSTR))
		return -errno;	
 
	sem_post(&lcd_sem);
	fimc0_cap_index = b.index;
	count ++;
	//memcpy((void *)temp_buf, (void *)fimc0_cap[b.index], 800*480*4);
	printf("%s,%d\n",__func__, count);
	return 0;
}
#if 0
int fimc0_cap_qbuf(int index)
{
 
	unsigned int i;
	enum v4l2_buf_type type;
	int ret;
	struct v4l2_buffer b, buf;
	struct v4l2_plane plane[3];
	
	/* enqueue buffer to fimc0 output */
	CLEAR(plane);
	CLEAR(b);
	b.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	b.memory = V4L2_MEMORY_MMAP;
	b.index = index;
	b.m.planes = plane;
	b.length = 1;
	if(b.memory == V4L2_MEMORY_USERPTR)
	{
		plane[0].m.userptr = (unsigned long)fimc0_out_buf[index].start;
		plane[0].length = (unsigned long)fimc0_out_buf[index].length;
		plane[0].bytesused = fimc0_out_buf[buf.index].length;
	}
//	printf("fimc0_out_buf:0x%08lx,length:%d,byteused:%d\n",fimc0_out_buf[index].start, 	fimc0_out_buf[index].length, fimc0_out_buf[index].bytesused);
//	process_image(fimc0_out_buf[index].start,0);	
	ret = ioctl(fimc0_fd, VIDIOC_QBUF, &b);
	
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_QBUF: %s\n", ERRSTR))
		return -errno;	
//	printf("%s -\n", __func__);
}
#endif
int fimc0_cap_qbuf(int index)
{
//	int *pdata = (int *)addr;
	int ret;
	struct v4l2_buffer b;
	struct v4l2_plane plane;
	static unsigned int count = 0;
	//sleep(0);
	printf("%s +\n", __func__);
		CLEAR(plane);
	CLEAR(b);
	b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	b.memory = V4L2_MEMORY_MMAP;
	b.m.planes = &plane;
	b.length = 1;
	b.index = index;
	
	ret = ioctl(fimc0_fd, VIDIOC_QBUF, &b);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_QBUF: %s\n", ERRSTR))
		return -errno;
//	printf("%s -\n", __func__);
}
void process_cam_to_fimc0()
{
	int index;
	printf("%s +\n", __func__);
	cam_cap_dbuf(&index);
	fimc0_out_qbuf(index);
	printf("%s -,index:%d\n",__func__, index);
}
void process_fimc0_to_cam()
{
	int index;
	printf("%s +\n", __func__);
	fimc0_out_dbuf(&index);
	cam_cap_qbuf(index);
	printf("%s -,index:%d\n",__func__, index);
}
 
int process_fimc0_capture()
{
//	int *pdata = (int *)addr;
	int ret;
	struct v4l2_buffer b;
	struct v4l2_plane plane;
	static unsigned int count = 0;
	//sleep(0);
//	printf("%s +\n", __func__);
		CLEAR(plane);
	CLEAR(b);
	b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	b.memory = V4L2_MEMORY_MMAP;
	b.m.planes = &plane;
	b.length = 1;
 
	/* grab processed buffers */
	ret = ioctl(fimc0_fd, VIDIOC_DQBUF, &b);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_DQBUF: %s\n", ERRSTR))
		return -errno;
	count ++;
	printf("%s,%d\n",__func__, count);
 
	//memcpy((void *)fb_buf, (void *)fimc0_cap[b.index], 800*480*4);
	//process_rgb32(fimc0_cap[b.index]);	
	//temp_buf = (char *)fimc0_cap[b.index];
		
	/* enqueue buffer to fimc0 capture */
//	CLEAR(plane);
//	CLEAR(b);
//	b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
//	b.memory = V4L2_MEMORY_MMAP;
//	b.index = 0;
//	b.m.planes = &plane;
//	b.length = 1;
 
	ret = ioctl(fimc0_fd, VIDIOC_QBUF, &b);
	if (ERR_ON(ret < 0, "fimc0: VIDIOC_QBUF: %s\n", ERRSTR))
		return -errno;
//	printf("%s -\n", __func__);
	//memcpy(fb_buf, fimc_out, 640)	
 
}
 
int mainloop(int cam_fd)
{ 
	int count = 1;//CapNum;
	clock_t startTime, finishTime;
	double selectTime, frameTime;
	struct pollfd fds[2];
	int nfds = 0;
 
	while(count++  > 0)
	{
		{
			struct timeval tv;
			int r;
			struct timeval start;
			struct timeval end;
			int time_use=0;
			gettimeofday(&start,NULL);
			
			fds[0].events |= POLLIN | POLLPRI;
			fds[0].fd = cam_fd;
 
			fds[1].events |= POLLIN | POLLPRI | POLLOUT;
			fds[1].fd = fimc0_fd;
			//++nfds;
			
			r = poll(fds, 2, -1);
			if(-1 == r)
			{
				if(EINTR == errno)
					continue;
				
				perror("Fail to select");
				exit(EXIT_FAILURE);
			}
			if(0 == r)
			{
				fprintf(stderr,"select Timeout\n");
				exit(EXIT_FAILURE);
			}
 
			if (fds[0].revents & POLLIN)
			{
				process_cam_to_fimc0();
				gettimeofday(&end,NULL);
				time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
			//	printf("time_use is %dms\n",time_use/1000);
			}
			if (fds[1].revents & POLLIN)
			{
		//		printf("fimc0 has data to read\n");
				int index;
				fimc0_cap_dbuf(&index);
			//	fimc0_cap_qbuf(fimc0_cap_index);
			}
			if (fds[1].revents & POLLOUT)
			{
				int index;
				process_fimc0_to_cam();
			}
		}
	}
	return 0;
}
 
void stop_capturing(int cam_fd)
{
	enum v4l2_buf_type type;
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(-1 == ioctl(cam_fd,VIDIOC_STREAMOFF,&type))
	{
		perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
		exit(EXIT_FAILURE);
	}
	return;
}
void uninit_camer_device()
{
	unsigned int i;
 
	for(i = 0;i < n_buffer;i ++)
	{
		if(-1 == munmap(fimc0_out_buf[i].start, fimc0_out_buf[i].length))
		{
			exit(EXIT_FAILURE);
		}
	}
	if (-1 == munmap(fb_buf, lcd_buf_size)) 
	{          
		perror(" Error: framebuffer device munmap() failed.\n");          
		exit (EXIT_FAILURE) ;       
	}   
	free(fimc0_out_buf);
 
 
	return;
}
 
void close_camer_device(int lcd_fd, int cam_fd)
{
	if(-1 == close(lcd_fd))
	{
		perror("Fail to close lcd_fd");
		exit(EXIT_FAILURE);
	}
	if(-1 == close(cam_fd))
	{
		perror("Fail to close cam_fd");
		exit(EXIT_FAILURE);
	}
 
	return;
}
static void *cam_thread(void *pVoid)
{
	mainloop(cam_fd);
}
static void *display_thread(void *pVoid)
{
	static unsigned int count = 0;
	printf("display_thread start\n");
 
	int num = 800*480*4;
	while(1)
	{
		sem_wait(&lcd_sem);
		count ++;
		memcpy((void *)fb_buf, (void *)fimc0_cap[fimc0_cap_index], num);
		//fb_wait_for_vsync(lcd_fd);
		fimc0_cap_qbuf(fimc0_cap_index);
	}
}
int main()
{
	sem_init(&lcd_sem, 0, 0);
	temp_buf =(char *)malloc(800*480*4);
	open_lcd_device();
	open_camera_device();
	init_device(lcd_fd, cam_fd);
	start_capturing(cam_fd);
	pthread_create(&capture_tid,NULL,cam_thread,(void *)NULL);  
	pthread_create(&display_tid,NULL,display_thread,(void *)NULL); 
	while(1)
	{
		sleep(10);
	}
	stop_capturing(cam_fd);
	uninit_camer_device();
	close_camer_device(lcd_fd, cam_fd);
	return 0;
}
