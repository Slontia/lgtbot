package api

import (
	"context"
	"encoding/json"
	"errors"
	"net/http"
	"strings"
	"time"

	"lgtbot/web/backend/internal/auth"
	"lgtbot/web/backend/internal/bot"
	"lgtbot/web/backend/internal/hub"

	"github.com/gorilla/mux"
	"github.com/gorilla/websocket"
)

type Server struct {
	Router    *mux.Router
	store     *auth.Store
	bot       *bot.BotClient
	jwtSecret string
	platform  string
	hub       *hub.Hub

	upgrader websocket.Upgrader
}

func New(store *auth.Store, bc *bot.BotClient, h *hub.Hub, jwtSecret, platform string) *Server {
	s := &Server{
		store:     store,
		bot:       bc,
		hub:       h,
		jwtSecret: jwtSecret,
		platform:  platform,
		upgrader: websocket.Upgrader{
			CheckOrigin: func(r *http.Request) bool { return true },
		},
	}
	r := mux.NewRouter()
	r.HandleFunc("/api/health", s.health).Methods(http.MethodGet)
	r.HandleFunc("/api/auth/register", s.register).Methods(http.MethodPost)
	r.HandleFunc("/api/auth/login", s.login).Methods(http.MethodPost)
	r.HandleFunc("/api/games", s.games).Methods(http.MethodGet)
	r.HandleFunc("/api/game/start", s.startGame).Methods(http.MethodPost)
	r.HandleFunc("/ws", s.ws)
	s.Router = r
	return s
}

func writeJSON(w http.ResponseWriter, v any) {
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(v)
}

func (s *Server) health(w http.ResponseWriter, r *http.Request) {
	if r.URL.Query().Get("core") == "1" {
		if _, err := s.bot.GetGameList(r.Context()); err != nil {
			http.Error(w, err.Error(), http.StatusBadGateway)
			return
		}
	}
	writeJSON(w, map[string]string{"status": "ok"})
}

type credBody struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

func (s *Server) register(w http.ResponseWriter, r *http.Request) {
	var body credBody
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		http.Error(w, "bad json", http.StatusBadRequest)
		return
	}
	if err := s.store.Register(body.Username, body.Password); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	if err := s.bot.UpdateUserInfo(r.Context(), s.platform, body.Username, body.Username, nil); err != nil {
		http.Error(w, "core UpdateUserInfo: "+err.Error(), http.StatusBadGateway)
		return
	}
	token, err := auth.SignJWT(body.Username, s.jwtSecret, 24*time.Hour)
	if err != nil {
		http.Error(w, "jwt", http.StatusInternalServerError)
		return
	}
	writeJSON(w, map[string]any{"token": token, "username": body.Username})
}

func (s *Server) login(w http.ResponseWriter, r *http.Request) {
	var body credBody
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		http.Error(w, "bad json", http.StatusBadRequest)
		return
	}
	if err := s.store.Verify(body.Username, body.Password); err != nil {
		http.Error(w, "unauthorized", http.StatusUnauthorized)
		return
	}
	token, err := auth.SignJWT(body.Username, s.jwtSecret, 24*time.Hour)
	if err != nil {
		http.Error(w, "jwt", http.StatusInternalServerError)
		return
	}
	writeJSON(w, map[string]any{"token": token, "username": body.Username})
}

func (s *Server) games(w http.ResponseWriter, r *http.Request) {
	list, err := s.bot.GetGameList(r.Context())
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	writeJSON(w, map[string]any{"games": list})
}

func (s *Server) bearerUsername(r *http.Request) (string, error) {
	token := r.Header.Get("Authorization")
	if strings.HasPrefix(strings.ToLower(token), "bearer ") {
		token = strings.TrimSpace(token[7:])
	}
	if token == "" {
		return "", errors.New("missing Authorization Bearer token")
	}
	claims, err := auth.ParseJWT(token, s.jwtSecret)
	if err != nil {
		return "", err
	}
	return claims.Username, nil
}

func (s *Server) startGame(w http.ResponseWriter, r *http.Request) {
	user, err := s.bearerUsername(r)
	if err != nil {
		http.Error(w, err.Error(), http.StatusUnauthorized)
		return
	}
	var body struct {
		GameName string `json:"game_name"`
		Solo     bool   `json:"solo"`
	}
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		http.Error(w, "bad json", http.StatusBadRequest)
		return
	}
	if body.GameName == "" {
		http.Error(w, "game_name required", http.StatusBadRequest)
		return
	}
	if err := s.bot.StartGame(r.Context(), s.platform, user, body.GameName, body.Solo); err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	writeJSON(w, map[string]string{"status": "ok"})
}

func (s *Server) ws(w http.ResponseWriter, r *http.Request) {
	token := r.URL.Query().Get("token")
	if token == "" {
		token = r.Header.Get("Authorization")
	}
	if strings.HasPrefix(strings.ToLower(token), "bearer ") {
		token = strings.TrimSpace(token[7:])
	}
	if token == "" {
		http.Error(w, "missing token", http.StatusUnauthorized)
		return
	}
	claims, err := auth.ParseJWT(token, s.jwtSecret)
	if err != nil {
		http.Error(w, "bad token", http.StatusUnauthorized)
		return
	}
	conn, err := s.upgrader.Upgrade(w, r, nil)
	if err != nil {
		return
	}
	client := &hub.Client{
		UserID: claims.Username,
		Conn:   conn,
		Send:   make(chan []byte, 64),
		Hub:    s.hub,
	}
	s.hub.Register(client)
	go client.WritePump()
	client.ReadPump(func(uid, text string) {
		ctx, cancel := context.WithTimeout(context.Background(), 60*time.Second)
		defer cancel()
		_ = s.bot.HandlePrivateRequest(ctx, s.platform, uid, text)
	})
}
