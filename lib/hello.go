package lib

//#include "hello.h"
import "C"

func Say() {
	C.SayHello(C.CString("Hello, World\n"))
}
