package main

import (
	"database/sql"
	"fmt"
	"rocket_server/socket"
	"sync"

	_ "github.com/lib/pq"
	log "github.com/sirupsen/logrus"
)

const (
	host     = "localhost"
	port     = 5432
	user     = "rockettracker"
	password = "rockettracker"
	dbname   = "rockettracker"
)

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
	var wg sync.WaitGroup
	wg.Add(1)
	go socket.Main(db, &wg)
	wg.Wait()
}
