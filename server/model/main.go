package model

type BasePacket struct {
	Serial uint16
	Freq   float32
	Type   uint8
	RTime  uint16
	Time   uint64
	Raw    string
}
