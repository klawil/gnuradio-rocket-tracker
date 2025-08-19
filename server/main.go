package main

import (
	"database/sql"
	"fmt"
	"os"
	"rocket_server/http"
	"rocket_server/socket"
	"sync"

	_ "github.com/lib/pq"
	log "github.com/sirupsen/logrus"

	"github.com/golang-migrate/migrate/v4"
	"github.com/golang-migrate/migrate/v4/database/postgres"
	_ "github.com/golang-migrate/migrate/v4/source/file"
)

const (
	host      = "postgres"
	port      = "5432"
	user      = "rockettracker"
	password  = "rockettracker"
	dbname    = "rockettracker"
	staticDir = "./build"
)

func getDbHost() string {
	envHost := os.Getenv("DB_HOST")
	if envHost != "" {
		return envHost
	}

	return host
}

func getDbPort() string {
	envPort := os.Getenv("DB_PORT")
	if envPort != "" {
		return envPort
	}

	return port
}

func main() {
	log.SetFormatter(&log.TextFormatter{
		FullTimestamp: true,
	})

	// Connnect to the postgres server
	psqlconn := fmt.Sprintf("host=%s port=%s user=%s password=%s dbname=%s sslmode=disable", getDbHost(), getDbPort(), user, password, dbname)
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

	// Run the migrations
	driver, err := postgres.WithInstance(db, &postgres.Config{})
	if err != nil {
		log.WithError(err).Fatal("Unable to get migrations driver")
	}
	m, err := migrate.NewWithDatabaseInstance(
		"file:///src/db",
		"postgres", driver)
	if err != nil {
		log.WithError(err).Fatal("Failed to bring up database")
	}
	err = m.Up()
	if err != nil {
		log.WithError(err).Fatal("Failed to run migration")
	}

	// Open and start the socket server
	var wg sync.WaitGroup
	wg.Add(2)
	go socket.Main(db, &wg)
	go http.Main(db, staticDir, &wg)
	wg.Wait()
}
