
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
 
int main () {
    int fp=0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int screensize=0;
    char *fbp = 0;
    unsigned int x = 0, y = 0;
    int location = 0;
    int bytes_per_pixel;
 
    int a = 3;
    int b = 4;
    // int r = 0;
 
    fp = open ("/dev/graphics/fb0",O_RDWR);
 
    if (fp < 0){
        printf("Error : Can not open framebuffer device\n");
        exit(1);
    }
 
    if (ioctl(fp,FBIOGET_FSCREENINFO,&finfo)){
        printf("Error reading fixed information\n");
        exit(2);
    }
    if (ioctl(fp,FBIOGET_VSCREENINFO,&vinfo)){
        printf("Error reading variable information\n");
        exit(3);
    }
    bytes_per_pixel = vinfo.bits_per_pixel / 8;
    screensize = vinfo.xres * vinfo.yres * bytes_per_pixel;   //单帧画面空间  单位：字节
    printf("x=%d  y=%d  bytes_per_pixel=%d\n", vinfo.xres, vinfo.yres, bytes_per_pixel);
    printf("screensize=%d\n", screensize);
 
 
 
    /*这就是把fp所指的文件中从开始到screensize大小的内容给映射出来，得到一个指向这块空间的指针*/
    fbp =(char *) mmap (0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fp,0);
    if ((int) fbp == -1)
    {
         printf ("Error: failed to map framebuffer device to memory.\n");
         exit (4);
    }
    /*这是你想画的点的位置坐标,(0，0)点在屏幕左上角*/
    while(1) {
 
        x = 0;
        y = 0;
        for(x = 0; x < vinfo.xres; x++) {
            y = a * x + b;
            if ( y > vinfo.yres)
                break;
 
            location = x * bytes_per_pixel + y  *  finfo.line_length;
 
            *(fbp + location) = 0;  /* 蓝色的色深 */  /*直接赋值来改变屏幕上某点的颜色*/
            *(fbp + location + 1) = 255; /* 绿色的色深*/   /*注明：这几个赋值是针对每像素四字节来设置的，如果针对每像素2字节，*/
            *(fbp + location + 2) = 0; /* 红色的色深*/   /*比如RGB565，则需要进行转化*/
            *(fbp + location + 3) = 0;  /* 是否透明*/ 
        }
       
        usleep(40);
    }
    munmap (fbp, screensize); /*解除映射*/
    close (fp);    /*关闭文件*/
 
    return 0;
 
}
