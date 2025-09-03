package http

import (
	"database/sql"
	"fmt"
	"io"
	"math"
	"net/http"
	"os"
	"strings"
	"sync"
	"time"

	"github.com/gofiber/contrib/websocket"
	"github.com/gofiber/fiber/v2"
	"github.com/gofiber/fiber/v2/middleware/proxy"
	log "github.com/sirupsen/logrus"
)

type BaseApiResponse struct {
	Success bool
	Msg     string
}

func sendError(c *fiber.Ctx, e error) error {
	c.JSON(BaseApiResponse{
		Success: false,
		Msg:     e.Error(),
	})
	return c.SendStatus(500)
}

type downloadInfo struct {
	Url      string
	Filename string
}

const (
	startZoom uint8  = 18
	endZoom   uint8  = 14
	tileDir   string = "tiles"
)

var publicDir string
var db *sql.DB

func getMapTileServer() string {
	envServer := os.Getenv("MAP_SERVER")
	if envServer != "" {
		return envServer
	}

	return "127.0.0.1:3334"
}

func downloadTiles(c chan downloadInfo, max uint16, taken *uint16, wg *sync.WaitGroup) {
	defer wg.Done()

	for {
		select {
		case dl := <-c:
			{
				*taken++
				log.Infof("Taken file: %v", dl.Filename)

				// Create the file
				file, err := os.Create(fmt.Sprintf("%v/%v/%v.jpg", publicDir, tileDir, dl.Filename))
				if err != nil {
					log.WithError(err).WithFields(log.Fields{
						"fn": dl.Filename,
					}).Error("Failed to create file")
					break
				}

				// Download the image
				client := http.Client{}
				resp, err := client.Get(dl.Url)
				if err != nil {
					log.WithError(err).WithFields(log.Fields{
						"fn": dl,
					}).Error("Failed to download file")
					break
				}
				size, err := io.Copy(file, resp.Body)
				if err != nil {
					log.WithError(err).Error("Failed to copy to file")
				}

				log.WithFields(log.Fields{
					"dl": dl,
					"s":  size,
				}).Info("Downloaded")

				resp.Body.Close()
				file.Close()

				if *taken >= max {
					return
				}
			}
		}
	}
}

func downloadMaps(minY int, maxY int, minX int, maxX int) {
	log.WithFields(log.Fields{
		"minY": minY,
		"maxY": maxY,
		"minX": minX,
		"maxX": maxX,
	}).Info("Downloading maps")

	// Figure out the tile distance to download
	radius := float64((maxX - minX) / 2)
	if float64((maxY-minY)/2) > radius {
		radius = float64((maxY - minY) / 2)
	}
	log.WithFields(log.Fields{
		"radX": (maxX - minX) / 2,
		"radY": (maxY - minY) / 2,
		"rad":  radius,
	}).Info("Radius")
	centerX := float64((maxX+minX)/2) + 0.5
	centerY := float64((maxY+minY)/2) + 0.5

	// Figure out how many tiles need to be downloaded
	tileCount := uint16(0)
	tiles := make(map[uint8]map[string]bool)
	var tileData []downloadInfo
	for zoom := startZoom; zoom >= endZoom; zoom-- {
		tiles[zoom] = make(map[string]bool)
	}
	// Calculate the info for each square
	for y := minY; y <= maxY; y++ {
		for x := minX; x <= maxX; x++ {
			// Pythagoras it up
			floatX := float64(x) + 0.5
			floatY := float64(y) + 0.5
			distance := math.Sqrt(
				(centerX-floatX)*(centerX-floatX) + (centerY-floatY)*(centerY-floatY),
			)
			if (distance - 0.5) <= radius {
				// Add the current zoom level
				tileName := fmt.Sprintf("%v/%v.png", y, x)
				tileData = append(tileData, downloadInfo{
					Url: fmt.Sprintf(
						"https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%v/%v/%v.jpg",
						startZoom,
						y,
						x,
					),
					Filename: fmt.Sprintf("%v-%v-%v", startZoom, y, x),
				})
				tiles[startZoom][tileName] = true
				tileCount++

				// Handle the other zoom levels
				zoomX := x
				zoomY := y
				for z := startZoom - 1; z >= endZoom; z-- {
					zoomX /= 2
					zoomY /= 2
					tileName := fmt.Sprintf("%v/%v.png", zoomY, zoomX)
					_, e := tiles[z][tileName]
					if !e {
						tiles[z][tileName] = true
						tileData = append(tileData, downloadInfo{
							Url: fmt.Sprintf(
								"https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%v/%v/%v.jpg",
								z,
								zoomY,
								zoomX,
							),
							Filename: fmt.Sprintf("%v-%v-%v", z, zoomY, zoomX),
						})
						tileCount++
					}
				}
			}
		}
	}

	// Make a channel for parallel processing
	downloadChannel := make(chan downloadInfo, tileCount)
	for i := 0; i < int(tileCount); i++ {
		downloadChannel <- tileData[i]
	}

	// Spin up the processors
	var wg sync.WaitGroup
	taken := uint16(0)
	for i := 0; i < 20; i++ {
		wg.Add(1)
		go downloadTiles(downloadChannel, tileCount, &taken, &wg)
	}

	wg.Wait()
	log.WithFields(log.Fields{
		"c": tileCount,
	}).Warn("Done downloading maps")
}

