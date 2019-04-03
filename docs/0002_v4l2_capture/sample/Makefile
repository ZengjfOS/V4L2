all: 
	gcc getCameraInfo.c -o getCameraInfo
	gcc getyuyv.c -o getyuyv
	gcc yuyv2bmp.c -o yuyv2bmp

clean:
	rm my.bmp my.yuyv

test:
	./getCameraInfo
	./getyuyv
	./yuyv2bmp
