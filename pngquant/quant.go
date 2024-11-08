package pngquant

/*
#cgo CFLAGS: -I. -I./libadvpng -I./libpng -I./zlib -std=c99 -O3
#cgo LDFLAGS: -L${SRCDIR}/build -lpngquant -lm -limagequant -lrwpng

#include "math.h"
#include "stdlib.h"
#include "quant.h"
*/
import "C"

import (
	"errors"
	"fmt"
	"io"
	"os"
	"unsafe"
)

var (
	ErrEmptyFile = errors.New("empty file")
	ErrTransfrom = errors.New("type transform has error")
)

func Compress(file *os.File, floyd float32, minQuality, maxQuality, speed, rzlevel int) (*os.File, error) {
	stat, err := file.Stat()
	if err != nil {
		return nil, err
	}

	size := stat.Size()
	if size <= 0 {
		return nil, ErrEmptyFile
	}

	data, err := io.ReadAll(file)
	if err != nil {
		return nil, err
	}

	var out *C.uchar
	var length C.size_t
	var width, height C.int

	outPtr := (**C.uchar)(unsafe.Pointer(&out))
	lengthPtr := (*C.size_t)(unsafe.Pointer(&length))
	widthPtr := (*C.int)(unsafe.Pointer(&width))
	heightPtr := (*C.int)(unsafe.Pointer(&height))

	C.PNGCompress((*C.uchar)(unsafe.Pointer(&data[0])), C.size_t(size), outPtr, lengthPtr, widthPtr, heightPtr,
		C.int(minQuality), C.int(maxQuality), C.float(floyd), C.int(speed), C.int(rzlevel))

	outimage := C.GoBytes(unsafe.Pointer(out), C.int(length))
	dataLen := len(outimage)

	C.free(unsafe.Pointer(out))

	if dataLen != len(outimage) {
		return nil, ErrTransfrom
	}

	img, err := os.Create("./temp.png")
	if err != nil {
		return nil, err
	}

	_, err = img.Write(outimage)
	if err != nil {
		return nil, err
	}

	return img, nil
}

func Say() {
	fmt.Println("say hallo")
}