func downloadMapApi(c *fiber.Ctx) error {
	invalidFields := []string{}
	minX := c.QueryInt("minX")
	minY := c.QueryInt("minY")
	maxX := c.QueryInt("maxX")
	maxY := c.QueryInt("maxY")
	if minX == 0 {
		invalidFields = append(invalidFields, "minX")
	}
	if minY == 0 {
		invalidFields = append(invalidFields, "minY")
	}
	if maxX == 0 {
		invalidFields = append(invalidFields, "maxX")
	}
	if maxY == 0 {
		invalidFields = append(invalidFields, "maxY")
	}
	if minY > maxY {
		invalidFields = append(invalidFields, "y")
	}
	if minX > maxX {
		invalidFields = append(invalidFields, "x")
	}

	if len(invalidFields) > 0 {
		return c.JSON(BaseApiResponse{
			Success: false,
			Msg:     strings.Join(invalidFields, ", "),
		})
	}

	go downloadMaps(minY, maxY, minX, maxX)

	return c.JSON(BaseApiResponse{
		Success: true,
		// Str:     downloadMaps(minY, maxY, minX, maxX),
	})
}

func Main(dbConn *sql.DB, staticDir string, wg *sync.WaitGroup) {
	db = dbConn
	publicDir = staticDir

	go runHub()

	app := fiber.New()

	// Websocket API
	app.Use("/ws", wsMiddleware)
	app.Get("/ws", websocket.New(handleWs))

	// app.Get("/api/clients", getClients)
	app.Get("/api/map/download", downloadMapApi)

	// Devices API
	app.Get("/api/devices", listDevices)
	app.Patch("/api/devices/:id", patchDevice)
	app.Get("/api/devices/:id", getDevice)
	app.Delete("/api/devices/:id", deleteDevice)

	// Channels API
	app.Get("/api/channels", listDevices)
	app.Patch("/api/channels/:id", patchDevice)
	app.Get("/api/channels/:id", getDevice)
	app.Delete("/api/channels/:id", deleteDevice)

	app.Get("/js/service_worker.js", func(c *fiber.Ctx) error {
		c.Response().Header.Set("Service-Worker-Allowed", "/")
		return c.Next()
	})

	app.Use("/osm/tile/:z/:x/:y.png", func(ctx *fiber.Ctx) error {
		err := proxy.DoTimeout(
			ctx,
			fmt.Sprintf(
				"http://%s/tile/%v/%v/%v.png",
				getMapTileServer(),
				ctx.Params("z"),
				ctx.Params("x"),
				ctx.Params("y"),
			),
			time.Second*2,
		)
		if err != nil {
			log.WithError(err).Error("Error forwarding request")
		} else {
			return nil
		}
		return nil
	})

	app.Static("/", staticDir)

	// Tiles that haven't been loaded (any that were loaded will hit static above)
	app.Use("/tiles", func(ctx *fiber.Ctx) error {
		imageIds := strings.Replace(
			strings.Replace(
				ctx.Path(),
				"/tiles/",
				"",
				-1,
			),
			".png",
			"",
			-1,
		)
		imageIdsSplit := strings.Split(imageIds, "-")
		url := fmt.Sprintf(
			"https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%v/%v/%v.jpg",
			imageIdsSplit[0],
			imageIdsSplit[1],
			imageIdsSplit[2],
		)
		err := proxy.DoTimeout(
			ctx,
			url,
			time.Second*2,
		)
		if err != nil {
			log.WithError(err).Error("Error forwarding request")
			return ctx.Redirect(url, 302)
		} else {
			return nil
		}
		// return nil
		// return ctx.Redirect("/icons/not-loaded-map.jpg", 302)
	})

	err := app.Listen(":8080")
	if err != nil {
		log.WithError(err).Fatal("Failed to start HTTP server")
	}
	wg.Done()
}
