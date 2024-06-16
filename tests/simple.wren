
import "carrica" for Host

var printRef = Host.ref("write")

Host.call(printRef, "\nHello world from carrica!")

