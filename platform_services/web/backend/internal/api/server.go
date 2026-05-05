package api

import (
	"context"
	"encoding/json"
	"errors"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	"lgtbot/proto/pb"
	"lgtbot/web/backend/internal/auth"
	"lgtbot/web/backend/internal/bot"
	"lgtbot/web/backend/internal/hub"

	"github.com/gorilla/mux"
	"github.com/gorilla/websocket"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"
)

type Server struct {
	Router    *mux.Router
	store     *auth.Store
	bot       *bot.BotClient
	jwtSecret string
	platform  string
	gamePath  string
	imageRoot string
	hub       *hub.Hub

	upgrader websocket.Upgrader
}

func New(store *auth.Store, bc *bot.BotClient, h *hub.Hub, jwtSecret, platform, gamePath, imageRoot string) *Server {
	s := &Server{
		store:     store,
		bot:       bc,
		hub:       h,
		jwtSecret: jwtSecret,
		platform:  platform,
		gamePath:  gamePath,
		imageRoot: imageRoot,
		upgrader: websocket.Upgrader{
			CheckOrigin: func(r *http.Request) bool { return true },
		},
	}
	r := mux.NewRouter()
	r.PathPrefix("/static/images/").Methods(http.MethodGet, http.MethodHead).Handler(http.HandlerFunc(s.serveCoreStaticImages))
	r.HandleFunc("/api/health", s.health).Methods(http.MethodGet)
	r.HandleFunc("/api/auth/register", s.register).Methods(http.MethodPost)
	r.HandleFunc("/api/auth/login", s.login).Methods(http.MethodPost)
	r.HandleFunc("/api/games", s.games).Methods(http.MethodGet)
	r.HandleFunc("/api/games/{name}/matches", s.gameMatches).Methods(http.MethodGet)
	r.HandleFunc("/api/games/{name}/rule", s.gameRule).Methods(http.MethodGet)
	r.HandleFunc("/api/games/{name}/achievements", s.gameAchievements).Methods(http.MethodGet)
	r.HandleFunc("/api/games/{name}/rankings", s.gameRankings).Methods(http.MethodGet)
	r.HandleFunc("/api/game-modules/{module}/icon", s.gameModuleIcon).Methods(http.MethodGet)
	r.HandleFunc("/api/me", s.me).Methods(http.MethodGet)
	r.HandleFunc("/api/profile", s.profileStats).Methods(http.MethodGet)
	r.HandleFunc("/api/game/start", s.startGame).Methods(http.MethodPost)
	r.HandleFunc("/ws", s.ws)
	s.Router = r
	return s
}

func writeJSON(w http.ResponseWriter, v any) {
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(v)
}

var protoJSONMarshal = protojson.MarshalOptions{UseProtoNames: true}

func writeProtoJSON(w http.ResponseWriter, msg proto.Message) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	b, err := protoJSONMarshal.Marshal(msg)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	_, _ = w.Write(b)
}

func parseWebTimeRange(r *http.Request) pb.WebTimeRange {
	switch r.URL.Query().Get("time_range") {
	case "1", "year":
		return pb.WebTimeRange_WEB_TIME_RANGE_YEAR
	case "2", "all":
		return pb.WebTimeRange_WEB_TIME_RANGE_ALL
	default:
		return pb.WebTimeRange_WEB_TIME_RANGE_MONTH
	}
}

