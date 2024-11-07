package pngquant

/*
#cgo CFLAGS: -std=c99
#cgo LDFLAGS: -lm

#include "libimagequant.h"
*/
import "C"

import "image"

func Compress(img image.Image, minQuality int, maxQuality int, floyd float32, speed int) (image.Image, error) {
	width := img.Bounds().Dx()
	height := img.Bounds().Dy()

	return nil, nil
}
