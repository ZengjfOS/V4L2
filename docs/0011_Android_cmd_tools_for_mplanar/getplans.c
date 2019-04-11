#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <error.h>
#include <string.h>
#include <assert.h>

// 下面四个头文件是linux系统编程特有的 
#include <sys/stat.h> 
#include <sys/ioctl.h> 
#include <sys/mman.h> 
#include <fcntl.h> 
#include <linux/videodev2.h> // 操作摄像头设备 


#define WIDTH 1280 			// 图片的宽度 
#define HEIGHT 720 			// 图片的高度 
#define FMT V4L2_PIX_FMT_YUYV 		// 图片格式 V4L2_PIX_FMT_YUYV
#define COUNT 3 			// 缓冲区个数 

// int main(int argc, char **argv) { 
int main() { 
	int ret; 
	int fd; 

	/* 第一步：打开摄像头设备文件 */ 
	fd = open("/dev/video0", O_RDWR); // 注意查看摄像头设备名 
	if (-1 == fd) { 
		perror("open /dev/video0"); 
		return -1; 
	} 

	/* 第二步：设置捕捉图片帧格式 */ 
	struct v4l2_format format; 
	// format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 操作类型为获取图片 
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; // 操作类型为获取图片 
	format.fmt.pix.width = WIDTH; // 图片的宽度 
	format.fmt.pix.height = HEIGHT; // 图片的高度 
	format.fmt.pix.pixelformat = FMT; // 图片格式 
	ret = ioctl(fd, VIDIOC_S_FMT, &format); // 进行设置(Set) 
	if (-1 == ret) { 
		perror("ioctl VIDIOC_S_FMT"); 
		close(fd); 
		return -2; 
	} 

	/* 第三步：检查是否设置成功 */ 
	ret = ioctl(fd, VIDIOC_G_FMT, &format); // Get 
	if (-1 == ret) { 
		perror("ioctl VIDIOC_G_FMT"); 
		close(fd); 
		return -3; 
	} 
	if (format.fmt.pix.pixelformat == FMT) { 
		printf("ioctl VIDIOC_S_FMT sucessful\n"); 
	} else { 
		printf("ioctl VIDIOC_S_FMT failed\n"); 
	} 


	struct v4l2_requestbuffers reqbuf;
	/* Our current format uses 3 planes per buffer */
	#define FMT_NUM_PLANES 1
	 
	struct {
		void *start[FMT_NUM_PLANES];
		size_t length[FMT_NUM_PLANES];
	} *buffers;
	unsigned int i, j;
	 
	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = COUNT;
	 
	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
        perror("VIDIOC_REQBUFS");
	}
	 
	buffers = calloc(reqbuf.count, sizeof(*buffers));
	assert(buffers != NULL);
	 
	for (i = 0; i < reqbuf.count; i++) {
		struct v4l2_buffer buffer;
		struct v4l2_plane planes[FMT_NUM_PLANES];
	 
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = reqbuf.type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;
		/* length in struct v4l2_buffer in multi-planar API stores the size
		 * of planes array. */
		buffer.length = FMT_NUM_PLANES;
		buffer.m.planes = planes;
	 
		if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) < 0) {
			perror("VIDIOC_QUERYBUF");
            break;
		}
	 
		/* Every plane has to be mapped separately */
		for (j = 0; j < FMT_NUM_PLANES; j++) {
			buffers[i].length[j] = buffer.m.planes[j].length; /* remember for munmap() */

            printf("i: %x, j:%x, planes length: %x, fd: %x, mem_offset: %x\r\n", i, j, buffer.m.planes[j].length, fd, buffer.m.planes[j].m.mem_offset);
	 
            if (buffer.m.planes[j].length == 0 )
                continue;

			buffers[i].start[j] = mmap(NULL, buffer.m.planes[j].length,
					 PROT_READ | PROT_WRITE, /* recommended */
					 MAP_SHARED,             /* recommended */
					 fd, buffer.m.planes[j].m.mem_offset);
	 
			if (MAP_FAILED == buffers[i].start[j]) {
				/* If you do not exit here you should unmap() and free()
				   the buffers and planes mapped so far. */
				perror("mmap");
                break;
			}
		}

		/* 把映射成功的缓冲区加入到摄像头驱动的图像数据采集队列里 */ 
		ret = ioctl(fd, VIDIOC_QBUF, &buffer); // Queue 
		if (-1 == ret) { 
			perror("VIDIOC_QBUF"); 
			return -6; 
		} 
	}

	/* 第六步：启动采集 */ 
	int on = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; // 设置启动标志位 
	ret = ioctl(fd, VIDIOC_STREAMON, &on); // 开启摄像头流 
	if (-1 == ret) { 
		perror("ioctl VIDIOC_STREAMON"); 
		return -7; 
	} 

    sleep(1);

	/* 第七步：让已经采集好的数据缓冲退出队列 */ 
    struct v4l2_buffer buffer;
    struct v4l2_plane planes[FMT_NUM_PLANES];
    buffer.m.planes = planes;
	ret = ioctl(fd, VIDIOC_DQBUF, &buffer); // Dequeue 
	if (-1 == ret) { 
		perror("ioctl VIDIOC_DQUF"); 
		return -8; 
	} 

    printf("buffer.index: %x, buffer.bytesused: %x\r\n", buffer.index, buffer.bytesused);
    for (i = 0; i < FMT_NUM_PLANES; i++) {
        printf("  i:%d, plane length: %x, mem_offset: %x, plane bytesused: %x\r\n", i, planes[i].length, planes[i].m.mem_offset, planes[i].bytesused);

        if (planes[i].bytesused > 0) {
            /* 第八步：从退出队列的缓冲区中获取数据并保存到文件中 */ 
            FILE *fl; fl = fopen("./my.yuyv", "w"); 
            if (NULL == fl) { 
                fprintf(stderr, "open write file failed."); 
            } 
            printf("plane index: %x, plane bytesused: %x\r\n", i, planes[i].bytesused);
            fwrite(buffers[buffer.index].start[i], planes[i].bytesused, 1, fl); 
            fclose(fl); // 记得关闭已打开的文件 
        } else {
            printf("buffer.bytesused is zero, plz check device\r\n");
        }
    }

	 
	/* Cleanup. */
	 
	for (i = 0; i < reqbuf.count; i++)
		for (j = 0; j < FMT_NUM_PLANES; j++)
			munmap(buffers[i].start[j], buffers[i].length[j]);


	close(fd); // 记得关闭已打开的设备 

	return 0; 
}

