package model

type BasePacket struct {
	SerialNumber uint16  `json:"serial"`
	Freq         float32 `json:"freq"`
	Type         uint8   `json:"type"`
	RocketTime   uint16  `json:"rtime"`
	Timestamp    uint64  `json:"time"`
	RawPacket    string  `json:"raw"`
	JsonPacket   string
}
