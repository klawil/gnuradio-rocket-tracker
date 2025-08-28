package http

import (
	"encoding/json"
	"rocket_server/model"

	"github.com/gofiber/contrib/websocket"
	"github.com/gofiber/fiber/v2"
	log "github.com/sirupsen/logrus"
)

type subscriptionType string

type wsSubscription struct {
	Type subscriptionType
	ID   uint16
}

const (
	SubscriptionLocation subscriptionType = "LOCATION"
	SubscriptionRocket   subscriptionType = "ROCKET"
)

type wsClient struct {
	Subscription wsSubscription
}

func (c wsClient) SubscribedTo(msg WsMessage) bool {
	// Check for a location packets
	isLocPacket := msg.Type == 0x05 || msg.Type == 0x04 || msg.Type == 0x0A || msg.Type == 0x11
	isLocPacket = isLocPacket || msg.Type == 0x09 || msg.Type == 0x10 || msg.Type == 0x15

	sub := c.Subscription

	// Check the type
	if sub.Type == SubscriptionLocation && isLocPacket {
		return true
	}

	if sub.Type == SubscriptionRocket && msg.Serial == sub.ID {
		return true
	}

	return false
}

type WsMessage struct {
	model.BasePacket

	RawJson string
}

var clients = make(map[*websocket.Conn]*wsClient)
var Broadcast = make(chan WsMessage)

func runHub() {
	for {
		select {
		case message := <-Broadcast:
			for connection := range clients {
				client := clients[connection]
				if client.SubscribedTo(message) {
					err := connection.WriteMessage(websocket.TextMessage, []byte(message.RawJson))
					if err != nil {
						log.WithError(err).Error("Error sending WS message")
						delete(clients, connection)
						connection.WriteMessage(websocket.CloseMessage, []byte{})
						connection.Close()
					}
				}
			}
		}
	}
}

func wsMiddleware(c *fiber.Ctx) error {
	if websocket.IsWebSocketUpgrade(c) {
		c.Locals("allowed", true)
		return c.Next()
	}
	return fiber.ErrUpgradeRequired
}

func handleWs(c *websocket.Conn) {
	defer func() {
		delete(clients, c)
		c.Close()
	}()

	var client wsClient
	baseLog := log.WithField("client", client)
	clients[c] = &client
	baseLog.Info("WS client connected")

	var (
		mt  int
		msg []byte
		err error
	)
	for {
		mt, msg, err = c.ReadMessage()
		if err != nil {
			if !websocket.IsCloseError(err, websocket.CloseGoingAway) && !websocket.IsCloseError(err, websocket.CloseNormalClosure) {
				log.WithError(err).Error("Websocket read error")
			} else {
				log.Info("Websocket client disconnected")
			}
			return
		}

		if mt == websocket.TextMessage {
			msgLog := baseLog.WithField("msg", string(msg))
			msgLog.Debug("New WS message")

			if string(msg) == "ping" {
				continue
			}

			var msgStruct wsSubscription
			err = json.Unmarshal(msg, &msgStruct)
			if err != nil {
				msgLog.WithError(err).Error("Unable to parse JSON message")
			} else {
				msgLog = msgLog.WithField("parsed", msgStruct)
				client.Subscription = msgStruct
				msgLog.Info("Added subscription")
			}
		} else {
			baseLog.WithField("type", mt).Warn("Weird WS message")
		}
	}
}
