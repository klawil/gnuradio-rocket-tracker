package socket

import (
	"bufio"
	"database/sql"
	"encoding/json"
	"io"
	"math/rand"
	"net"
	"rocket_server/model"
	"strconv"
	"strings"
	"sync"
	"time"

	log "github.com/sirupsen/logrus"
)

type Source struct {
	Type   string
	Source string
	Id     string
}

const charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

func (s *Source) SetId() {
	seededRand := rand.New(rand.NewSource(time.Now().UnixNano())) // Seed the random number generator
	b := make([]byte, 20)
	for i := range b {
		b[i] = charset[seededRand.Intn(len(charset))]
	}
	s.Id = string(b)
}

func (s *Source) Save(db *sql.DB) {
	_, err := db.Exec(
		"INSERT INTO sources (id, source_type, source_source) VALUES ($1, $2, $3)",
		s.Id,
		s.Type,
		s.Source,
	)

	if err != nil {
		log.WithField("source", s).WithError(err).Error("Failed to save source")
	}
}

func (s *Source) Delete(db *sql.DB) {
	_, err := db.Exec(
		"UPDATE sources SET deleted_at = now() WHERE id = $1",
		s.Id,
	)
	if err != nil {
		log.WithField("source", s).WithError(err).Error("Failed to save source")
	}

	_, err = db.Exec(
		"UPDATE channels SET deleted_at = now() WHERE source_id = $1 AND deleted_at IS NULL",
		s.Id,
	)
	if err != nil {
		log.WithField("source", s).WithError(err).Error("Failed to save source")
	}
}

const (
	SERVER_HOST = "0.0.0.0"
	SERVER_PORT = "8765"
	SERVER_TYPE = "tcp"
)

func parseLine(message string, db *sql.DB, source Source) {
	baseLog := log.WithField("msg", message).WithField("source", source)

	// Check for a ping
	if message == "ping\n" {
		baseLog.Debug("Ping received")
		return
	}

	// Check for a channel add / remove
	if message[0] == 'c' || message[0] == 'r' {
		split := strings.Split(message, ":")
		baseLog = baseLog.WithField("split", split)
		freq := split[1][0 : len(split[1])-1]
		baseLog.Info("channel message received")

		chFreq, err := strconv.Atoi(freq)
		if err != nil {
			log.WithError(err).Error("Failed to convert frequency to int")
		}
		if message[0] == 'c' {
			_, err = db.Exec(
				"INSERT INTO channels (source_id, frequency) VALUES ($1, $2)",
				source.Id,
				chFreq,
			)
		} else {
			_, err = db.Exec(
				"UPDATE channels SET deleted_at = NOW() WHERE source_id = $1 AND frequency = $2 AND deleted_at IS NULL",
				source.Id,
				chFreq,
			)
		}
		if err != nil {
			baseLog.WithError(err).Error("Failed to update channel record")
		}
		return
	}

	var packet model.BasePacket
	err := json.Unmarshal([]byte(message), &packet)
	if err != nil {
		baseLog.WithError(err).Error("Failed to parse packet")
	} else {
		_, err = db.Exec(
			"INSERT INTO packets (packet_json, source_id) VALUES ($1, $2)",
			message,
			source.Id,
		)
		if err != nil {
			baseLog.WithError(err).Error("Failed to insert packet")
		}
	}
}

func handleSource(conn net.Conn, db *sql.DB) {
	defer func() {
		conn.Close()
	}()
	source := Source{
		Type:   "socket",
		Source: conn.RemoteAddr().String(),
	}
	source.SetId()
	source.Save(db)
	baseLog := log.WithField("source", source)
	baseLog.Info("New socket source connected")
	defer func() {
		source.Delete(db)
	}()

	_, err := conn.Write([]byte("!!\n"))
	if err != nil {
		baseLog.WithError(err).Error("Error writing to source")
		conn.Close()
		return
	}

	socketOpen := true
	packetsReceived := 0
	go func() {
		connReader := bufio.NewReader(conn)
		for {
			message, err := connReader.ReadString('\n')
			if err != nil && err != io.EOF {
				baseLog.WithError(err).Error("Error reading from socket")
				socketOpen = false
				return
			} else if err != nil {
				baseLog.Info("Socket source disconnected")
				socketOpen = false
				return
			}
			packetsReceived++
			go parseLine(message, db, source)
		}
	}()

	for socketOpen {
		time.After(1 * time.Second)
	}

	baseLog.WithField("rcv", packetsReceived).Info("Total received")
}

var SendClientCommand = make(chan model.ClientCommand)
var ReceiveNewPackets = make(chan model.BasePacket)

func Main(db *sql.DB, wg *sync.WaitGroup) {
	defer wg.Done()

	server, err := net.Listen(SERVER_TYPE, SERVER_HOST+":"+SERVER_PORT)
	if err != nil {
		log.WithError(err).Panic("Failed to start socket server")
	}
	log.WithField("host", SERVER_HOST).WithField("port", SERVER_PORT).Info("Socket server started")

	for {
		conn, err := server.Accept()
		if err != nil {
			log.WithError(err).Error("Error accepting incoming connectiong")
		} else {
			go handleSource(conn, db)
		}
	}
}
