package main

import (
	"fmt"
	"os"

	"github.com/lkangle/go-imagekit/pngquant"
)

func main() {
	file, e := os.Open("./head.png")
	if e != nil {
		fmt.Println(e.Error())
		return
	}
	defer file.Close()

	img, err := pngquant.Compress(file, 1, 50, 98, 0, 1)
	if err != nil {
		fmt.Println(err.Error())
		return
	}
	defer img.Close()

	name := img.Name()

	fmt.Println("compress success, name: ", name)
}
