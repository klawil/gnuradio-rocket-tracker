package http

import (
	"database/sql"
	"encoding/json"
	"time"

	"github.com/gofiber/fiber/v2"
	log "github.com/sirupsen/logrus"
)

type devicePacket struct {
	DeviceName    sql.NullString
	DeviceType    sql.NullInt16
	DeviceSerial  uint16
	MaxCreatedAt  time.Time
	CombinedState string
}

func listDevices(c *fiber.Ctx) error {
	var devices []devicePacket
	var count int = 0

	var rows *sql.Rows
	var err error
	if c.QueryBool("all") {
		rows, err = db.Query("SELECT device_name, device_type, device_serial, max_created_at, combined_state #>> '{}' FROM devices_state")
	} else {
		rows, err = db.Query("SELECT device_name, device_type, device_serial, max_created_at, combined_state #>> '{}' FROM devices_state WHERE max_created_at >= timezone('utc', now()) - INTERVAL '12 days'")
	}
	if err != nil {
		log.WithError(err).Error("Failed to query devices")
		return sendError(c, err)
	}

	defer rows.Close()
	for rows.Next() {
		device := devicePacket{}
		err = rows.Scan(
			&device.DeviceName,
			&device.DeviceType,
			&device.DeviceSerial,
			&device.MaxCreatedAt,
			&device.CombinedState,
		)
		if err != nil {
			log.WithError(err).Error("Failed to get device row")
			continue
		}

		count++
		devices = append(devices, device)
	}

	return c.JSON(DevicesApiResponse{
		BaseApiResponse: BaseApiResponse{
			Success: true,
		},
		Data:  devices,
		Count: count,
	})
}

type patchDeviceBody struct {
	Name string
	Type uint8
}

func patchDevice(c *fiber.Ctx) error {
	baseLog := log.WithFields(log.Fields{
		"body": string(c.Body()),
		"id":   c.Params("id"),
	})

	id, err := c.ParamsInt("id")
	if err != nil {
		baseLog.WithError(err).Error("Invalid ID")
		return sendError(c, err)
	}

	// Parse the body
	var body patchDeviceBody
	err = json.Unmarshal(c.Body(), &body)
	if err != nil {
		baseLog.WithError(err).Error("Failed to unmarshal body")
		return sendError(c, err)
	}
	// if body.Name == "" {
	// 	baseLog.Error("Missing Name in body")
	// 	return c.JSON(BaseApiResponse{
	// 		Success: false,
	// 		Msg:     "Name is missing",
	// 	})
	// }
	baseLog.WithField("bodyp", body).Info("Patch request")

	// Update the database
	if body.Name == "" && body.Type == 0 {
		_, err = db.Exec(
			"INSERT INTO devices (serial, name, device_type) VALUES ($1, NULL, NULL) ON CONFLICT (serial) DO UPDATE SET name = NULL, device_type = NULL",
			id,
		)
	} else if body.Name == "" {
		_, err = db.Exec(
			"INSERT INTO devices (serial, name, device_type) VALUES ($1, NULL, $2) ON CONFLICT (serial) DO UPDATE SET name = NULL, device_type = $2",
			id,
			body.Type,
		)
	} else if body.Type == 0 {
		_, err = db.Exec(
			"INSERT INTO devices (serial, name, device_type) VALUES ($1, $2, NULL) ON CONFLICT (serial) DO UPDATE SET name = $2, device_type = NULL",
			id,
			body.Name,
		)
	} else {
		_, err = db.Exec(
			"INSERT INTO devices (serial, name, device_type) VALUES ($1, $2, $3) ON CONFLICT (serial) DO UPDATE SET name = $2, device_type = $3",
			id,
			body.Name,
			body.Type,
		)
	}
	if err != nil {
		baseLog.WithError(err).Error("Failed to update db")
		return sendError(c, err)
	}

	return c.JSON(BaseApiResponse{
		Success: true,
		Msg:     c.Params("id"),
	})
}

func deleteDevice(c *fiber.Ctx) error {
	baseLog := log.WithFields(log.Fields{
		"id": c.Params("id"),
	})

	id, err := c.ParamsInt("id")
	if err != nil {
		baseLog.WithError(err).Error("Invalid ID")
		return sendError(c, err)
	}

	// Update the database
	_, err = db.Exec(
		"DELETE FROM devices WHERE serial = $1",
		id,
	)
	if err != nil {
		baseLog.WithError(err).Error("Failed to update db")
		return sendError(c, err)
	}

	return c.JSON(BaseApiResponse{
		Success: true,
		Msg:     c.Params("id"),
	})
}

type deviceApiResponse struct {
	BaseApiResponse

	Serial     uint32
	CreatedAt  time.Time
	Name       sql.NullString
	DeviceType sql.NullInt16
	Packets    []string
}

func getDevice(c *fiber.Ctx) error {
	baseLog := log.WithFields(log.Fields{
		"id": c.Params("id"),
	})

	id, err := c.ParamsInt("id")
	if err != nil {
		baseLog.WithError(err).Error("Invalid ID")
		return sendError(c, err)
	}

	var device deviceApiResponse
	device.Success = true

	// Get the device table information
	deviceRow, err := db.Query(
		"SELECT serial, created_at, name, device_type FROM devices WHERE serial = $1",
		id,
	)
	if err != nil {
		baseLog.WithError(err).Error("Failed to get device information")
		return sendError(c, err)
	}
	if deviceRow.Next() {
		err = deviceRow.Scan(
			&device.Serial,
			&device.CreatedAt,
			&device.Name,
			&device.DeviceType,
		)
		if err != nil {
			log.WithError(err).Error("Failed to parse device row")
			return sendError(c, err)
		}
	}
	deviceRow.Close()

	// Get the packets
	rows, err := db.Query(
		"SELECT packet_json FROM packets WHERE packet_json->>'serial' = $1 ORDER BY created_at DESC LIMIT 200",
		id,
	)
	if err != nil {
		log.WithError(err).Error("Failed to query packets")
		return sendError(c, err)
	}
	for rows.Next() {
		var packet string
		err = rows.Scan(&packet)
		if err != nil {
			log.WithError(err).Error("Failed to parse packet row")
			return sendError(c, err)
		}

		device.Packets = append(device.Packets, packet)
	}
	rows.Close()

	return c.JSON(device)
}
