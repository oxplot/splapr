package main

import (
	"flag"
	"fmt"
	"time"

	"github.com/sigurn/crc8"
	"github.com/tarm/serial"
)

var (
	crcSMBUSParams = crc8.Params{0x07, 0x00, false, false, 0x00, 0xf4, "CRC-8/SMBUS"}
	smbusCRCTable  = crc8.MakeTable(crcSMBUSParams)

	baud         = flag.Int("b", 115200, "baud rate")
	device       = flag.String("d", "/dev/ttyUSB0", "serial device")
	sendCmd      = flag.Bool("c", true, "send command instead of response")
	targetPos    = flag.Int("p", 0, "target position")
	targetModule = flag.Int("m", 0, "target module")
	readReply    = flag.Bool("r", false, "read reply packet")
)

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
	valid  bool
}

func unmarshalPacket(raw []byte) packet {
	p := packet{}
	p.srcDst = int(raw[1])<<2 | int(raw[2])>>6
	p.pos = ((int(raw[2]) & 0x3f) << 6) | (int(raw[3]) >> 2)
	p.typ = packetType((raw[3] & 2) >> 1)
	p.status = moduleStatus(raw[3] & 1)
	p.valid = crc8.Checksum(raw[:4], smbusCRCTable) == raw[4] && raw[0] == 0xd4
	return p
}

func (p packet) marshal() []byte {
	ret := make([]byte, 5)
	ret[0] = 0xd4
	ret[1] = byte(p.srcDst >> 2)
	ret[2] = byte(((p.srcDst & 0x3) << 6) | (p.pos >> 6))
	ret[3] = byte((p.pos & 0x3f) << 2)
	ret[3] |= byte((byte(p.typ) << 1) | byte(p.status))
	ret[4] = crc8.Checksum(ret[:4], smbusCRCTable)
	return ret
}

func main() {
	flag.Parse()
	s, err := serial.OpenPort(&serial.Config{
		Name: *device,
		Baud: *baud,
	})
	if err != nil {
		panic(err)
	}
	defer s.Close()

	p := packet{
		srcDst: *targetModule,
		pos:    *targetPos,
		valid:  true,
	}
	if *sendCmd {
		p.typ = packetTypeCommand
	} else {
		p.typ = packetTypeResponse
	}

	buf := p.marshal()
	fmt.Printf("Sending %+v\n", p)

	if _, err := s.Write(buf); err != nil {
		panic(err)
	}

	if !*readReply {
		return
	}

	time.Sleep(time.Second)

	if _, err := s.Read(buf); err != nil {
		panic(err)
	}

	p = unmarshalPacket(buf)
	fmt.Printf("Received %+v\n", p)
}
