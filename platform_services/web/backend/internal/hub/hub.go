package hub

import (
	"context"
	"encoding/json"
	"log"
	"sync"

	"lgtbot/web/backend/internal/bot"
	"lgtbot/proto/pb"

	"github.com/gorilla/websocket"
)

// Client is one authenticated websocket connection (platform_user_id = web username).
type Client struct {
	UserID string
	Conn   *websocket.Conn
	Send   chan []byte
	Hub    *Hub
}

func (c *Client) WritePump() {
	defer func() {
		c.Hub.unregister <- c
		c.Conn.Close()
	}()
	for msg := range c.Send {
		if err := c.Conn.WriteMessage(websocket.TextMessage, msg); err != nil {
			return
		}
	}
}

func (c *Client) ReadPump(handle func(uid string, text string)) {
	defer func() {
		c.Hub.unregister <- c
		c.Conn.Close()
	}()
	for {
		_, data, err := c.Conn.ReadMessage()
		if err != nil {
			return
		}
		var m struct {
			Type string `json:"type"`
			Text string `json:"text"`
		}
		if json.Unmarshal(data, &m) != nil || m.Type != "send_message" {
			continue
		}
		handle(c.UserID, m.Text)
	}
}

type Hub struct {
	mu       sync.RWMutex
	clients  map[string]*Client
	register chan *Client
	unregister chan *Client

	bot       *bot.BotClient
	platform  string
	pushInput <-chan *pb.PushEvent
}

func NewHub(bc *bot.BotClient, platform string, push <-chan *pb.PushEvent) *Hub {
	return &Hub{
		clients:    make(map[string]*Client),
		register:   make(chan *Client),
		unregister: make(chan *Client),
		bot:        bc,
		platform:   platform,
		pushInput:  push,
	}
}

func (h *Hub) Register(c *Client) {
	h.register <- c
}

func (h *Hub) Run(ctx context.Context) {
	for {
		select {
		case <-ctx.Done():
			return
		case c := <-h.register:
			h.mu.Lock()
			if old, ok := h.clients[c.UserID]; ok {
				close(old.Send)
				old.Conn.Close()
			}
			h.clients[c.UserID] = c
			h.mu.Unlock()
		case c := <-h.unregister:
			h.mu.Lock()
			if cur, ok := h.clients[c.UserID]; ok && cur == c {
				delete(h.clients, c.UserID)
				close(c.Send)
			}
			h.mu.Unlock()
		case ev := <-h.pushInput:
			if ev == nil {
				continue
			}
			h.deliver(ev)
		}
	}
}

func (h *Hub) deliver(ev *pb.PushEvent) {
	var uid string
	switch t := ev.Target.(type) {
	case *pb.PushEvent_PlatformUserId:
		uid = t.PlatformUserId
	case *pb.PushEvent_PlatformGroupId:
		h.broadcastGroup(ev)
		return
	default:
		log.Printf("hub: skip push event sign=%d (no target)", ev.EventSign)
		return
	}
	msg := pushEventToJSON(ev)
	if msg == nil {
		return
	}
	h.mu.RLock()
	c, ok := h.clients[uid]
	h.mu.RUnlock()
	if !ok {
		return
	}
	select {
	case c.Send <- msg:
	default:
	}
}

func (h *Hub) broadcastGroup(ev *pb.PushEvent) {
	msg := pushEventToJSON(ev)
	if msg == nil {
		return
	}
	h.mu.RLock()
	defer h.mu.RUnlock()
	for _, c := range h.clients {
		select {
		case c.Send <- msg:
		default:
		}
	}
}

func pushEventToJSON(ev *pb.PushEvent) []byte {
	switch e := ev.Event.(type) {
	case *pb.PushEvent_Message:
		payload := map[string]any{
			"type": "bot_message",
			"items": messageToItems(e.Message),
		}
		b, err := json.Marshal(payload)
		if err != nil {
			return nil
		}
		return b
	case *pb.PushEvent_CommandsUpdate:
		payload := map[string]any{
			"type":     "commands_update",
			"commands": e.CommandsUpdate.Commands,
		}
		b, err := json.Marshal(payload)
		if err != nil {
			return nil
		}
		return b
	default:
		return nil
	}
}

func messageToItems(m *pb.Message) []map[string]any {
	if m == nil {
		return nil
	}
	var items []map[string]any
	for _, it := range m.Items {
		switch x := it.Msg.(type) {
		case *pb.MsgItem_Text:
			items = append(items, map[string]any{"type": "text", "content": x.Text})
		case *pb.MsgItem_Html:
			items = append(items, map[string]any{"type": "html", "content": x.Html})
		case *pb.MsgItem_Image:
			items = append(items, map[string]any{
				"type":    "image_url",
				"content": x.Image.Url,
				"delete_after_send": x.Image.DeleteAfterSend,
			})
		case *pb.MsgItem_AtPlatformUserId:
			items = append(items, map[string]any{"type": "at", "content": x.AtPlatformUserId})
		}
	}
	return items
}