func isSafeGameModuleName(mod string) bool {
	if mod == "" || mod == "." || mod == ".." {
		return false
	}
	if strings.Contains(mod, "..") || strings.ContainsAny(mod, "/\\") {
		return false
	}
	for _, r := range mod {
		switch {
		case r >= 'a' && r <= 'z', r >= 'A' && r <= 'Z', r >= '0' && r <= '9':
		case r == '_' || r == '-' || r == '.':
		default:
			return false
		}
	}
	return true
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

func (s *Server) me(w http.ResponseWriter, r *http.Request) {
	user, err := s.bearerUsername(r)
	if err != nil {
		http.Error(w, err.Error(), http.StatusUnauthorized)
		return
	}
	info, err := s.bot.GetUserInfo(r.Context(), s.platform, user)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	writeJSON(w, map[string]any{
		"username":     user,
		"display_name": info.GetDisplayName(),
		"avatar_url":   info.GetAvatarUrl(),
	})
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

func (s *Server) gameMatches(w http.ResponseWriter, r *http.Request) {
	name := mux.Vars(r)["name"]
	q := r.URL.Query()
	off64, _ := strconv.ParseUint(q.Get("offset"), 10, 32)
	lim64, _ := strconv.ParseUint(q.Get("limit"), 10, 32)
	off := uint32(off64)
	lim := uint32(lim64)
	if lim == 0 {
		lim = 50
	}
	resp, err := s.bot.WebListGameMatches(r.Context(), s.platform, name, off, lim)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	writeProtoJSON(w, resp)
}

func (s *Server) gameRule(w http.ResponseWriter, r *http.Request) {
	name := mux.Vars(r)["name"]
	resp, err := s.bot.WebGetGameRule(r.Context(), name)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	writeProtoJSON(w, resp)
}

func (s *Server) gameAchievements(w http.ResponseWriter, r *http.Request) {
	name := mux.Vars(r)["name"]
	viewer := ""
	if u, err := s.bearerUsername(r); err == nil {
		viewer = u
	}
	resp, err := s.bot.WebGetGameAchievements(r.Context(), name, viewer)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	writeProtoJSON(w, resp)
}

func (s *Server) gameRankings(w http.ResponseWriter, r *http.Request) {
	name := mux.Vars(r)["name"]
	top64, _ := strconv.ParseUint(r.URL.Query().Get("top_n"), 10, 32)
	top := uint32(top64)
	if top == 0 {
		top = 20
	}
	tr := parseWebTimeRange(r)
	resp, err := s.bot.WebGetGameRankings(r.Context(), s.platform, name, tr, top)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	writeProtoJSON(w, resp)
}

func (s *Server) gameModuleIcon(w http.ResponseWriter, r *http.Request) {
	mod := mux.Vars(r)["module"]
	if !isSafeGameModuleName(mod) {
		http.NotFound(w, r)
		return
	}
	baseAbs, err := filepath.Abs(s.gamePath)
	if err != nil {
		http.Error(w, "game path invalid", http.StatusInternalServerError)
		return
	}
	fullPath := filepath.Clean(filepath.Join(baseAbs, mod, "icon.png"))
	rel, err := filepath.Rel(baseAbs, fullPath)
	if err != nil || strings.HasPrefix(rel, "..") {
		http.NotFound(w, r)
		return
	}
	st, err := os.Stat(fullPath)
	if err != nil || st.IsDir() {
		http.NotFound(w, r)
		return
	}
	f, err := os.Open(fullPath)
	if err != nil {
		http.NotFound(w, r)
		return
	}
	defer f.Close()
	w.Header().Set("Content-Type", "image/png")
	http.ServeContent(w, r, "icon.png", st.ModTime(), f)
}

// Serves files under the same root as lgtbot_grpc_server LGTBOT_IMAGE_PATH (avatar PNGs use /static/images/... URLs from core).
func (s *Server) serveCoreStaticImages(w http.ResponseWriter, r *http.Request) {
	const prefix = "/static/images/"
	if !strings.HasPrefix(r.URL.Path, prefix) {
		http.NotFound(w, r)
		return
	}
	rel := strings.TrimPrefix(r.URL.Path, prefix)
	if rel == "" {
		http.NotFound(w, r)
		return
	}
	rel = filepath.ToSlash(rel)
	if strings.Contains(rel, "..") || strings.HasPrefix(rel, "/") {
		http.NotFound(w, r)
		return
	}

	baseAbs, err := filepath.Abs(s.imageRoot)
	if err != nil {
		http.Error(w, "image root invalid", http.StatusInternalServerError)
		return
	}

	full := filepath.Join(baseAbs, filepath.FromSlash(rel))
	fullClean, err := filepath.Abs(filepath.Clean(full))
	if err != nil {
		http.NotFound(w, r)
		return
	}

	relFinal, err := filepath.Rel(baseAbs, fullClean)
	if err != nil || strings.HasPrefix(relFinal, "..") {
		http.NotFound(w, r)
		return
	}

	st, err := os.Stat(fullClean)
	if err != nil || st.IsDir() {
		http.NotFound(w, r)
		return
	}

	http.ServeFile(w, r, fullClean)
}

func (s *Server) profileStats(w http.ResponseWriter, r *http.Request) {
	user, err := s.bearerUsername(r)
	if err != nil {
		http.Error(w, err.Error(), http.StatusUnauthorized)
		return
	}
	tr := parseWebTimeRange(r)
	resp, err := s.bot.WebUserProfile(r.Context(), s.platform, user, tr)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	writeProtoJSON(w, resp)
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
