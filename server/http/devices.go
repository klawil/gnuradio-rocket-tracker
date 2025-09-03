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
	CombinedState interface{}
}

type listDevicesApiResponse struct {
	BaseApiResponse
	Devices []devicePacket
	Count   int
}

func listDevices(c *fiber.Ctx) error {
	var devices []devicePacket
	var count int = 0

	var rows *sql.Rows
	var err error
	if c.QueryBool("all") {
		rows, err = db.Query("SELECT device_name, device_serial, max_created_at, combined_state FROM devices_state")
	} else {
		rows, err = db.Query("SELECT device_name, device_serial, max_created_at, combined_state FROM devices_state WHERE max_created_at >= timezone('utc', now()) - INTERVAL '12 hours'")
	}
	if err != nil {
		log.WithError(err).Error("Failed to query devices")
		return sendError(c, err)
	}

	defer rows.Close()
	for rows.Next() {
		device := devicePacket{}
		var jsonStr []byte
		err = rows.Scan(
			&device.DeviceName,
			&device.DeviceSerial,
			&device.MaxCreatedAt,
			&jsonStr,
		)
		if err != nil {
			log.WithError(err).Error("Failed to get device row")
			continue
		}
		err = json.Unmarshal(
			jsonStr,
			&device.CombinedState,
		)

		count++
		devices = append(devices, device)
	}

	return c.JSON(listDevicesApiResponse{
		BaseApiResponse: BaseApiResponse{
			Success: true,
		},
		Devices: devices,
		Count:   count,
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

	Serial        uint32
	Name          sql.NullString
	DeviceType    sql.NullInt16
	CombinedState interface{}
	Packets       []interface{}
	Height        sql.NullInt32
	Speed         sql.NullInt32
	Accel         sql.NullInt32
	Altitude      sql.NullInt32
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
		"SELECT device_serial, device_name, min_created_at, combined_state FROM devices_state WHERE device_serial = $1",
		id,
	)
	var min_created_at time.Time
	if err != nil {
		baseLog.WithError(err).Error("Failed to get device information")
		return sendError(c, err)
	}
	if deviceRow.Next() {
		var jsonStr []byte
		err = deviceRow.Scan(
			&device.Serial,
			&device.Name,
			&min_created_at,
			&jsonStr,
		)
		if err != nil {
			log.WithError(err).Error("Failed to parse device row")
			return sendError(c, err)
		}
		err = json.Unmarshal(
			jsonStr,
			&device.CombinedState,
		)
		if err != nil {
			log.WithError(err).Error("Failed to parse device row JSON")
			return sendError(c, err)
		}
	}
	deviceRow.Close()

	// Get the max and most recent state
	maxState, err := db.Query(
		"SELECT MAX(CAST(packet_json->'Height' as integer)) as Height, MAX(CAST(packet_json->'Speed' as integer)) as Speed, MAX(CAST(packet_json->'Accel' as integer)) as Accel, MAX(CAST(packet_json->'Altitude' as integer)) as Altitude FROM packets WHERE CAST(packets.packet_json->'Serial' as integer) = $1 AND created_at >= $2",
		id,
		min_created_at,
	)
	if err != nil {
		log.WithField("id", id).WithField("min", min_created_at).WithError(err).Error("Failed to get max states")
		return sendError(c, err)
	}
	if maxState.Next() {
		err = maxState.Scan(
			&device.Height,
			&device.Speed,
			&device.Accel,
			&device.Altitude,
		)
		if err != nil {
			log.WithError(err).Error("Failed to parse device maximums")
			return sendError(c, err)
		}
	}
	maxState.Close()

	// Get the packets
	rows, err := db.Query(
		"SELECT packet_json FROM packets WHERE packet_json->>'Serial' = $1 ORDER BY created_at DESC LIMIT 200",
		id,
	)
	if err != nil {
		log.WithError(err).Error("Failed to query packets")
		return sendError(c, err)
	}
	for rows.Next() {
		var jsonStr []byte
		err = rows.Scan(&jsonStr)
		if err != nil {
			log.WithError(err).Error("Failed to parse packet row")
			return sendError(c, err)
		}
		var packet interface{}
		err = json.Unmarshal(
			jsonStr,
			&packet,
		)
		if err != nil {
			log.WithError(err).Error("Failed to parse packet row JSON")
			return sendError(c, err)
		}

		device.Packets = append(device.Packets, packet)
	}
	rows.Close()

	return c.JSON(device)
}
