package main

import (
	"fmt"
	"time"

	"github.com/sigurn/crc8"
	"github.com/tarm/serial"
)

var crcSMBUSParams = crc8.Params{0x07, 0x00, false, false, 0x00, 0xf4, "CRC-8/SMBUS"}
var maximCRCTable = crc8.MakeTable(crcSMBUSParams)

type packetType int
type moduleStatus int

const (
	packetTypeCommand  = 1
	packetTypeResponse = 0

	moduleStatusIdle   = 0
	moduleStatusMoving = 1
)

type packet struct {
	srcDst int
	pos    int
	typ    packetType
	status moduleStatus
}

func unmarshalPacket(raw []byte) packet {
	p := packet{}
	p.srcDst = int(raw[1])<<2 | int(raw[2])>>6
	p.pos = ((int(raw[2]) & 0x3f) << 6) | (int(raw[3]) >> 2)
	p.typ = packetType((raw[3] & 2) >> 1)
	p.status = moduleStatus(raw[3] & 1)
	return p
}

func (p packet) marshal() []byte {
	ret := make([]byte, 5)
	ret[0] = 0xd4
	ret[1] = byte(p.srcDst >> 2)
	ret[2] = byte(((p.srcDst & 0x3) << 6) | (p.pos >> 6))
	ret[3] = byte((p.pos & 0x3f) << 2)
	ret[3] |= byte((byte(p.typ) << 1) | byte(p.status))
	ret[4] = crc8.Checksum(ret[:4], maximCRCTable)
	return ret
}

func main() {
	s, err := serial.OpenPort(&serial.Config{
		Name: "/dev/ttyUSB0",
		Baud: 9600,
	})
	if err != nil {
		panic(err)
	}

	p := packet{
		srcDst: 0,
		pos:    0,
		typ:    packetTypeCommand,
	}
	buf := p.marshal()
	fmt.Printf("%+v\n", p)
	p = unmarshalPacket(buf)

	if _, err := s.Write(buf); err != nil {
		panic(err)
	}

	time.Sleep(time.Second)

	if _, err := s.Read(buf); err != nil {
		panic(err)
	}

	p = unmarshalPacket(buf)
	fmt.Printf("%+v\n", p)

	s.Close()
}
