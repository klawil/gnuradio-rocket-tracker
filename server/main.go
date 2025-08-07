package main

import (
	"bufio"
	"database/sql"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"math/rand"
	"net"
	"os"
	"time"

	_ "github.com/lib/pq"
	log "github.com/sirupsen/logrus"
)

type BasePacket struct {
	SerialNumber uint16  `json:"serial"`
	Freq         float32 `json:"freq"`
	Type         uint8   `json:"type"`
	RocketTime   uint16  `json:"rtime"`
	Timestamp    uint64  `json:"time"`
	RawPacket    string  `json:"raw"`
	JsonPacket   string
}

type Client struct {
	Type   string
	Source string
	Id     string
}

const charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

func (c *Client) SetId() {
	seededRand := rand.New(rand.NewSource(time.Now().UnixNano())) // Seed the random number generator
	b := make([]byte, 20)
	for i := range b {
		b[i] = charset[seededRand.Intn(len(charset))]
	}
	c.Id = string(b)
}

func (c *Client) Save(db *sql.DB) {
	_, err := db.Exec(
		"INSERT INTO sources (id, source_type, source_source) VALUES ($1, $2, $3)",
		c.Id,
		c.Type,
		c.Source,
	)

	if err != nil {
		log.WithField("client", c).WithError(err).Error("Failed to save client")
	}
}

func (c *Client) Delete(db *sql.DB) {
	_, err := db.Exec(
		"UPDATE sources SET deleted_at = now() WHERE id = $1",
		c.Id,
	)

	if err != nil {
		log.WithField("client", c).WithError(err).Error("Failed to save client")
	}
}

const (
	SERVER_HOST = "0.0.0.0"
	SERVER_PORT = "8765"
	SERVER_TYPE = "tcp"

	host     = "localhost"
	port     = 5432
	user     = "rockettracker"
	password = "rockettracker"
	dbname   = "rockettracker"
)

func parseLine(message string, db *sql.DB, client Client) {
	baseLog := log.WithField("msg", message).WithField("client", client)

	// Check for a ping
	if message == "ping\n" {
		baseLog.Debug("Ping received")
		return
	}

	var packet BasePacket
	err := json.Unmarshal([]byte(message), &packet)
	if err != nil {
		baseLog.WithError(err).Error("Failed to parse packet")
	} else {
		_, err = db.Exec(
			"INSERT INTO packets (packet_json) VALUES ($1)",
			message,
		)
		if err != nil {
			baseLog.WithError(err).Error("Failed to insert packet")
		}
	}
}

func handleClient(conn net.Conn, db *sql.DB) {
	defer func() {
		conn.Close()
	}()
	client := Client{
		Type:   "socket",
		Source: conn.RemoteAddr().String(),
	}
	client.SetId()
	client.Save(db)
	baseLog := log.WithField("client", client)
	baseLog.Info("New socket client connected")
	defer func() {
		client.Delete(db)
	}()

	_, err := conn.Write([]byte("!!"))
	if err != nil {
		baseLog.WithError(err).Error("Error writing to client")
		conn.Close()
		return
	}

	socketOpen := true
	go func() {
		for {
			message, err := bufio.NewReader(conn).ReadString('\n')
			if err != nil && err != io.EOF {
				baseLog.WithError(err).Error("Error reading from socket")
				socketOpen = false
				return
			} else if err != nil {
				baseLog.Info("Socket client disconnected")
				socketOpen = false
				return
			}
			parseLine(message, db, client)
		}
	}()

	for socketOpen {
		time.After(1 * time.Second)
	}
}

func main() {
	log.SetFormatter(&log.TextFormatter{
		FullTimestamp: true,
	})

	// Connnect to the postgres server
	psqlconn := fmt.Sprintf("host=%s port=%d user=%s password=%s dbname=%s sslmode=disable", host, port, user, password, dbname)
	db, err := sql.Open("postgres", psqlconn)
	if err != nil {
		log.WithField("err", err).Fatal("Unable to connect to database")
		return
	}
	defer db.Close()

	err = db.Ping()
	if err != nil {
		log.WithField("err", err).Fatal("Connection to DB lost")
		return
	}

	// Open and start the socket server
	server, err := net.Listen(SERVER_TYPE, SERVER_HOST+":"+SERVER_PORT)
	if err != nil {
		log.WithError(err).Panic("Error starting socket server")
	}
	log.WithField("host", SERVER_HOST).WithField("port", SERVER_PORT).Warn("Socket server started")
	defer server.Close()

	go func() {
		for {
			conn, err := server.Accept()
			if err != nil {
				log.WithError(err).Error("Error accepting incoming connections")
			} else {
				handleClient(conn, db)
			}
		}
	}()

	// Open and start listening on the pipe
	for {
		p, err := os.Open("/tmp/altus-tracker-fifo-out")
		if err != nil && errors.Is(err, os.ErrNotExist) {
			time.After(100 * time.Millisecond)
			continue
		}
		if err != nil {
			log.WithError(err).Panic("Failed to open pipe")
		}

		client := Client{
			Type:   "pipe",
			Source: "/tmp/altus-tracker-fifo-out",
		}
		client.SetId()
		client.Save(db)
		baseLog := log.WithField("client", client)
		baseLog.Info("Pipe source connected")

		pipeOpen := true
		go func() {
			for {
				message, err := bufio.NewReader(p).ReadString('\n')
				if err != nil && err != io.EOF {
					baseLog.WithError(err).Error("Error reading from pipe")
					pipeOpen = false
					return
				} else if err != nil {
					baseLog.Info("Pipe source disconnected")
					pipeOpen = false
					return
				}
				parseLine(message, db, client)
			}
		}()

		for pipeOpen {
			time.After(1 * time.Second)
		}
		p.Close()
		client.Delete(db)
	}
}
