package main

import (
	"fmt"

	"github.com/sigurn/crc8"
	"github.com/tarm/serial"
)

var crcSMBUSParams = crc8.Params{0x07, 0x00, false, false, 0x00, 0xf4, "CRC-8/SMBUS"}
var maximCRCTable = crc8.MakeTable(crcSMBUSParams)

func main() {
	s, err := serial.OpenPort(&serial.Config{
		Name: "/dev/ttyUSB0",
		Baud: 9600,
	})
	if err != nil {
		panic(err)
	}

	buf := []byte{0xd4, 0, 0, 2, 0}
	buf[4] = crc8.Checksum(buf[:4], maximCRCTable)
	fmt.Printf("%x %x %x %x\n", buf[1], buf[2], buf[3], buf[4])
	//if _, err := s.Write(buf); err != nil {
	//	panic(err)
	//}
	s.Close()
}
